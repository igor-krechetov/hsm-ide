BUILD_DIR := "./build"
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
    export PATH=$PATH:~/Qt/6.3.0/gcc_64/bin
    mkdir -p {{BUILD_DIR}}
    cd {{BUILD_DIR}}
    {{CMAKE_TOOL}} ..

build: configure
    #!/usr/bin/env bash
    set -euxo pipefail
    mkdir -p {{BUILD_DIR}}
    cd {{BUILD_DIR}}
    clear
    make -j5
    pwd

run:
    {{BUILD_DIR}}/src/hsm_ide

test: build
    {{BUILD_DIR}}/tests/ModelTests

build_run: build run

clean_build: clean build
