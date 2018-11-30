#! /usr/bin/env bash
set -e

# Recompiles and runs uarch-bench repeatedly, varying the value of a given nasm define (defaulting to NOPCOUNT)

# You can override any of these variables from outside to customize the behavior of this script.
: ${VAR:="NOPCOUNT"}
: ${TOUCH:="rm -f x86_methods2.o"}
: ${COMPILE:="make"} 
: ${TEST:="memory/store-fwd-try/stfwd-raw-mixed"}
: ${CMD:="./uarch-bench --timer=libpfc --test-name=$TEST --extra-events=MACHINE_CLEARS.MEMORY_ORDERING"}
: ${START:=1}
: ${STOP:=1000}

for nop in $(seq $START $STOP); do
$TOUCH
$COMPILE "NASM_DEFINES=-D${VAR}=${nop}"
$CMD
#	$CMD
done