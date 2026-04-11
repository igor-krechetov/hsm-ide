#!/usr/bin/env bash
set -euo pipefail

build_dir="$1"
source_dir="$2"

cd $build_dir

export HSM_IDE_TEST_SCXML_ROOT="$source_dir/tests/data/scxml"
chmod +x ./tests/ModelTests
./tests/ModelTests