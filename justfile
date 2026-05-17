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

build_run: build run

clean_build: clean build

test: build
    ctest --test-dir {{BUILD_DIR}}

list_tests:
    ctest --test-dir {{BUILD_DIR}} -N

list_test_usecases testname:
    {{BUILD_DIR}}/tests/{{testname}} -functions

one_test testname: build
    ctest --test-dir {{BUILD_DIR}} -R {{testname}}

test_usecase testname usecase:
    {{BUILD_DIR}}/tests/{{testname}} {{usecase}}
