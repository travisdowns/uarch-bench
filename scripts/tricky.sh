
TEST=${TEST-bandwidth-tricky-128}
MAX=${MAX-50}
OUT_FILE=${1-tricky.out}
echo "Using output file $OUT_FILE for test $TEST, up to UNROLL of $MAX"

FTEST=memory/bandwidth/$TEST

for i in $(seq 1 $MAX); do
    rm -f x86_methods2.o && echo "UNROLL $i out of $MAX" && NASM_DEFINES="-DUNROLLB=$i $NASM_MORE" make && ./uarch-bench.sh --test-name=$FTEST
done | tee "${OUT_FILE}.tmp" | grep 'UNROLL '

grep ' bandwidth' "${OUT_FILE}.tmp" | ec 4 > ${OUT_FILE}
rm "${OUT_FILE}.tmp"
echo "Wrote result to ${OUT_FILE}"