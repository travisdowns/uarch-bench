#!/bin/bash

# Extracts the nasm binary from the files in the ./rpms directory
# which must have the suffix .x86_64.rpm

set -euo pipefail

function deleteo {
    for rpm in *.asm; do
        prefix=${rpm%".asm"}
        rm "${prefix}.o"
    done
}

make -j4

for nasm in nasm-binaries/linux/*; do
    echo "Testing nasm: $nasm"
    deleteo
    make -j4 ASM=${nasm}
    ./uarch-bench --test-name=basic/*
done