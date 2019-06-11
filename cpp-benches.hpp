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

bench2_f crc8_bench;
bench2_f sum_halves_bench;
bench2_f mul_by_bench;
bench2_f mul_chain_bench;
bench2_f mul_chain4_bench;
bench2_f add_indirect;
bench2_f add_indirect_shift;

void* getLinkedList();

#endif /* CPP_BENCHES_HPP_ */
