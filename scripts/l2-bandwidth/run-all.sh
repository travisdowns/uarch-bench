#!/bin/bash

# runs all the tests needed for the l2 max bandwidth investigation at
# https://github.com/travisdowns/uarch-bench/wiki/Maxing-out-the-L2-cache
# to reproduce the results in a reasonable way you should have at least the following things configured:
# sudo cpupower -c all frequency-set -g performance
# disable turboboost
# ensure hugepages are allowed: /sys/kernel/mm/transparent_hugepage/enabled should be [always] or [madvise]

set -e

OUT_DIR=${1-./results}
mkdir -p $OUT_DIR

function do_test {
    TEST=$1 scripts/l2-bandwidth/tricky.sh $OUT_DIR/$2
    echo
}

# turn prefetcher off, this only works on Intel chips
echo "Disabling prefetch which needs sudo, you might be prompted for your root password"
sudo wrmsr -a 0x1a4 "$((2#1111))"

# plot using 
# eplot -x "UNROLLB (First/second read offset in lines)" -y "Cycles per line" -r '[][1.5:2.5]' $OUT_DIR/1-wide.cycles2
do_test bandwidth-oneloop-u1-128 1-wide.cycles
do_test bandwidth-oneloop-u2-128 2-wide.cycles

do_test bandwidth-normal-128 linearA.cycles
do_test bandwidth-normal-128 linearB.cycles

echo "Enabling prefetch again"
sudo wrmsr -a 0x1a4 0

do_test bandwidth-oneloop-u2-128 2-wide-pfonA.cycles
do_test bandwidth-oneloop-u2-128 2-wide-pfonB.cycles

do_test bandwidth-normal-128 linear-pfonA.cycles
do_test bandwidth-normal-128 linear-pfonB.cycles

