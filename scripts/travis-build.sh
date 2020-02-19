#!/bin/bash

# This is the script called by TravisCI to do the needful.

set -euo pipefail

echo "CC is ${CC-unset}, CXX is ${CXX-unset}"
[[ -z ${CC+x}  ]] ||  ${CC} --version
[[ -z ${CXX+x} ]] || ${CXX} --version
ccache -s
ccache -z
make
ccache -s