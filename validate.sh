#!/bin/bash

export HSM_IDE_TEST_SCXML_ROOT="$1/tests/data/scxml"
chmod +x ./tests/ModelTests
./tests/ModelTests
