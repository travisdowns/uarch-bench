/*
 * cpp-benches.cpp
 *
 * Benchmarks written in C++.
 *
 */

#include "cpp-benches.hpp"
#include "hedley.h"
#include "opt-control.hpp"
#include "util.hpp"

#include <limits>
#include <cinttypes>
#include <vector>
#include <random>
#include <cstddef>

#include <sys/time.h>


using std::size_t;
using std::uint64_t;

typedef uint64_t (div_func)(uint64_t);

template <div_func F>
HEDLEY_NEVER_INLINE
uint64_t no_inline(uint64_t a) {
    return F(a);
}

static inline uint64_t div32_64(uint64_t a) {
    return 0x12345678u / a;
}

static inline uint64_t div64_64(uint64_t a) {
    return 0x1234567812345678ull / a;
}

static inline uint64_t div128_64(uint64_t a) {
#if !UARCH_BENCH_PORTABLE
    uint64_t high = 123, low = 2;
    a |= 0xF234567890123456ull;
    asm ("div %2" : "+d"(high), "+a"(low) : "r"(a) : );
    return low;
#else
    return 1;
#endif
}

template <div_func F, bool forcedep>
long div64_templ(uint64_t iters, void *arg) {
    uint64_t sum = 0, zero = always_zero();
    for (uint64_t k = 1; k <= iters; k++) {
        uint64_t d = k;
        if (forcedep) {
            d += (sum & zero);
        }
        sum += F(d);
    }
    return (long)sum;
}

#define MAKE_DIV_BENCHES(suffix)                                            \
        long div_lat_inline ## suffix (uint64_t iters, void *arg) {         \
            return div64_templ<div ## suffix, true>(iters, arg);              \
        }                                                                   \
                                                                            \
        long div_tput_inline ## suffix(uint64_t iters, void *arg) {         \
            return div64_templ<div ## suffix, false>(iters, arg);             \
        }                                                                   \
                                                                            \
        long div_lat_noinline ## suffix(uint64_t iters, void *arg) {        \
            return div64_templ<no_inline<div ## suffix>, true>(iters, arg);   \
        }                                                                   \
                                                                            \
        long div_tput_noinline ## suffix(uint64_t iters, void *arg) {       \
            return div64_templ<no_inline<div ## suffix>, false>(iters, arg);  \
        }                                                                   \


DIV_SPEC_X(MAKE_DIV_BENCHES)


struct list_node {
    int value;
    list_node* next;
};

static_assert(offsetof(list_node, next) == 8, "double_load tests expect next to be a multiple of 8 offset");

struct list_head {
    int size;
    list_node *first;
};

list_head makeList(int size) {
    list_head head = { size, nullptr };
    if (size != 0) {
        list_node* all_nodes = new list_node[size]();
        head.first = new list_node{ 1, nullptr };
        list_node* cur = head.first;
        while (--size > 0) {
            list_node* next = all_nodes++;
            cur->next = next;
            cur = next;
        }
    }
    return head;
}

constexpr int NODE_COUNT = 5;

std::vector<list_head> makeLists() {
    std::mt19937_64 engine;
    std::uniform_int_distribution<int> dist(0, NODE_COUNT * 2);
    std::vector<list_head> lists;
    for (int i = 0; i < LIST_COUNT; i++) {
        lists.push_back(makeList(NODE_COUNT));
    }
    return lists;
}

std::vector<list_head> listOfLists = makeLists();

typedef long (list_sum)(list_head head);

template <list_sum SUM_IMPL>
long linkedlist_sum(uint64_t iters) {
    int sum = 0;
    while (iters-- > 0) {
        for (size_t list_index = 0; list_index < LIST_COUNT; list_index++) {
            sum += SUM_IMPL(listOfLists[list_index]);
        }
    }
    return sum;
}

long sum_counter(list_head list) {
    int sum = 0;
    list_node* cur = list.first;
    for (int i = 0; i < list.size; cur = cur->next, i++) {
        sum += cur->value;
    }
    return sum;
}

long sum_sentinel(list_head list) {
    int sum = 0;
    for (list_node* cur = list.first; cur; cur = cur->next) {
        sum += cur->value;
    }
    return sum;
}

long linkedlist_counter(uint64_t iters, void *arg) {
    return linkedlist_sum<sum_counter>(iters);
}

long linkedlist_sentinel(uint64_t iters, void *arg) {
    return linkedlist_sum<sum_sentinel>(iters);
}

long sumlist(list_node *first) {
    long sum = 0;
    list_node *p = first;
    do {
        sum += p->value;
        p = p->next;
    } while (p != first);
    return sum;
}

long shuffled_list_sum(uint64_t iters, void *arg) {
    int sum = 0;
    region* r = (region*)arg;
    while (iters-- > 0) {
        sum += sumlist((list_node*)r->start);
    }
    return sum;
}

long gettimeofday_bench(uint64_t iters, void *arg) {
    struct timeval tv;
    for (uint64_t i = 0; i < iters; i++) {
        gettimeofday(&tv, nullptr);
    }
    return (long)tv.tv_usec;
}

static inline void sink_ptr(void *p) {
    __asm__ volatile ("" :: "r"(p) : "memory");
}

long strided_stores(uint64_t iters, void *arg) {
    mem_args args = *(mem_args *)arg;
    for (uint64_t i = 0; i < iters; i += 4) {
        uint64_t offset = i * args.stride & args.mask;
        args.region[offset + args.stride * 0] = 0;
        args.region[offset + args.stride * 1] = 0;
        args.region[offset + args.stride * 2] = 0;
        args.region[offset + args.stride * 3] = 0;
    }
    sink_ptr(args.region);
    return (long)args.region[0];
}

long portable_add_chain(uint64_t itersu, void *arg) {
    using opt_control::modify;

    int64_t iters = itersu;
    // we use the modify call to force the compiler to emit the separate
    // decrements, otherwise it will simply combine consecutive subtractions
    do {
        modify(iters);
        --iters;
        modify(iters);
        --iters;
        // it is key that the last decrement before the check doesn't have a modify call
        // after since this lets the compiler use the result of the flags set by the last
        // decrement in the check (which will be fused)
    } while (iters != 0);

    return iters;
}







