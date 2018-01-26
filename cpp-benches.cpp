/*
 * cpp-benches.cpp
 *
 * Benchmarks written in C++.
 *
 */

#include "cpp-benches.hpp"

#include <limits>
#include <cinttypes>

using std::size_t;
using std::uint64_t;

typedef uint64_t (div_func)(uint64_t);

static inline uint64_t somefunction_inline(uint64_t a) {
    return 10000 / a;
}

__attribute__ ((noinline))
uint64_t somefunction_noinline(uint64_t a) {
    return somefunction_inline(a);
}

template <div_func F>
long div64_lat_templ(uint64_t iters, void *arg) {
    uint64_t z = (uintptr_t)arg;
    for(uint64_t k = 0; k < iters; k++)
      z = F(z);
    return (long)z;
}

template <div_func F>
long div64_tput_templ(uint64_t iters, void *arg) {
    uint64_t z = 0;
    for(uint64_t k = 0; k < iters; k++)
      z += F(k + 1);
    return (long)z;
}

long div64_lat_inline(uint64_t iters, void *arg) {
    return div64_lat_templ<somefunction_inline>(iters, arg);
}

long div64_tput_inline(uint64_t iters, void *arg) {
    return div64_tput_templ<somefunction_inline>(iters, arg);
}

long div64_lat_noinline(uint64_t iters, void *arg) {
    return div64_lat_templ<somefunction_noinline>(iters, arg);
}

long div64_tput_noinline(uint64_t iters, void *arg) {
    return div64_tput_templ<somefunction_noinline>(iters, arg);
}








