/*
 * mem-benches.cpp
 *
 * Testing various memory and prefetching patterns for latency and throughput.
 */

#include "cpp-benches.hpp"
#include "util.hpp"

#include <random>
#include "benchmark.hpp"

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

static_assert(MAX_SIZE <= MAX_SHUFFLED_REGION_SIZE, "MAX_SHUFFLED_REGION_SIZE too small");

#define ALL_SIZES_X(func)  ALL_SIZES_X1(func,dummy,MAX_KIB)
#define ALL_SIZES_X_ARG(func,arg)  ALL_SIZES_X1(func,arg,MAX_KIB)

// we need one level of indirection to expand MAX_KIB properly. See:
// https://stackoverflow.com/questions/50403741/using-a-macro-as-an-argument-in-an-x-macro-definition
#define ALL_SIZES_X1(func, arg, max)  \
            func(  16, arg)     \
            func(  24, arg)     \
            func(  30, arg)     \
            func(  31, arg)     \
            func(  32, arg)     \
            func(  33, arg)     \
            func(  34, arg)     \
            func(  35, arg)     \
            func(  40, arg)     \
            func(  48, arg)     \
            func(  56, arg)     \
            func(  64, arg)     \
            func(  80, arg)     \
            func(  96, arg)     \
            func( 112, arg)     \
            func( 128, arg)     \
            func( 196, arg)     \
            func( 252, arg)     \
            func( 256, arg)     \
            func( 260, arg)     \
            func( 384, arg)     \
            func( 512, arg)     \
            func(1024, arg)     \
            func(max, arg)

#define APPEND_COMMA2(x,dummy) x,

#define ALL_SIZES_ARRAY { ALL_SIZES_X(APPEND_COMMA2) }

#define SERIAL_DECL(size) bench2_f serial_load_bench ## size ;
//#define SERIAL_DECL1(size) bench2_f serial_load_bench ## size ;

extern "C" {
/* misc benches */

bench2_f   serial_load_bench;
bench2_f   serial_load_bench2;

bench2_f serial_double_load1;
bench2_f serial_double_load2;
bench2_f serial_double_loadpf_same;
bench2_f serial_double_loadpf_diff;
bench2_f serial_double_loadpft1_diff;

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

bench2_f bandwidth_test256;
bench2_f bandwidth_test256i;
bench2_f bandwidth_test256i_orig;
bench2_f bandwidth_test256i_double;

}

template <typename TIMER>
void register_mem_oneshot(GroupList& list);

template <bench2_f F, typename M>
static void make_load_bench(M& maker, int kib, const char* id_prefix, const char *desc_suffix, uint32_t ops) {
    maker.template make<F>(
            string_format("%s-%d", id_prefix, kib),
            string_format("%d-KiB %s", kib, desc_suffix),
            ops,
            [kib]{ return &shuffled_region(kib * 1024); }
    );
}

#define MAKE_SERIAL(kib,test)  make_load_bench<test>             (maker, kib, "serial-loads",   "serial loads", 1);
#define MAKEP_LOAD(l,kib) make_load_bench<parallel_mem_bench_##l>(maker, kib, "parallel-" #l, "parallel " #l, LOAD_LOOP_UNROLL);
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

        for (auto kib : {16, 32, 64, 128, 256, 512, 2048, 4096, 8192, 8192 * 4}) {
            PFTYPE_X(MAKEP_LOAD,kib)
        }
    }


    {
        // this group of tests isn't directly comparable to the parallel tests since the access pattern is "more random" than the
        // parallel test, which is strided albeit with a large stride. In particular it's probably worse for the TLB. The result is
        // that the implied "max MLP" derived by dividing the serial access time by the parallel one is larger than 10 (about 12.5),
        // which I think is impossible on current Intel. We should make comparable parallel/serial tests that have identical access
        // patterns.
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("memory/load-serial", "Serial loads from fixed-size regions");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 1000 * 1000).setTags({"default"});

        ALL_SIZES_X_ARG(MAKE_SERIAL,serial_load_bench)

        maker = maker.setLoopCount(100 * 1000); // speed things up for the bigger tests
        for (int kib = MAX_KIB * 2; kib <= MAX_SIZE / 1024; kib *= 2) {
            MAKE_SERIAL(kib,serial_load_bench);
        }
    }

    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("memory/super-load-serial", "Finer-grained serial loads from fixed-size regions");
        list.push_back(group);
        // loop_count needs to be large enough to touch all the elements!
        auto maker = DeltaMaker<TIMER>(group.get(), 5 * 1000 * 1000).setTags({"slow"});

        for (int kib = 16; kib <= MAX_SIZE / 1024; kib *= 2) {
            size_t last = 0;
            for (double fudge = 0.90; fudge <= 1.10; fudge += 0.02) {
                size_t fudgedkib = fudge * kib;
                if (fudgedkib != last) {  // avoid duplicate tests for small kib values
                    MAKE_SERIAL(fudge * kib,serial_load_bench);
                }
                last = fudgedkib;
            }
        }
    }

    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("memory/bandwidth", "Linear AVX2 loads");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 1024);

        for (int kib : {8, 16, 32, 54, 64, 128, 256, 512}) {
            make_load_bench<bandwidth_test256>(maker, kib, "bandwidth-normal", "linear bandwidth", kib * 1024 / 64); // timings are per cache line
            make_load_bench<bandwidth_test256i>(maker, kib, "bandwidth-tricky", "interleaved bandwidth", kib * 1024 / 64); // timings are per cache line
            make_load_bench<bandwidth_test256i_orig>(maker, kib, "bandwidth-orig", "original bandwidth", kib * 1024 / 64); // timings are per cache line
            make_load_bench<bandwidth_test256i_double>(maker, kib, "bandwidth-oneloop-u2", "oneloop 2-wide", kib * 1024 / 64); // timings are per cache line
        }
    }

    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("studies/memory/crit-word", "Serial loads at differnet cache line offsets");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 1024 * 1024);

        ALL_SIZES_X_ARG(MAKE_SERIAL,serial_load_bench2)

        maker = maker.setLoopCount(100 * 1000); // speed things up for the bigger tests
        for (int kib = MAX_KIB * 2; kib <= MAX_SIZE / 1024; kib *= 2) {
            MAKE_SERIAL(kib,serial_load_bench2);
        }
    }

    {
        // see https://www.realworldtech.com/forum/?threadid=178902&curpostid=178902
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("studies/memory/l2-doubleload", "Serial loads at differnet cache line offsets");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 1024 * 1024);

        maker.template make<serial_double_load1>("dummy-first", "Dummy load first",  1, []{ return &shuffled_region(128 * 1024); });
        maker.template make<serial_double_load2>("dummy-second","Dummy load second", 1, []{ return &shuffled_region(128 * 1024); });
        maker.template make<serial_double_loadpf_same>  ("pf-first-same",    "Same loc prefetcht0 first", 1, []{ return &shuffled_region(128 * 1024); });
        maker.template make<serial_double_loadpf_diff>  ("pf-first-diff",    "Diff loc prefetcht0 first", 1, []{ return &shuffled_region(128 * 1024); });
        maker.template make<serial_double_loadpft1_diff>("pf-first-diff-t1",    "Diff loc prefetcht1 first", 1, []{ return &shuffled_region(128 * 1024); });

        // these tests are written in C++ and do a linked list traversal
        maker.setLoopCount(1000).template make<shuffled_list_sum>("list-traversal", "Linked list traversal + sum",  128 * 1024 / UB_CACHE_LINE_SIZE, []{ return &shuffled_region(128 * 1024); });
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



