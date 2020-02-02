/*
 * mem-benches.cpp
 *
 * Testing various memory and prefetching patterns for latency and throughput.
 */

#include "cpp-benches.hpp"
#include "util.hpp"

#include <random>
#include "benchmark.hpp"

extern "C" {

bench2_f replay_crossing;

}

template <typename TIMER>
void register_mem_studies(GroupList& list)
{
#if !UARCH_BENCH_PORTABLE
    {
        // this group of tests isn't directly comparable to the parallel tests since the access pattern is "more random" than the
        // parallel test, which is strided albeit with a large stride. In particular it's probably worse for the TLB. The result is
        // that the implied "max MLP" derived by dividing the serial access time by the parallel one is larger than 10 (about 12.5),
        // which I think is impossible on current Intel. We should make comparable parallel/serial tests that have identical access
        // patterns.
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("studies/replay", "Cacheline crossing loads from fixed-size regions");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 10 * 1000);

        // for (int kib = 8; kib <= MAX_SIZE / 1024; kib *= 2) {
        //     auto& maker = kib > MAX_KIB ? maker_slow : maker_fast;
        //     MAKE_SERIALO(kib, serial_load_bench, -1);
        // }
        constexpr size_t size = 10 * 1024 * 1024;
        maker.template make<replay_crossing>("replay-crossing2", "description", 100, []{ return aligned_ptr(64, size, true); });
    }


#endif // #if !UARCH_BENCH_PORTABLE

}

#define REG_DEFAULT(CLOCK) template void register_mem_studies<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



