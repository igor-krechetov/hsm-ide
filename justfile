BUILD_DIR := env_var_or_default("HSM_IDE_BUILD_DIR", "./build")
CMAKE_TOOL := "qt-cmake"

clean:
    #!/usr/bin/env bash
    if [ -d {{BUILD_DIR}} ]; then
        cd {{BUILD_DIR}}
        rm -Rvf ./*
    else
        echo "No build directory found";
    fi

configure:
    #!/usr/bin/env bash
    set -euxo pipefail
    unset Qt5_DIR
    export PATH=$PATH:~/Qt/6.8.3/gcc_64/bin
    mkdir -p {{BUILD_DIR}}
    ./scripts/build/posix/configure.sh {{BUILD_DIR}} Debug "" . True True

build: configure
    #!/usr/bin/env bash
    set -euxo pipefail
    mkdir -p {{BUILD_DIR}}
    # Only clear the screen on an interactive terminal that supports it;
    # skip in non-interactive/CI shells where `clear` errors on an unset TERM.
    if [ -t 1 ] && [ -n "${TERM:-}" ] && command -v clear >/dev/null 2>&1; then clear; fi
    ./scripts/build/posix/build.sh {{BUILD_DIR}} Debug

run:
    {{BUILD_DIR}}/src/hsm_ide

build_run: build run

clean_build: clean build

test: build
    # ctest --test-dir {{BUILD_DIR}}
    ./scripts/test/ubuntu/test.sh {{BUILD_DIR}} $PWD

list_tests:
    ctest --test-dir {{BUILD_DIR}} -N

list_test_usecases testname:
    {{BUILD_DIR}}/tests/{{testname}} -functions

one_test testname: build
    ctest --test-dir {{BUILD_DIR}} -R {{testname}}

test_usecase testname usecase:
    {{BUILD_DIR}}/tests/{{testname}} {{usecase}} -maxwarnings 100000

# REQUIREMENTS

# Validate the TRLC requirements only (fast, CI-friendly).
req_validate:
    trlc --brief requirements/

# Build the requirements documentation: validate the TRLC model + requirements,
# then render the collapsible HTML view (descriptions + implementation status).
req_build:
    #!/usr/bin/env bash
    set -euo pipefail
    OUT={{BUILD_DIR}}/requirements
    mkdir -p "$OUT"
    # Validate the TRLC model + requirements.
    trlc requirements/
    # Render the single-page requirements document.
    python3 scripts/requirements/render_html.py "$OUT/hsm_ide_requirements.html"
