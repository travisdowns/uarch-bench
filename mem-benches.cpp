/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benches.hpp"
#include "util.hpp"

#include <random>

#define LOAD_LOOP_UNROLL    8

#define BENCH_DECL_X(name)  \
    bench2_f name ## 16;    \
    bench2_f name ## 32;    \
    bench2_f name ## 64;    \
    bench2_f name ## 128;   \
    bench2_f name ## 256;   \
    bench2_f name ## 512;   \
    bench2_f name ## 2048;  \

#define FWD_BENCH_DECL(delay) \
        bench2_f fwd_lat_delay_ ## delay ; \
        bench2_f fwd_tput_conc_ ## delay ;

#define MAX_KIB         2048
#define MAX_SIZE        (MAX_KIB * 1024)

#define ALL_SIZES_X(func)  ALL_SIZES_X1(func,MAX_KIB)
// we need one level of indirection to expand MAX_KIB properly. See:
// https://stackoverflow.com/questions/50403741/using-a-macro-as-an-argument-in-an-x-macro-definition
#define ALL_SIZES_X1(func, max)  \
            func(  16)     \
            func(  24)     \
            func(  30)     \
            func(  31)     \
            func(  32)     \
            func(  33)     \
            func(  34)     \
            func(  35)     \
            func(  40)     \
            func(  48)     \
            func(  56)     \
            func(  64)     \
            func(  80)     \
            func(  96)     \
            func( 112)     \
            func( 128)     \
            func( 196)     \
            func( 252)     \
            func( 256)     \
            func( 260)     \
            func( 384)     \
            func( 512)     \
            func(1024)     \
            func(max)

#define SERIAL_DECL(size) bench2_f serial_load_bench ## size ;
//#define SERIAL_DECL1(size) bench2_f serial_load_bench ## size ;

extern "C" {
/* misc benches */
BENCH_DECL_X(load_loop)
BENCH_DECL_X(prefetcht0_bench)
BENCH_DECL_X(prefetcht1_bench)
BENCH_DECL_X(prefetcht2_bench)
BENCH_DECL_X(prefetchnta_bench)

bench2_f serial_load_bench;

FWD_BENCH_DECL(0);
FWD_BENCH_DECL(1);
FWD_BENCH_DECL(2);
FWD_BENCH_DECL(3);
FWD_BENCH_DECL(4);
FWD_BENCH_DECL(5);

bench2_f fwd_tput_conc_6;
bench2_f fwd_tput_conc_7;
bench2_f fwd_tput_conc_8;
bench2_f fwd_tput_conc_9;
bench2_f fwd_tput_conc_10;

}

template <typename TIMER>
void register_mem_oneshot(GroupList& list);

struct CacheLine {
    CacheLine *next;
    char padding[UB_CACHE_LINE_SIZE - sizeof(CacheLine*)];
};

// I don't think this going to fail on any platform in common use today, but who knows?
static_assert(sizeof(CacheLine) == UB_CACHE_LINE_SIZE, "really weird layout on this platform");

size_t count(CacheLine* first) {
    CacheLine* p = first;
    size_t count = 0;
    do {
        p = p->next;
        count++;
    } while (p != first);
    return count;
}

void *shuffled_region(size_t size) {
    assert(size <= MAX_SIZE);
    assert(size % UB_CACHE_LINE_SIZE == 0);
    size_t size_lines = size / UB_CACHE_LINE_SIZE;
    assert(size_lines > 0);

    // only get the storage once and keep re-using it, to minimize variance (e.g., some benchmarks getting huge pages
    // and others not, etc)
    static CacheLine* storage = (CacheLine*)new_huge_ptr(MAX_SIZE);

    std::vector<size_t> indexes(size_lines);
    std::iota(indexes.begin(), indexes.end(), 0);
    std::shuffle(indexes.begin(), indexes.end(), std::mt19937_64{123});

    CacheLine* p = storage + indexes[0];
    for (size_t i = 1; i < size_lines; i++) {
        p->next = storage + indexes[i];
        p = p->next;
    }
    p->next = storage + indexes[0];

    assert(count(storage) == size_lines);

    return storage;
}

template <typename TIMER, bench2_f FUNC>
static void make_load_bench(DeltaMaker<TIMER>& maker, size_t kib, const std::string &suffix) {
    std::string id = suffix + "-" + std::to_string(kib);
    std::string name = std::to_string(kib) +  "-KiB " + suffix;
    maker.template make<FUNC>(id, name, LOAD_LOOP_UNROLL, []{ return aligned_ptr(4096, 2048 * 1024); } );
}

#define MAKE_PARALLEL(kib) \
    make_load_bench<TIMER,load_loop ## kib>        (maker, kib,      "parallel-loads"); \
    make_load_bench<TIMER,prefetcht0_bench  ## kib>(maker, kib, "parallel-prefetcht0"); \
    make_load_bench<TIMER,prefetcht1_bench  ## kib>(maker, kib, "parallel-prefetcht1"); \
    make_load_bench<TIMER,prefetcht2_bench  ## kib>(maker, kib, "parallel-prefetcht2"); \
    make_load_bench<TIMER,prefetchnta_bench ## kib>(maker, kib, "parallel-prefetchnta");

#define MAKE_SERIAL(kib) \
        maker.template make<serial_load_bench>(                   \
        std::string("serial-loads-") + std::to_string(kib),       \
        std::to_string(kib) + std::string("-KiB serial loads") ,  \
        1,                                                        \
        []{ return shuffled_region(kib * 1024); });

template <typename TIMER>
void register_mem(GroupList& list) {
    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("memory/load-parallel", "Parallel load/prefetches from fixed-size regions");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 100000).setTags({"default"});

        MAKE_PARALLEL(16)
        MAKE_PARALLEL(32)
        MAKE_PARALLEL(64)
        MAKE_PARALLEL(128)
        MAKE_PARALLEL(256)
        MAKE_PARALLEL(512)
        MAKE_PARALLEL(2048)
    }

    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("memory/load-serial", "Serial loads from fixed-size regions");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 1024 * 1024).setTags({"default"});

        ALL_SIZES_X(MAKE_SERIAL)
    }

    {
        std::shared_ptr<BenchmarkGroup> fwd_group = std::make_shared<BenchmarkGroup>("memory/store-fwd", "Store forwaring latency and throughput");

        using default_maker = StaticMaker<TIMER>;

#define LAT_DELAY_BENCH(delay) \
        default_maker::template make_bench<fwd_lat_delay_ ## delay>(fwd_group.get(), "latency-" #delay, \
                    "Store forward latency delay " #delay, 1)

#define TPUT_BENCH(conc) \
        default_maker::template make_bench<fwd_tput_conc_ ## conc>(fwd_group.get(), "concurrency-" #conc, \
                    "Store fwd tput concurrency " #conc, conc)


        auto benches = std::vector<Benchmark> {
            LAT_DELAY_BENCH(0),
            LAT_DELAY_BENCH(1),
            LAT_DELAY_BENCH(2),
            LAT_DELAY_BENCH(3),
            LAT_DELAY_BENCH(4),
            LAT_DELAY_BENCH(5),

            TPUT_BENCH(1),
            TPUT_BENCH(2),
            TPUT_BENCH(3),
            TPUT_BENCH(4),
            TPUT_BENCH(5),
            TPUT_BENCH(6),
            TPUT_BENCH(7),
            TPUT_BENCH(8),
            TPUT_BENCH(9),
            TPUT_BENCH(10)

        };

        fwd_group->add(benches);
        list.push_back(fwd_group);
    }

    register_mem_oneshot<TIMER>(list);
}

#define REG_DEFAULT(CLOCK) template void register_mem<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



