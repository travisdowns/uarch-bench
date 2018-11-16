#!/bin/bash

# Creates the plots used in https://github.com/travisdowns/uarch-bench/wiki/Getting-everything-the-L2-has-to-give

set -e

IN_DIR=${1-./results}
OUT_DIR=${1-./plots}

OUT_DIR=$(realpath "$OUT_DIR")
mkdir -p "$OUT_DIR"

echo "Reading data from $IN_DIR and writing to $OUT_DIR"

XTITLE="UNROLLB (First/second read offset in lines)"
YTITLE="Cycles per line"

cd "$IN_DIR"
eplot    -x "$XTITLE" -y "$YTITLE" -r '[][1.5:2.5]' 1-wide.cycles --svg -o "$OUT_DIR/1-wide.svg"
eplot    -x "$XTITLE" -y "$YTITLE" -r '[][1.3:2.2]' 2-wide.cycles --svg -o "$OUT_DIR/2-wide.svg"
eplot -m -x "$XTITLE" -y "$YTITLE" -r '[][1.3:2.5]' 2-wide*.cycles linear*.cycles --svg -o "$OUT_DIR/2-wide-pfon.svg"
cd -
