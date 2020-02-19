#!/bin/bash

# This is the script called by TravisCI to do the needful.

set -euo pipefail

scripts/travis-build.sh
lscpu
./unit-test
./uarch-bench --test-tag=~slow