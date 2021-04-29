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
#include "boost/preprocessor/repetition/repeat.hpp"


#include <cassert>
#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <limits>
#include <cmath>
#include <random>
#include <vector>

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

template <typename T>
long strided_stores(uint64_t iters, void *arg) {
    mem_args args = *(mem_args *)arg;
    char* region = args.region;
    size_t mask = args.mask;
    for (uint64_t i = 0; i < iters; i += 4) {
        uint64_t offset = i * args.stride & mask;
        char *base = region + offset;
        *(T *)base = 0;
        base += args.stride;
        *(T *)base = 0;
        base += args.stride;
        *(T *)base = 0;
        base += args.stride;
        *(T *)base = 0;
    }
    sink_ptr(args.region);
    return (long)args.region[0];
}

long strided_stores_1byte(uint64_t iters, void* arg) {
    return strided_stores<uint8_t>(iters, arg);
}

long strided_stores_4byte(uint64_t iters, void *arg) {
    return strided_stores<uint32_t>(iters, arg);
}

long strided_stores_8byte(uint64_t iters, void *arg) {
    return strided_stores<uint64_t>(iters, arg);
}


template <typename T, size_t S, size_t A>
struct aligned_buf {
    static constexpr size_t BUF_SIZE = sizeof(T) * S + A;
    unsigned char buf[BUF_SIZE];
    
    T* get() {
        void *p = buf;
        auto sz = BUF_SIZE;
        auto ret = std::align(A, S, p, sz);
        assert(ret);
        return (T*)ret;
    }
};

#define VS_STUDY_UNROLL 10

/**
 * Specific study for 64-bit writes on Graviton 2
 */
long volatile_stores_study(uint64_t iters, void* arg) {
    // constexpr size_t gap = 1;
    using type = uint64_t;

    aligned_buf<type, 1024 * 128, 64> buf;
    volatile type * vptr = buf.get();

    for (uint64_t i = 0; i < iters; i += 4 * VS_STUDY_UNROLL) {
        #define BODY(z, n, data) \
                vptr[0] = 1;  \
                vptr[8] = 1;  \
                vptr[0] = 1;  \
                vptr[8] = 1;  

        BOOST_PP_REPEAT(VS_STUDY_UNROLL, BODY, _);
        #undef BODY
    }

    return 0; 
}

template <typename T>
HEDLEY_ALWAYS_INLINE
void write_at_offsets(volatile T *p) {
}

template <typename T, size_t O, size_t... Os>
HEDLEY_ALWAYS_INLINE
void write_at_offsets(volatile T *p) {
    p[O] = 1;
    write_at_offsets<T, Os...>(p);
}

/**
 * Specific study for 64-bit writes on Graviton 2
 */
template <typename type, size_t... Os>
HEDLEY_ALWAYS_INLINE
long volatile_stores_arb_offsets(uint64_t iters, void* arg) {

    aligned_buf<type, 1024 * 16, 64> buf;
    volatile type * vptr = buf.get();

    for (uint64_t i = 0; i < iters; i += 2 * sizeof...(Os)) {
        write_at_offsets<type, Os...>(vptr);
        write_at_offsets<type, Os...>(vptr);
    }

    return 0; 
}

#define DEFINE_ARB_OFFSET(type, name, ...) \
        long arb_offset_##type##_##name(uint64_t iters, void* arg) { \
            return volatile_stores_arb_offsets<type, __VA_ARGS__>(iters, arg); \
        }

ARB_OFFSET_X(DEFINE_ARB_OFFSET)

/**
 * When writing portable benchmarks, we don't get to specify exactly the assembly
 * we want. Instead, we have to rely on various tricks to produce code that might
 * otherwise be unnatural to the compiler.
 * 
 * Here, we want a series of stores with as few additional instructions in the loop
 * as possible. Redundant stores will be happily eliminated by the compiler, and 
 * not-provably redundant stores often require some addressing which might add 
 * overhead. So we use volatile stores instead.
 */
template <size_t dist, typename type>
HEDLEY_ALWAYS_INLINE
long volatile_stores_bytes(uint64_t iters, void* arg) {
    constexpr size_t UNROLL = 8;
    static_assert(dist % sizeof(type) == 0, "must be able to convert dist (bytes) to an element gap");
    constexpr auto gap = dist / sizeof(type); 

    aligned_buf<type, 1 + UNROLL * dist, 64> buf;
    volatile type * vptr = buf.get();

    for (uint64_t i = 0; i < iters; i += UNROLL) {
        for (size_t i = 0; i < UNROLL / 2; i++) {
            vptr[0 * gap] = 1;
            vptr[1 * gap] = 2;
        }
    }

    return 0; 
}

/**
 * Allows you to specify dist in type-sized elements instead of bytes.
 */
template <size_t dist, typename type>
HEDLEY_ALWAYS_INLINE
long volatile_stores_elems(uint64_t iters, void* arg) {
    return volatile_stores_bytes<dist * sizeof(type), type>(iters, arg);
}

#define DEFINE_GAP(gap, gaptype, bits, ...) long GAP_FN(gap, gaptype, bits)(uint64_t iters, void* arg) \
        { return volatile_stores_ ## gaptype<gap, uint ## bits ## _t>(iters, arg); }

VS_GAP_GAP_X(DEFINE_GAP)

//////////////////////////////
// Portable alignment tests //
//////////////////////////////

/**
 * Repeatedly do a store to the same location at the given alignment
 * within a 64-bit cache line.
 */
template <size_t roll> 
HEDLEY_ALWAYS_INLINE
long aligned_stores_helper(uint64_t iters, void* arg) {
    using type = uint64_t;

    aligned_buf<type, 1024, 128> buf;
    volatile type * vptr = (type *)((char *)buf.get() + (size_t)arg);

#define ALIGNED_STORES_UNROLL 16

    for (uint64_t i = 0; i < iters; i += ALIGNED_STORES_UNROLL) {
        #define BODY(z, n, data) vptr[n * roll] = 1;
        BOOST_PP_REPEAT(ALIGNED_STORES_UNROLL, BODY, _);
        #undef BODY
    }

    return 0; 
}

long misaligned_stores_sameloc(uint64_t iters, void* arg) {
    return aligned_stores_helper<0>(iters, arg);
}

long misaligned_stores_rolling(uint64_t iters, void* arg) {
    return aligned_stores_helper<1>(iters, arg);
}

/*
 * Alternating stores to two misaligned locations: offset and
 * 64 - offset.
 * 
 * Tests whether non-consecutive stores can coalesce (while 
 * aligned_stores_sameloc tests if consecutive stores can).
 */
long misaligned_stores_twoloc(uint64_t iters, void* arg) {
    using type = uint64_t;

    aligned_buf<type, 1024, 128> buf;
    auto offset = (size_t)arg;
    volatile type * vptr0 = (type *)((char *)buf.get() + offset);
    volatile type * vptr1 = (type *)((char *)buf.get() + 61 - offset);

#define TWOLOC_STORES_UNROLL 8

    for (uint64_t i = 0; i < iters; i += TWOLOC_STORES_UNROLL * 2) {
        #define BODY(z, n, data) \
                *vptr0 = 1; \
                *vptr1 = 1;
        BOOST_PP_REPEAT(TWOLOC_STORES_UNROLL, BODY, _);
        #undef BODY
    }

    return 0; 
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

// off course the table is not supposed to be full of zeros but for our purposes this is fine
uint8_t crc8_table[256] = {};

// from https://stackoverflow.com/a/15171925
uint32_t crc8(uint32_t crc, uint8_t *data, size_t len)
{
    crc &= 0xff;
    unsigned char const *end = data + len;
    while (data < end)
        crc = crc8_table[crc ^ *data++];
    return crc;
}


long crc8_bench(uint64_t iters, void *arg) {
    uint8_t buf[4096];
    opt_control::sink_ptr(buf);
    uint32_t crc = 0;
    do {
        crc = crc8(crc, buf, sizeof(buf));
    } while (--iters != 0);
    return crc;
}

struct top_bottom {
    uint32_t top, bottom;
};

// HEDLEY_NEVER_INLINE
top_bottom sum_halves(const uint32_t *data, size_t len) {
    uint32_t top = 0, bottom = 0;
    for (size_t i = 0; i < len; i += 2) {
        uint32_t elem;

        elem = data[i];
        top    += elem >> 16;
        bottom += elem & 0xFFFF;

        elem = data[i+1];
        top    += elem >> 16;
        bottom += elem & 0xFFFF;
    }
    return {top, bottom};
}

long sum_halves_bench(uint64_t iters, void *arg) {
    uint32_t buf[4096];
    opt_control::sink_ptr(buf);
    do {
        auto ret = sum_halves(buf, sizeof(buf) / sizeof(buf[0]));
        opt_control::sink(ret.top + ret.bottom);
    } while (--iters != 0);
    return 0;
}

HEDLEY_NEVER_INLINE
uint32_t mul_by(const uint32_t *data, size_t len, uint32_t m) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len - 1; i++) {
        uint32_t x = data[i], y = data[i + 1];
        sum += x * y * m * i * i;
    }
    opt_control::sink(sum);
    return sum;
}

HEDLEY_NEVER_INLINE
uint32_t mul_chain(const uint32_t *data, size_t len, uint32_t m) {
    uint32_t product = 1;
    for (size_t i = 0; i < len; i++) {
        uint32_t x = data[i];
        product *= x;
    }
    opt_control::sink(product);
    return product;
}

HEDLEY_NEVER_INLINE
uint32_t mul_chain4(const uint32_t *data, size_t len, uint32_t m) {
    uint32_t p1 = 1, p2 = 1, p3 = 1, p4 = 1;
    for (size_t i = 0; i < len; i += 4) {
        p1 *= data[i + 0];
        p2 *= data[i + 1];
        p3 *= data[i + 2];
        p4 *= data[i + 3];
    }
    uint32_t product = p1 * p2 * p3 * p4;
    opt_control::sink(product);
    return product;
}

template <typename F>
long mul_by_bench_f(uint64_t iters, void *arg, F f) {
    uint32_t buf[4096];
    opt_control::sink_ptr(buf);
    uint32_t x = 123;
    opt_control::modify(x);
    do {
        opt_control::sink(f(buf, sizeof(buf) / sizeof(buf[0]), x));
    } while (--iters != 0);
    return 0;
}

long mul_by_bench(uint64_t iters, void *arg) {
    return mul_by_bench_f(iters, arg, mul_by);
}

long mul_chain_bench(uint64_t iters, void *arg) {
    return mul_by_bench_f(iters, arg, mul_chain);
}

long mul_chain4_bench(uint64_t iters, void *arg) {
    return mul_by_bench_f(iters, arg, mul_chain4);
}

HEDLEY_NEVER_INLINE
uint32_t add_indirect_inner(const uint32_t *data, const uint32_t *offsets, size_t len)
{
    assert(len >= 2 && len % 2 == 0);
    uint32_t sum1 = 0, sum2 = 0;
    size_t i = len;
    do {
        sum1 += data[offsets[i - 1]];
        sum2 += data[offsets[i - 2]];
        i -= 2;
    } while (i);
    opt_control::sink(sum1 + sum2);
    return sum1 + sum2;
}

HEDLEY_NEVER_INLINE
uint32_t add_indirect_shift_inner(const uint32_t *data, const uint32_t *offsets, size_t len) {
    uint32_t sum1 = 0, sum2 = 0;
    size_t i = len;
    do {
        uint64_t twooffsets;
        std::memcpy(&twooffsets, offsets + i - 2, sizeof(uint64_t));
        sum1 += data[twooffsets >> 32];
        sum2 += data[twooffsets & 0xFFFFFFFF];
        i -= 2;
    } while (i);
    opt_control::sink(sum1 + sum2);
    return sum1 + sum2;
}


template <typename F>
long add_indirect_f(uint64_t iters, void *arg, F f) {
    uint32_t buf[4096], offsets[4096] = {};
    opt_control::sink_ptr(buf);
    opt_control::sink_ptr(offsets);
    uint32_t x = 123;
    opt_control::modify(x);
    do {
        opt_control::sink(f(buf, offsets, sizeof(buf) / sizeof(buf[0])));
    } while (--iters != 0);
    return 0;
}

long add_indirect(uint64_t iters, void *arg) {
    return add_indirect_f(iters, arg, add_indirect_inner);
}

long add_indirect_shift(uint64_t iters, void *arg) {
    return add_indirect_f(iters, arg, add_indirect_shift_inner);
}


HEDLEY_ALWAYS_INLINE double log_helper(double x, double y) { return std::log(x); }
HEDLEY_ALWAYS_INLINE double exp_helper(double x, double y) { return std::exp(x); }
HEDLEY_ALWAYS_INLINE double pow_helper(double x, double y) { return std::pow(x, y); }

template <typename F>
long do_transcendental(uint64_t iters, void *arg, F helper) {
    double x0 = 0.123;
    double y0 = 0.456;
    while (iters--) {
        double x = x0, y = y0; 
        opt_control::modify(x);
        // opt_control::modify(y);
        double r = helper(x, y);
        // double r = std::log(x);
        opt_control::sink(r);
    }
    return 0;
}

template <typename F>
long do_transcendental_lat(uint64_t iters, void *arg, F helper) {
    double x = 0.123;
    double y = 0.456;
    double z = 0.;
    opt_control::modify(z);
    while (iters--) {
        double t = helper(x, y);
        x += t * z; // no change to x, but makes x dependent on t, completing the chain
        opt_control::sink(x);
    }
    return 0;
}


#define MAKE_TRAN(name)                           \
long transcendental_##name(uint64_t iters, void *arg) {   \
    return do_transcendental(iters, arg, name##_helper);  \
} \
long transcendental_lat_##name(uint64_t iters, void *arg) {   \
    return do_transcendental_lat(iters, arg, name##_helper);  \
}

TRANSCENDENTAL_X(MAKE_TRAN)







