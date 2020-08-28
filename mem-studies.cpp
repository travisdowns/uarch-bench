/*
 * mem-benches.cpp
 *
 * Testing various memory and prefetching patterns for latency and throughput.
 */

#include <random>

#include "benchmark.hpp"
#include "cpp-benches.hpp"
#include "util.hpp"

extern "C" {

bench2_f replay_crossing;

bench2_f fw_write_read;
bench2_f fw_write_read_rcx;
bench2_f fw_write_readx;
bench2_f fw_split_write_read;
bench2_f fw_split_write_read_chained;
bench2_f fw_write_split_read;
bench2_f fw_write_split_read_both;
bench2_f fw_split_write_split_read;

bench2_f nt_normal_32;
bench2_f nt_normal_64;
bench2_f nt_normal_128;
bench2_f nt_normal_256;

bench2_f nt_extra_32;
bench2_f nt_extra_64;
bench2_f nt_extra_128;
bench2_f nt_extra_256;
}

template <bench2_f F, typename M>
static void make_mem_bench(M& maker, int kib, const char* id_prefix, const char *desc_suffix, uint32_t ops, size_t offset = 0, bool sizecheck = true) {

    maker.template make<F>(
            string_format("%s-%d", id_prefix, kib),
            string_format("%d-KiB %s", kib, desc_suffix),
            ops,
            [=]{ return &shuffled_region(kib * 1024, offset); }
    );
}

template <typename TIMER>
void register_mem_studies(GroupList& list) {
#if !UARCH_BENCH_PORTABLE

    // replay studies
    {
        std::shared_ptr<BenchmarkGroup> group =
                std::make_shared<BenchmarkGroup>("studies/replay", "Cacheline crossing loads from fixed-size regions");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 10 * 1000);

        constexpr size_t size = 10 * 1024 * 1024;
        maker.template make<replay_crossing>("replay-crossing2", "description", 100,
                                             [] { return aligned_ptr(64, size, true); });
    }

    // forwarding
    {
        std::shared_ptr<BenchmarkGroup> group =
                std::make_shared<BenchmarkGroup>("studies/forwarding", "Forwarding scenarios");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 10 * 1000);

#define DEF_FW(name) maker.template make<name>(#name, #name, 100);
        DEF_FW(fw_write_read);
        DEF_FW(fw_write_read_rcx);
        DEF_FW(fw_write_readx);
        DEF_FW(fw_split_write_read);
        DEF_FW(fw_split_write_read_chained);
        DEF_FW(fw_write_split_read);
        DEF_FW(fw_write_split_read_both);
    }

    // nt stores

    {
        std::shared_ptr<BenchmarkGroup> group =
                std::make_shared<BenchmarkGroup>("studies/nt-stores", "NT stores");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get(), 10);
        auto maker_avx2   = maker.setFeatures({AVX2});

        const size_t kib = 32 * 1024; // 128 MiB

        // the "full" variants just write a full cache line and move on to to the next
        // the "extra" variants are the same but do N extra writes to the same cache
        // line after the full line is written
        // Skylake result: up to 4 extra stores can catch the bus without any slowdown,
        // but at 5 stores, you get a slowdown
        // see: https://twitter.com/rygorous/status/1264753615551356929
#define MAKE_NT(bits) \
    make_mem_bench<nt_normal_##bits >(maker, kib, "nt-full-write-" #bits "b",  #bits "-bit NT full per CL", kib * 1024 / 64); \
    make_mem_bench<nt_extra_##bits >(maker, kib, "nt-extra-write-" #bits "b",  #bits "-bit NT extra per CL", kib * 1024 / 64);

        MAKE_NT(32)
        MAKE_NT(64)
        MAKE_NT(128)
        MAKE_NT(256)
    }

#endif  // #if !UARCH_BENCH_PORTABLE
}

#define REG_DEFAULT(CLOCK) template void register_mem_studies<CLOCK>(GroupList & list);

ALL_TIMERS_X(REG_DEFAULT)
