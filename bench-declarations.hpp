/*
 * bench-declarations.hpp
 *
 * Basic definitions for benchmark methods types, used by assembly and C/C++ benchmark methods alike.
 */

#ifndef BENCH_DECLARATIONS_HPP_
#define BENCH_DECLARATIONS_HPP_

extern "C" {

typedef void (bench_f)(uint64_t);
typedef void (bench2_f)(uint64_t, void *);

}

#endif /* BENCH_DECLARATIONS_HPP_ */
