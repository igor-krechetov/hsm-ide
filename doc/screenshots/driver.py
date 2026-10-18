#!/usr/bin/env python3
"""Automated UI screenshot driver for hsm-ide.

This script drives the UNMODIFIED ``hsm_ide`` binary from the outside to
produce documentation screenshots. It launches the application under a headless
X server (Xvfb), opens a sample state machine through the UI, performs a short,
declarative list of keyboard actions, and captures the application window.

Nothing about the screenshots is hardcoded in C++ or in this script: the list
of screenshots lives entirely in ``scenarios.yaml`` next to this file. Adding a
new screenshot is a pure data change (one YAML entry, optionally a new
``.scxml``) -- no code edits required.

Prerequisites (Linux):
    system packages:  sudo apt-get install -y xvfb xdotool imagemagick
    python package:   pip install pyyaml

The hsm_ide binary does NOT accept a file path on the command line (see
src/main.cpp: ``MainEditorController::start()`` is called with no arguments and
no QCommandLineParser/arguments() usage exists). Therefore projects are opened
through the UI: Ctrl+O, type the absolute path, Enter.

The application window is located by WM_CLASS ``hsm_ide`` (from
src/view/ui/main.ui ``<class>hsm_ide</class>``), falling back to the window
title ``HSM IDE``. Matching by class is preferred because the title may later
gain a filename suffix while the class stays stable.

Usage:
    python3 doc/screenshots/driver.py [options]

Options:
    --only NAME            only run the scenario with this name (repeatable)
    --build-dir DIR        hsm-ide build directory (default: env
                           HSM_IDE_BUILD_DIR or ../../build relative to repo)
    --scenarios PATH       path to scenarios.yaml (default: next to this file)
    --out-dir DIR          output directory for PNGs
                           (default: <repo>/doc/_images/hsm-ide)
    --resolution WxHxD     Xvfb resolution (default: 1600x1000x24)
    --keep-going          continue to the next scenario if one fails
                           (default: stop on first failure)
"""

from __future__ import annotations

import argparse
import os
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

try:
    import yaml
except ImportError:  # pragma: no cover - dependency hint
    sys.stderr.write(
        "error: PyYAML is required. Install it with: pip install pyyaml\n"
    )
    sys.exit(2)


# --- paths -----------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent          # doc/screenshots
REPO_ROOT = SCRIPT_DIR.parent.parent                  # repo root
DEFAULT_SCENARIOS = SCRIPT_DIR / "scenarios.yaml"
DEFAULT_OUT_DIR = REPO_ROOT / "doc" / "_images" / "hsm-ide"

# Window identity (see module docstring).
WINDOW_CLASS = "hsm_ide"
WINDOW_TITLE = "HSM IDE"

# Timeouts / delays (seconds).
WINDOW_WAIT_TIMEOUT = 20.0     # max time to wait for the app window to appear
WINDOW_POLL_INTERVAL = 0.3     # how often to poll for the window
WINDOW_SETTLE_DELAY = 1.0      # extra settle time once the window is found
XVFB_WAIT_TIMEOUT = 10.0       # max time to wait for Xvfb to come up
OPEN_DIALOG_TIMEOUT = 15.0     # max time to wait for the Open dialog open/close
OPEN_LOAD_DELAY = 1.5          # settle after loading a project (scene render)
APP_EXIT_TIMEOUT = 8.0         # max time to wait for the app to exit

# Title of the modal file-open dialog (from the app's Open HSM action). Used to
# synchronize project opening instead of relying on fixed sleeps.
OPEN_DIALOG_TITLE = "Open SCXML File"


class DriverError(RuntimeError):
    """Raised for any unrecoverable driver failure."""


# --- small shell helpers ----------------------------------------------------

def _run(cmd, *, env=None, check=True, capture=False, timeout=None):
    """Run a command, returning CompletedProcess. Raise DriverError on failure."""
    try:
        result = subprocess.run(
            cmd,
            env=env,
            check=False,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.PIPE if capture else None,
            text=True,
            timeout=timeout,
        )
    except FileNotFoundError as exc:
        raise DriverError(f"command not found: {cmd[0]}") from exc
    except subprocess.TimeoutExpired as exc:
        raise DriverError(f"command timed out: {' '.join(cmd)}") from exc
    if check and result.returncode != 0:
        detail = ""
        if capture and result.stderr:
            detail = f"\n{result.stderr.strip()}"
        raise DriverError(
            f"command failed ({result.returncode}): {' '.join(cmd)}{detail}"
        )
    return result


def check_dependencies():
    """Verify required external tools are installed; exit clearly if not."""
    required = {
        "Xvfb": "sudo apt-get install -y xvfb",
        "xdotool": "sudo apt-get install -y xdotool",
        "import": "sudo apt-get install -y imagemagick",
    }
    missing = []
    for tool, hint in required.items():
        if shutil.which(tool) is None:
            missing.append((tool, hint))
    if missing:
        lines = ["error: missing required tools:"]
        for tool, hint in missing:
            lines.append(f"  - {tool}  (install: {hint})")
        raise DriverError("\n".join(lines))


# --- headless display -------------------------------------------------------

# Window managers tried, in order of preference. A running WM is what makes
# focus/activation (_NET_ACTIVE_WINDOW) work under Xvfb; without one, xdotool
# windowactivate is refused and keystrokes never reach the app.
WINDOW_MANAGERS = (
    ["openbox"],
    ["fluxbox"],
    ["icewm"],
    ["jwm"],
    ["matchbox-window-manager"],
    ["twm"],
)


class HeadlessDisplay:
    """Manage an Xvfb instance (and a window manager) with cleanup on exit."""

    def __init__(self, resolution="1600x1000x24"):
        self.resolution = resolution
        parts = resolution.split("x")
        self.width = int(parts[0])
        self.height = int(parts[1])
        self.display = None
        self.proc = None
        self.wm_proc = None

    def _pick_display_number(self):
        for num in range(99, 130):
            if not Path(f"/tmp/.X11-unix/X{num}").exists():
                return num
        raise DriverError("no free X display number found in range 99-129")

    def start(self):
        num = self._pick_display_number()
        self.display = f":{num}"
        self.proc = subprocess.Popen(
            ["Xvfb", self.display, "-screen", "0", self.resolution, "-nolisten", "tcp"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        # Wait for the display socket to be ready.
        deadline = time.monotonic() + XVFB_WAIT_TIMEOUT
        socket_path = Path(f"/tmp/.X11-unix/X{num}")
        while time.monotonic() < deadline:
            if self.proc.poll() is not None:
                raise DriverError(
                    f"Xvfb exited early (code {self.proc.returncode})"
                )
            if socket_path.exists():
                print(f"[xvfb] display {self.display} ready ({self.resolution})")
                self._start_window_manager()
                return
            time.sleep(0.1)
        raise DriverError(f"Xvfb did not become ready within {XVFB_WAIT_TIMEOUT}s")

    def _start_window_manager(self):
        """Start a lightweight WM so window activation/focus works.

        Without a WM, Xvfb reports no _NET_ACTIVE_WINDOW support and xdotool
        refuses to activate windows, so keystrokes never reach the app.
        """
        env = self.env()
        for wm in WINDOW_MANAGERS:
            if shutil.which(wm[0]) is None:
                continue
            self.wm_proc = subprocess.Popen(
                wm, env=env,
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            )
            # Give the WM a moment to own the root window.
            time.sleep(1.0)
            if self.wm_proc.poll() is None:
                print(f"[wm] started {wm[0]}")
                return
            self.wm_proc = None
        print(
            "[wm] WARNING: no window manager found "
            f"(tried {', '.join(w[0] for w in WINDOW_MANAGERS)}); "
            "window activation may fail. Install one, e.g. "
            "`sudo apt-get install -y openbox`."
        )

    def env(self):
        env = os.environ.copy()
        env["DISPLAY"] = self.display
        return env

    def stop(self):
        for proc_attr in ("wm_proc", "proc"):
            proc = getattr(self, proc_attr, None)
            if proc and proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=5)
            setattr(self, proc_attr, None)


# --- the application under test ---------------------------------------------

class HsmIdeApp:
    """Launch and control a single hsm_ide process under a display."""

    def __init__(self, binary: Path, env: dict):
        self.binary = binary
        self.env = env
        self.proc = None
        self.window_id = None

    def launch(self):
        self.proc = subprocess.Popen(
            [str(self.binary)],
            env=self.env,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

    def wait_for_window(self):
        """Poll for the app window; return (window_id, match_description)."""
        deadline = time.monotonic() + WINDOW_WAIT_TIMEOUT
        attempts = (
            ("--class", WINDOW_CLASS, f"--class {WINDOW_CLASS}"),
            ("--name", WINDOW_TITLE, f'--name "{WINDOW_TITLE}"'),
        )
        while time.monotonic() < deadline:
            if self.proc.poll() is not None:
                raise DriverError(
                    f"hsm_ide exited early (code {self.proc.returncode}) "
                    "before its window appeared"
                )
            for flag, value, desc in attempts:
                result = _run(
                    ["xdotool", "search", "--onlyvisible", flag, value],
                    env=self.env, check=False, capture=True,
                )
                ids = [w for w in result.stdout.split() if w.strip()]
                if ids:
                    self.window_id = ids[-1]
                    time.sleep(WINDOW_SETTLE_DELAY)
                    return self.window_id, desc
            time.sleep(WINDOW_POLL_INTERVAL)
        raise DriverError(
            f"hsm_ide window not found within {WINDOW_WAIT_TIMEOUT}s "
            f"(tried --class {WINDOW_CLASS} and --name {WINDOW_TITLE})"
        )

    def focus(self):
        """Raise, activate, and focus the window before sending input."""
        _run(["xdotool", "windowactivate", "--sync", self.window_id],
             env=self.env, check=False)
        _run(["xdotool", "windowfocus", "--sync", self.window_id],
             env=self.env, check=False)
        _run(["xdotool", "windowraise", self.window_id],
             env=self.env, check=False)

    def maximize(self, width, height):
        """Fill the display so the diagram has room, then re-focus."""
        _run(["xdotool", "windowsize", self.window_id, str(width), str(height)],
             env=self.env, check=False)
        _run(["xdotool", "windowmove", self.window_id, "0", "0"],
             env=self.env, check=False)
        time.sleep(0.3)
        self.focus()

    def send_key(self, chord: str):
        self.focus()
        _run(["xdotool", "key", "--clearmodifiers", chord], env=self.env)

    def type_text(self, text: str):
        self.focus()
        _run(["xdotool", "type", "--clearmodifiers", "--", text], env=self.env)

    def _find_dialog(self):
        """Return the Open-dialog window id if present, else None."""
        result = _run(
            ["xdotool", "search", "--name", OPEN_DIALOG_TITLE],
            env=self.env, check=False, capture=True,
        )
        ids = [w for w in result.stdout.split() if w.strip()]
        return ids[-1] if ids else None

    def _wait_for_dialog(self, present, timeout):
        """Poll until the Open dialog is present/absent as requested."""
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            found = self._find_dialog() is not None
            if found == present:
                return True
            time.sleep(0.2)
        return False

    def open_project(self, abs_path: str):
        """Open a project through the UI (binary takes no file CLI arg).

        Synchronizes on the modal Open dialog rather than fixed sleeps: wait
        for the dialog to appear, type the path + Enter, then wait for the
        dialog to close (which means the file was accepted and loaded). This
        avoids flaky captures of the still-empty window on a slow launch.
        """
        self.send_key("ctrl+o")
        if not self._wait_for_dialog(present=True, timeout=OPEN_DIALOG_TIMEOUT):
            raise DriverError(
                f"Open dialog ('{OPEN_DIALOG_TITLE}') did not appear within "
                f"{OPEN_DIALOG_TIMEOUT}s"
            )
        time.sleep(0.4)  # let the dialog's path field take focus
        _run(["xdotool", "type", "--clearmodifiers", "--", abs_path], env=self.env)
        time.sleep(0.3)
        _run(["xdotool", "key", "--clearmodifiers", "Return"], env=self.env)
        if not self._wait_for_dialog(present=False, timeout=OPEN_DIALOG_TIMEOUT):
            raise DriverError(
                "Open dialog did not close after confirming the path "
                f"({abs_path}); the file may not have loaded"
            )
        time.sleep(OPEN_LOAD_DELAY)  # let the scene render after load

    def capture(self, out_path: Path):
        out_path.parent.mkdir(parents=True, exist_ok=True)
        time.sleep(0.3)
        # Capture the root window (whole display) rather than the app window id.
        # The window is maximized to fill the display, so the screen frames the
        # app exactly; and grabbing the root composites open menus/dialogs
        # correctly, whereas `import -window <appId>` renders transient popups
        # (File/View menus) as black because they are separate override-redirect
        # windows that are not children of the app window.
        _run(["import", "-window", "root", str(out_path)],
             env=self.env, capture=True)

    def close(self):
        """Close the app cleanly, then ensure the process is gone."""
        if self.window_id:
            _run(["xdotool", "key", "--clearmodifiers", "ctrl+q"],
                 env=self.env, check=False)
        if self.proc is None:
            return
        deadline = time.monotonic() + APP_EXIT_TIMEOUT
        while time.monotonic() < deadline:
            if self.proc.poll() is not None:
                self.proc = None
                return
            time.sleep(0.2)
        # Still alive (likely an unsaved-changes prompt): terminate.
        self.proc.terminate()
        try:
            self.proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.proc.kill()
            self.proc.wait(timeout=3)
        self.proc = None


# --- step dispatch ----------------------------------------------------------

def execute_step(app: HsmIdeApp, step: dict):
    """Execute one declarative step. New kinds slot in here."""
    if not isinstance(step, dict) or len(step) != 1:
        raise DriverError(f"invalid step (expected single-key mapping): {step!r}")
    (kind, value), = step.items()
    if kind in ("shortcut", "key"):
        app.send_key(str(value))
    elif kind == "wait":
        time.sleep(float(value))
    elif kind == "type":
        app.type_text(str(value))
    else:
        raise DriverError(f"unknown step kind: {kind!r}")


# --- scenario runner --------------------------------------------------------

def run_scenario(scenario: dict, binary: Path, display: HeadlessDisplay,
                 out_dir: Path):
    name = scenario.get("name")
    if not name:
        raise DriverError(f"scenario is missing a 'name': {scenario!r}")
    print(f"\n[scenario] {name}: {scenario.get('description', '').strip()}")

    app = HsmIdeApp(binary, display.env())
    try:
        app.launch()
        window_id, match_desc = app.wait_for_window()
        print(f"  window matched via {match_desc} -> id {window_id}")
        app.maximize(display.width, display.height)

        project = scenario.get("project")
        if project:
            abs_project = (REPO_ROOT / project).resolve()
            if not abs_project.is_file():
                raise DriverError(f"project file not found: {abs_project}")
            print(f"  opening project via UI: {abs_project}")
            app.open_project(str(abs_project))

        for step in scenario.get("steps", []) or []:
            execute_step(app, step)

        capture = scenario.get("capture") or {"window": True}
        if capture.get("window"):
            out_path = out_dir / f"{name}.png"
            app.capture(out_path)
            size = out_path.stat().st_size if out_path.exists() else 0
            print(f"  captured -> {out_path} ({size} bytes)")
        else:
            raise DriverError(f"unknown capture spec: {capture!r}")
    finally:
        app.close()


def load_scenarios(path: Path):
    if not path.is_file():
        raise DriverError(f"scenarios file not found: {path}")
    with path.open() as fh:
        data = yaml.safe_load(fh)
    if not isinstance(data, dict) or "screenshots" not in data:
        raise DriverError("scenarios file must have a top-level 'screenshots' list")
    scenarios = data["screenshots"]
    if not isinstance(scenarios, list) or not scenarios:
        raise DriverError("'screenshots' must be a non-empty list")
    return scenarios


def resolve_binary(build_dir_arg):
    if build_dir_arg:
        build_dir = Path(build_dir_arg)
    else:
        env_dir = os.environ.get("HSM_IDE_BUILD_DIR")
        build_dir = Path(env_dir) if env_dir else (REPO_ROOT / "build")
    build_dir = build_dir.resolve()
    binary = build_dir / "src" / "hsm_ide"
    if not binary.is_file():
        raise DriverError(
            f"hsm_ide binary not found at {binary}\n"
            "Build it first (e.g. `just build`) or pass --build-dir."
        )
    if not os.access(binary, os.X_OK):
        raise DriverError(f"hsm_ide binary is not executable: {binary}")
    return binary


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Capture hsm-ide UI screenshots headlessly (data-driven).",
    )
    parser.add_argument("--only", action="append", default=[], metavar="NAME",
                        help="only run the scenario(s) with this name (repeatable)")
    parser.add_argument("--build-dir", default=None,
                        help="hsm-ide build directory (default: env "
                             "HSM_IDE_BUILD_DIR or <repo>/build)")
    parser.add_argument("--scenarios", default=str(DEFAULT_SCENARIOS),
                        help="path to scenarios.yaml")
    parser.add_argument("--out-dir", default=str(DEFAULT_OUT_DIR),
                        help="output directory for PNGs")
    parser.add_argument("--resolution", default="1600x1000x24",
                        help="Xvfb resolution WxHxDepth (default 1600x1000x24)")
    parser.add_argument("--keep-going", action="store_true",
                        help="continue on scenario failure instead of stopping")
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv if argv is not None else sys.argv[1:])

    try:
        check_dependencies()
        binary = resolve_binary(args.build_dir)
        scenarios = load_scenarios(Path(args.scenarios))
    except DriverError as exc:
        sys.stderr.write(f"{exc}\n")
        return 2

    if args.only:
        wanted = set(args.only)
        scenarios = [s for s in scenarios if s.get("name") in wanted]
        if not scenarios:
            sys.stderr.write(f"no scenarios match --only {sorted(wanted)}\n")
            return 2

    out_dir = Path(args.out_dir).resolve()
    print(f"binary:     {binary}")
    print(f"out-dir:    {out_dir}")
    print(f"scenarios:  {[s.get('name') for s in scenarios]}")

    display = HeadlessDisplay(resolution=args.resolution)
    failures = []
    try:
        display.start()
        for scenario in scenarios:
            name = scenario.get("name", "<unnamed>")
            try:
                run_scenario(scenario, binary, display, out_dir)
            except DriverError as exc:
                failures.append((name, str(exc)))
                sys.stderr.write(f"[scenario] {name} FAILED: {exc}\n")
                if not args.keep_going:
                    break
    finally:
        display.stop()
        _reap_stray_xvfb()

    if failures:
        sys.stderr.write("\nFAILED scenarios:\n")
        for name, msg in failures:
            sys.stderr.write(f"  - {name}: {msg}\n")
        return 1

    print("\nAll scenarios captured successfully.")
    return 0


def _reap_stray_xvfb():
    """Best-effort: nothing to do normally; HeadlessDisplay owns its process.

    Present as a hook in case a future change spawns extra Xvfb instances.
    """
    return


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.stderr.write("\ninterrupted\n")
        sys.exit(130)
