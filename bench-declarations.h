/*
 * bench-declarations.hpp
 *
 * Basic definitions for benchmark methods types, used by assembly and C/C++ benchmark methods alike.
 */

#ifndef BENCH_DECLARATIONS_H_
#define BENCH_DECLARATIONS_H_

#include <stdint.h>

extern "C" {

/**
 * The prototype for a benchmark function.
 *
 * iters: the number of times the benchmark shoudl execute using its internal loop
 * arg: the benchmark specific arg, for benchmarks that need an argument (e.g,. a memory region to read from)
 *
 * return value: ignored by the benchmarking code, but useful for C/C++ benchmarks to return some value that
 * depends on the substance of the benchmark so that the method isn't optimized away.
 */
typedef long (bench2_f)(uint64_t iters, void *arg);

}

#endif /* BENCH_DECLARATIONS_H_ */
