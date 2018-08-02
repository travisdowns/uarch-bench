/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benches.hpp"
#include "util.hpp"

#include <random>

#define LOAD_LOOP_UNROLL    8

#define PFTYPE_X(f,arg) \
    f( prefetcht0, arg) \
    f( prefetcht1, arg) \
    f( prefetcht2, arg) \
    f(prefetchnta, arg) \

#define LOADTYPE_X(f,arg)  \
    f(       load, arg)    \
    PFTYPE_X(f,arg)        \

#define FWD_BENCH_DECL(delay) \
        bench2_f fwd_lat_delay_ ## delay ; \
        bench2_f fwd_tput_conc_ ## delay ;

#define MAX_KIB         2048
#define MAX_SIZE        (400 * 1024 * 1024)

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

#define ALL_SIZES_ARRAY { ALL_SIZES_X(APPEND_COMMA) }

#define SERIAL_DECL(size) bench2_f serial_load_bench ## size ;
//#define SERIAL_DECL1(size) bench2_f serial_load_bench ## size ;

extern "C" {
/* misc benches */

bench2_f   serial_load_bench;

#define PARALLEL_MEM_DECL(loadtype,arg) bench2_f parallel_mem_bench_ ## loadtype;
LOADTYPE_X(PARALLEL_MEM_DECL,dummy);
bench2_f parallel_load_bench;

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

struct region {
    size_t size;
    void *start;
};

/**
 * Return a region of memory of size bytes, where each cache line sized chunk points to another random chunk
 * within the region. The pointers cover all chunks in a cycle of maximum size.
 *
 * The region_struct is returned by reference and points to a static variable that is overwritten every time
 * this function is called.
 */
region& shuffled_region(const size_t size) {
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
        p = p->next = storage + indexes[i];
    }
    p->next = storage + indexes[0];

    assert(count(storage) == size_lines);

    return *(new region{ size, storage }); // leak
}

template <bench2_f F, typename M>
static void make_load_bench2(M& maker, int kib, const char* id_prefix, const char *desc_suffix, uint32_t ops) {
    maker.template make<F>(
            string_format("%s-%d", id_prefix, kib),
            string_format("%d-KiB %s", kib, desc_suffix),
            ops,
            [kib]{ return &shuffled_region(kib * 1024); }
    );
}

#define MAKE_SERIAL(kib)  make_load_bench2<  serial_load_bench>   (maker, kib, "serial-loads",   "serial loads", 1);
#define MAKEP_LOAD(l,kib) make_load_bench2<parallel_mem_bench_##l>(maker, kib, "parallel-" #l, "parallel " #l, LOAD_LOOP_UNROLL);
#define MAKEP_ALL(kib) LOADTYPE_X(MAKEP_LOAD,kib)

template <typename TIMER>
void register_mem(GroupList& list) {
    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("memory/load-parallel", "Parallel loads from fixed-size regions");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 100000).setTags({"default"});

        for (auto kib : ALL_SIZES_ARRAY) {
            MAKEP_LOAD(load, kib);
        }

        for (int kib = MAX_KIB * 2; kib <= MAX_SIZE / 1024; kib *= 2) {
            MAKEP_LOAD(load, kib);
        }
    }

    {
        // this group of tests is flawed for prefetch1 and prefetch2 and probably prefetchnta in that is highly dependent on the initial cache state.
        // If the accessed region is in L1 at the start of the test, loads like prefetch1 which would normally leave the only line in L2,
        // will find it in L1 and be much faster. If the line isn't in L1, it won't get in there and the test will be slower. Each line can be in
        // either state, so you'll get a range of results somewhere between slow and fast, depending on random factors preceeding the test.
        // To fix, we could put the region in a consistent state.
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("memory/prefetch-parallel", "Parallel prefetches from fixed-size regions");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 100000).setTags({"default"});

        for (auto kib : {16, 32, 64, 128, 256, 512, 2048}) {
            PFTYPE_X(MAKEP_LOAD,kib)
        }
    }


    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("memory/load-serial", "Serial loads from fixed-size regions");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 1024 * 1024).setTags({"default"});

        ALL_SIZES_X(MAKE_SERIAL)

        for (int kib = MAX_KIB * 2; kib <= MAX_SIZE / 1024; kib *= 2) {
            MAKE_SERIAL(kib);
        }
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



