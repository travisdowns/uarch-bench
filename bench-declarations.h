/*
 * bench-declarations.hpp
 *
 * Basic definitions for benchmark methods types, used by assembly and C/C++ benchmark methods alike.
 */

#ifndef BENCH_DECLARATIONS_H_
#define BENCH_DECLARATIONS_H_

#include <stdint.h>

extern "C" {

typedef void (bench2_f)(uint64_t, void *);

}

#endif /* BENCH_DECLARATIONS_H_ */
