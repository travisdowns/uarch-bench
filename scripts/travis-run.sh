#!/bin/bash

# This is the script called by TravisCI to do the needful.

set -euo pipefail

: ${TEST_TAG:='default'}
: ${TEST_NAME:='*'}

scripts/travis-build.sh
lscpu
./unit-test
echo "Running tests with name=$TEST_NAME and tag=$TEST_TAG"
./uarch-bench --test-name="$TEST_NAME" --test-tag="$TEST_TAG"