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
    bench2_f name ## 64;    \
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

bench2_f fwd_dense_lat;
}

template <typename TIMER, bench2_f FUNC>
static Benchmark make_load_bench(const BenchmarkGroup* parent, size_t kib, const std::string &suffix) {
    using default_maker = BenchmarkMaker<TIMER>;
    std::string id = suffix + "-" + std::to_string(kib);
    std::string name = std::to_string(kib) +  "-KiB parallel " + suffix;
    return default_maker::template make_bench<FUNC>(parent, id, name, LOAD_LOOP_UNROLL,
                                    []{ return aligned_ptr(4096, 2048 * 1024); }, 100000);
}

#define BOTH_BENCH(kib) \
    make_load_bench<TIMER,load_loop ## kib>        (memload_group.get(), kib,      "loads"), \
    make_load_bench<TIMER,prefetcht0_bench  ## kib>(memload_group.get(), kib, "prefetcht0"), \
    make_load_bench<TIMER,prefetcht1_bench  ## kib>(memload_group.get(), kib, "prefetcht1"), \
    make_load_bench<TIMER,prefetcht2_bench  ## kib>(memload_group.get(), kib, "prefetcht2"), \
    make_load_bench<TIMER,prefetchnta_bench ## kib>(memload_group.get(), kib, "prefetchnta")

template <typename TIMER>
void register_mem(GroupList& list) {
    {
        std::shared_ptr<BenchmarkGroup> memload_group = std::make_shared<BenchmarkGroup>("memory/load", "Load/prefetches from fixed-size regions");

        auto benches = std::vector<Benchmark> {
            BOTH_BENCH(16),
            BOTH_BENCH(32),
            BOTH_BENCH(64),
            BOTH_BENCH(128),
            BOTH_BENCH(256),
            BOTH_BENCH(512),
            BOTH_BENCH(2048)
        };

        memload_group->add(benches);
        list.push_back(memload_group);
    }

    {
        std::shared_ptr<BenchmarkGroup> fwd_group = std::make_shared<BenchmarkGroup>("memory/store-fwd", "Store forwaring latency and throughput");

        using default_maker = BenchmarkMaker<TIMER>;

        auto benches = std::vector<Benchmark> {
            default_maker::template make_bench<fwd_dense_lat>(fwd_group.get(), "fwd-dense-lat",
                    "Dense store forward latency", 1, []{ return nullptr; /*aligned_ptr(4096, 2048 * 1024);*/ })
        };

        fwd_group->add(benches);
        list.push_back(fwd_group);
    }


}

#define REG_DEFAULT(CLOCK) template void register_mem<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



