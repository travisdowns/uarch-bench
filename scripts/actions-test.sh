#!/bin/bash

# This is the script called by GitHub actions.

set -euo pipefail

: ${TEST_TAG:='default'}
: ${TEST_NAME:='*'}

echo "Running tests with name=$TEST_NAME and tag=$TEST_TAG"
./uarch-bench --test-name="$TEST_NAME" --test-tag="$TEST_TAG"