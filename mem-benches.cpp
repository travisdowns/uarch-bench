/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benches.hpp"
#include "util.hpp"

#define LOAD_LOOP_UNROLL    8

#define BENCH_DECL_X(name) \
    bench2_f name ## 16;    \
    bench2_f name ## 32;    \
    bench2_f name ## 128;   \
    bench2_f name ## 256;   \
    bench2_f name ## 512;   \
    bench2_f name ## 2048;  \

extern "C" {
/* misc benches */
BENCH_DECL_X(load_loop)
BENCH_DECL_X(prefetcht0_bench)
BENCH_DECL_X(prefetcht1_bench)
BENCH_DECL_X(prefetcht2_bench)
BENCH_DECL_X(prefetchnta_bench)
}

template <typename TIMER, bench2_f FUNC>
static Benchmark make_load_bench(size_t kib, const std::string &suffix) {
    using default_maker = BenchmarkMaker<TIMER>;
    std::string name = std::to_string(kib) +  "-KiB parallel " + suffix;
    return default_maker::template make_bench<FUNC>(name, LOAD_LOOP_UNROLL,
                                    []{ return aligned_ptr(4096, 2048 * 1024); }, 1000000);
}

#define BOTH_BENCH(kib) \
    make_load_bench<TIMER,load_loop ## kib>(kib,         "loads"), \
    make_load_bench<TIMER,prefetcht0_bench  ## kib>(kib, "prefetcht0"), \
    make_load_bench<TIMER,prefetcht1_bench  ## kib>(kib, "prefetcht1"), \
    make_load_bench<TIMER,prefetcht2_bench  ## kib>(kib, "prefetcht2"), \
    make_load_bench<TIMER,prefetchnta_bench ## kib>(kib, "prefetchnta")

template <typename TIMER>
void register_misc(BenchmarkList& list) {
    std::shared_ptr<BenchmarkGroup> misc_group = std::make_shared<BenchmarkGroup>("memory/load");

    auto benches = std::vector<Benchmark> {
        BOTH_BENCH(16),
        BOTH_BENCH(32),
        BOTH_BENCH(128),
        BOTH_BENCH(256),
        BOTH_BENCH(512),
        BOTH_BENCH(2048)
    };

    misc_group->add(benches);
    list.push_back(misc_group);
}

#define REG_DEFAULT(CLOCK) template void register_misc<CLOCK>(BenchmarkList& list);

ALL_TIMERS_X(REG_DEFAULT)



