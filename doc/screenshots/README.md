# hsm-ide UI screenshot system

An automated, data-driven way to produce the documentation screenshots of the
hsm-ide user interface. The driver launches the **unmodified** `hsm_ide` binary
under a headless X server, opens a sample state machine through the UI, performs
a short list of keyboard actions, and captures the application window.

Nothing about the screenshots is baked into the C++ application. The complete
list of screenshots lives in [`scenarios.yaml`](scenarios.yaml); the driver
([`driver.py`](driver.py)) just interprets it.

## Prerequisites

System tools (Debian/Ubuntu):

```bash
sudo apt-get install -y xvfb xdotool imagemagick openbox
```

- `xvfb` — headless X server (`Xvfb`), so no real display is needed.
- `xdotool` — sends keyboard/mouse events and finds the app window.
- `imagemagick` — provides `import`, used to grab the window as a PNG.
- `openbox` — a minimal window manager. Under a bare `Xvfb` there is no
  window manager, so `_NET_ACTIVE_WINDOW` is unsupported and window activation
  fails, which means keystrokes never reach the app. A WM fixes this. The
  driver auto-detects any of `openbox`, `fluxbox`, `icewm`, `jwm`,
  `matchbox-window-manager`, or `twm`; `openbox` is the lightest common choice.

Python dependency:

```bash
pip install pyyaml
```

You also need a built `hsm_ide` binary. It is found at
`<build-dir>/src/hsm_ide`, where the build directory defaults to the
environment variable `HSM_IDE_BUILD_DIR` or `<repo>/build`. Build it with
`just build` from the repository root if it does not exist yet.

## Running

From the repository root, the simplest path is the justfile recipe (it builds
first, then runs the driver):

```bash
just gen_screenshots
```

Or run the driver directly:

```bash
python3 doc/screenshots/driver.py                 # all scenarios
python3 doc/screenshots/driver.py --only overview # a single scenario
```

Useful options:

| Option              | Meaning                                                      |
| ------------------- | ------------------------------------------------------------ |
| `--only NAME`       | run just the named scenario (repeatable)                     |
| `--build-dir DIR`   | override the hsm-ide build directory                         |
| `--scenarios PATH`  | use a different scenarios file                               |
| `--out-dir DIR`     | override the output directory                                |
| `--resolution WxHxD`| Xvfb resolution (default `1600x1000x24`)                     |
| `--keep-going`      | continue to the next scenario if one fails                   |

## Output

Images are written to:

```
doc/_images/hsm-ide/<name>.png
```

one per scenario `name`. The directory is created automatically. These match
the image references in `doc/manual/manual.rst`.

## How to add a screenshot (pure data change — no code)

1. (Optional) Drop a new `.scxml` under `doc/` if you want a different diagram,
   and point the scenario's `project` at it (path relative to the repo root).
2. Add **one** entry to the `screenshots:` list in
   [`scenarios.yaml`](scenarios.yaml) with a unique `name`, the `steps` you
   want, and an optional `capture`.
3. Run `just screenshots` (or `python3 doc/screenshots/driver.py --only NAME`).

No C++ and no `driver.py` changes are required.

### Scenario schema (summary)

```yaml
screenshots:
  - name: my-shot              # output basename -> doc/_images/hsm-ide/my-shot.png
    description: "..."         # optional
    project: doc/sample.scxml  # optional; opened via the UI (Ctrl+O)
    steps:                     # optional, ordered
      - shortcut: "ctrl+shift+e"  # xdotool key chord
      - key: "alt+f"              # xdotool key chord (same mechanism)
      - wait: 0.5                 # seconds
      - type: "some text"         # literal text
    capture:
      window: true             # capture the whole app window (default)
```

New step kinds can be added by extending the small dispatch map in
`execute_step()` in `driver.py`; the four kinds above cover the current
scenarios. See the comment block at the top of `scenarios.yaml` for the full
schema and a list of useful hsm-ide shortcuts.

## How it works (notes)

- **No CLI file argument.** `src/main.cpp` hands `argc`/`argv` to `QApplication`
  and then calls `MainEditorController::start()` with no arguments, so the
  binary cannot open a file from the command line. The driver therefore opens
  projects through the UI: `Ctrl+O`, type the absolute path, `Enter`.
- **Window identity.** The window is located by its WM_CLASS `hsm_ide` (from
  `src/view/ui/main.ui`), falling back to the title `HSM IDE`. Matching by class
  is more stable if the title later gains a filename suffix.
- **Headless.** A dedicated `Xvfb` display is started per run and torn down on
  exit (including on failure).
