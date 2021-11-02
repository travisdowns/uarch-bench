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
bench2_f fw_write_read_cl_split;
bench2_f fw_write_read_rcx;
bench2_f fw_write_read_rcx2;
bench2_f fw_write_read_rcx3;
bench2_f fw_write_read_rcx4;
bench2_f fw_write_read_rcx5;
bench2_f fw_write_read_rcx4s;
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

bench2_f parallel_misses;
bench2_f add_misses;
bench2_f lockadd_misses;
bench2_f xadd_misses;
bench2_f lockxadd_misses;
bench2_f lfenced_misses;
bench2_f mfenced_misses;
bench2_f sfenced_misses;
bench2_f sfenced_misses2x;
bench2_f sfence_lfence_misses;

bench2_f parallel_misses_store;
bench2_f add_misses_store;
bench2_f lockadd_misses_store;
bench2_f xadd_misses_store;
bench2_f lockxadd_misses_store;
bench2_f lfenced_misses_store;
bench2_f mfenced_misses_store;
bench2_f sfenced_misses_store;
bench2_f sfenced_misses2x_store;
bench2_f sfence_lfence_misses_store;

bench2_f add_fencesolo;
bench2_f lockadd_fencesolo;
bench2_f xadd_fencesolo;
bench2_f lockxadd_fencesolo;

#define LOAD_PATTERNS_X(f) \
        f(4, 0_0_0_0)         \
        f(4, 0_4_0_4)         \
        f(4, 0_8_0_8)         \
        f(4, 0_12_0_12)       \
        f(4, 0_16_0_16)       \
        f(4, 0_20_0_20)       \
        f(4, 0_24_0_24)       \
        f(4, 0_0_8_8)         \
        f(4, 0_0_0_8)         \
        f(4, 4_8_4_8)         \
        f(4, 0_4_8_12)        \
        f(4, 1_5_9_13)        \
        f(4, 12_8_4_0)        \
        f(4, 0_32_0_32)       \
        f(4, 0_64_0_64)       \
        f(4, 0_96_0_96)       \
        f(4, 0_72_0_72)       \
        f(4, reg_0_4_8_12)    \
        f(3, 0_4_8)           \
        f(3, 0_8_80)          \

#define DECLARE_LOAD_PATTERN_BENCH(_, suffix) bench2_f load_pattern_##suffix;
LOAD_PATTERNS_X(DECLARE_LOAD_PATTERN_BENCH)

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

#define DEF_FW2(fn, id, desc) maker.template make<fn>(id, desc, 100);
#define DEF_FW(name)  DEF_FW2(name, #name, #name)

        DEF_FW(fw_write_read);
        DEF_FW2(fw_write_read_cl_split, "fw_write_read_cl_split", "Cacheline split forwarding");
        DEF_FW(fw_write_read_rcx);
        DEF_FW(fw_write_read_rcx2);
        DEF_FW(fw_write_read_rcx3);
        DEF_FW(fw_write_read_rcx4);
        DEF_FW(fw_write_read_rcx5);
        DEF_FW(fw_write_read_rcx4s);
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
        auto maker = DeltaMaker<TIMER>(group.get(), 10).setTags({"slow"});
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
 

    {
        // determine the effect of fencing and LOCKed instructions on load misses
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("studies/fencing", "Fencing benches");
        list.push_back(group);

        constexpr size_t BUFSIZE = (1u << 25);

        auto aligned_buf = [](){ return aligned_ptr(4096, BUFSIZE); };
        auto maker = DeltaMaker<TIMER>(group.get()).setLoopCount(BUFSIZE / UB_CACHE_LINE_SIZE).setTags({"slow"});

#define MAKE_INTERLEAVED(name,op) \
        maker.template make<name>(#name, "Interleaved load  misses and " #op, 1, aligned_buf); \
        maker.template make<name##_store>(#name "_store", "Interleaved store misses and " #op, 1, aligned_buf); \

        // these interleave a vanilla miss with the payload instruction (which doesn't miss)
        MAKE_INTERLEAVED(parallel_misses , nop);
        MAKE_INTERLEAVED(add_misses      , add);
        MAKE_INTERLEAVED(lockadd_misses  , lock add);
        MAKE_INTERLEAVED(xadd_misses     , xadd);
        MAKE_INTERLEAVED(lockxadd_misses , lock xadd);
        MAKE_INTERLEAVED(mfenced_misses  , mfence);
        MAKE_INTERLEAVED(lfenced_misses  , lfence);
        MAKE_INTERLEAVED(sfenced_misses  , sfence);
        MAKE_INTERLEAVED(sfenced_misses2x, 2x sfence);
        MAKE_INTERLEAVED(sfence_lfence_misses, s & lfence);


#define MAKE_SOLO(name,op) maker.template make<name>(#name, "Solo " #op " that miss", 1, aligned_buf)


        MAKE_SOLO(add_fencesolo, adds);
        MAKE_SOLO(lockadd_fencesolo, lock adds);
        MAKE_SOLO(xadd_fencesolo, xadds);
        MAKE_SOLO(lockxadd_fencesolo, lock xadds);

    }


     {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("studies/load-patterns", "Load patterns loads");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get());

        #define DEFINE_LOAD_PATTERN_BENCH(size, pat) \
            maker.template make<load_pattern_##pat>("load-pattern-" #pat,  "32-bit loads with pattern " #pat,  1);

        LOAD_PATTERNS_X(DEFINE_LOAD_PATTERN_BENCH)
     }

#endif  // #if !UARCH_BENCH_PORTABLE
}

#define REG_DEFAULT(CLOCK) template void register_mem_studies<CLOCK>(GroupList & list);

ALL_TIMERS_X(REG_DEFAULT)
