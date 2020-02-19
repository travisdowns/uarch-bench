#!/bin/bash

# Extracts the nasm binary from the files in the ./rpms directory
# which must have the suffix .x86_64.rpm

set -euo pipefail

cd rpms

for rpm in *.x86_64.rpm; do
    prefix=${rpm%".x86_64.rpm"}
    echo "Processing $rpm ($prefix)"
    rpm2cpio "$rpm" | cpio -ivd './usr/bin/nasm'
    mv './usr/bin/nasm' "../linux/$prefix"
done

rmdir ./usr/bin
rmdir ./usr