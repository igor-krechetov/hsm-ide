#!/usr/bin/env bash
set -euo pipefail

build_dir="$1"
source_dir="$2"

export HSM_IDE_TEST_SCXML_ROOT="$source_dir/tests/data/scxml"
export QT_QPA_PLATFORM=offscreen
chmod +x ./tests/*
ctest --test-dir "$build_dir"