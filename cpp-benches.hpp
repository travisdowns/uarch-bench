/*
 * cpp-benches.hpp
 */

#ifndef CPP_BENCHES_HPP_
#define CPP_BENCHES_HPP_

#include "bench-declarations.h"

#include <stdlib.h>

// division benches

#define IDENTITY(x, ...) x
#define RCONCAT(x, y, ...) y ## x

#define DIV_SPEC_X3(g, e) g(e)

#define DIV_SPEC_X2(f, g, arg)     \
    DIV_SPEC_X3(f, g( 32_64, arg)) \
    DIV_SPEC_X3(f, g( 64_64, arg)) \
    DIV_SPEC_X3(f, g(128_64, arg)) \


// call f with 32_64, 64_64, etc - each division spec
#define DIV_SPEC_X(f) DIV_SPEC_X2(f, IDENTITY, )


// call f with each function name, the product of the 4
#define DIV_BENCH_X(f)                             \
        DIV_SPEC_X2(f, RCONCAT, div_lat_inline)    \
        DIV_SPEC_X2(f, RCONCAT, div_lat_noinline)  \
        DIV_SPEC_X2(f, RCONCAT, div_tput_inline)   \
        DIV_SPEC_X2(f, RCONCAT, div_tput_noinline) \


#define DECL_BENCH(name) bench2_f name;

DIV_BENCH_X(DECL_BENCH)

bench2_f linkedlist_sentinel;
bench2_f linkedlist_counter;
bench2_f shuffled_list_sum;
bench2_f gettimeofday_bench;

constexpr int LIST_COUNT = 4000;

struct mem_args {
    char *region;
    uint64_t stride;
    uint64_t mask;
};

bench2_f strided_stores_1byte;
bench2_f strided_stores_4byte;
bench2_f strided_stores_8byte;

bench2_f volatile_stores_study;

#define VS_GAP_LIST_X(fn, bits, bitstr) \
    fn( 0, elems, bits, bitstr)  \
    fn( 1, elems, bits, bitstr)  \
    fn( 2, elems, bits, bitstr)  \
    fn(56, bytes, bits, bitstr)  \
    fn(64, bytes, bits, bitstr)

#define VS_GAP_GAP_X(fn)   \
    VS_GAP_LIST_X(fn, 8,  "8-bit") \
    VS_GAP_LIST_X(fn, 32, "32-bit") \
    VS_GAP_LIST_X(fn, 64, "64-bit")

#define GAP_FN(gap, gaptype, bits) vs_ ## bits ## b_gap_ ## gap ## _ ## gaptype
#define DECLARE_GAP_FNS(gap, gaptype, bits, ...) bench2_f GAP_FN(gap, gaptype, bits);
VS_GAP_GAP_X(DECLARE_GAP_FNS)

// arbitrary offset store study
#define ARB_OFFSET_X(fn) \
    fn(uint64_t, 0_0_0_0,    0, 0, 0, 0) \
    fn(uint64_t, 0_1_0_1,    0, 1, 0, 1) \
    fn(uint64_t, 0_2_4_6,    0, 2, 4, 6) \
    fn(uint64_t, 0_xxx,      0, 0, 0, 1) \
    fn(uint32_t, 0_1_0_1,    0, 1, 0, 1) \
    fn(uint32_t, 0_2_4_6,    0, 2, 4, 6) \
    fn(uint32_t, 0_16_0_16,  0, 16,  0, 16) \
    fn(uint32_t, 0_16_16_16, 0, 16, 16, 16) \
    fn(uint32_t, 0_16_17_18, 0, 16, 17, 18) \

#define DECLARE_ARB_FNS(type, name, ...) bench2_f arb_offset_##type##_##name;
ARB_OFFSET_X(DECLARE_ARB_FNS)

bench2_f misaligned_stores_sameloc;
bench2_f misaligned_stores_rolling;
bench2_f misaligned_stores_twoloc;

bench2_f crc8_bench;
bench2_f sum_halves_bench;
bench2_f mul_by_bench;
bench2_f mul_chain_bench;
bench2_f mul_chain4_bench;
bench2_f add_indirect;
bench2_f add_indirect_shift;

#define TRANSCENDENTAL_X(f) \
    f(log)      \
    f(exp)      \
    f(pow)      \

#define DECLARE_TRAN(name, ...) bench2_f transcendental_##name; bench2_f transcendental_lat_##name;
TRANSCENDENTAL_X(DECLARE_TRAN)


void* getLinkedList();

#endif /* CPP_BENCHES_HPP_ */
