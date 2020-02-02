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

bench2_f fw_write_read;
bench2_f fw_write_readx;
bench2_f fw_split_write_read;
bench2_f fw_split_write_read_chained;
bench2_f fw_write_split_read;
bench2_f fw_write_split_read_both;
bench2_f fw_split_write_split_read;


}

template <typename TIMER>
void register_mem_studies(GroupList& list)
{
#if !UARCH_BENCH_PORTABLE
    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("studies/replay", "Cacheline crossing loads from fixed-size regions");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 10 * 1000);

        constexpr size_t size = 10 * 1024 * 1024;
        maker.template make<replay_crossing>("replay-crossing2", "description", 100, []{ return aligned_ptr(64, size, true); });
    }

    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("studies/forwarding", "Forwarding scenarios");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 10 * 1000);

        #define DEF_FW(name) maker.template make<name> (#name, #name, 100);
        DEF_FW(fw_write_read);
        DEF_FW(fw_write_readx);
        DEF_FW(fw_split_write_read);
        DEF_FW(fw_split_write_read_chained);
        DEF_FW(fw_write_split_read);
        DEF_FW(fw_write_split_read_both);
    }


#endif // #if !UARCH_BENCH_PORTABLE

}

#define REG_DEFAULT(CLOCK) template void register_mem_studies<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



