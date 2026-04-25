build:
    #!/usr/bin/env bash
    set -euxo pipefail
    mkdir -p ./build
    cd ./build
    clear
    make -j5
    pwd

run:
    ./build/src/hsm_ide

test: build
    ./build/tests/ModelTests

build_run: build run
