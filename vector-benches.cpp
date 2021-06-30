/*
 * vector-benches.cpp
 *
 * Benchmarks relating to SSE and AVX instructions.
 */

#if !UARCH_BENCH_PORTABLE

#ifdef __AVX2__
#include <immintrin.h>
#endif

#include "benchmark.hpp"
#include "opt-control.hpp"

extern "C" {

bench2_f bypass_vmovdqa_latency;
bench2_f bypass_vmovdqu_latency;
bench2_f bypass_vmovups_latency;
bench2_f bypass_vmovupd_latency;

bench2_f bypass_movd_latency;
bench2_f bypass_movq_latency;

bench2_f vector_load_load_lat_movdqu_0_xmm;
bench2_f vector_load_load_lat_vmovdqu_0_xmm;
bench2_f vector_load_load_lat_lddqu_0_xmm;
bench2_f vector_load_load_lat_vlddqu_0_xmm;
bench2_f vector_load_load_lat_vmovdqu_0_ymm;
bench2_f vector_load_load_lat_vmovdqu32_0_zmm;

bench2_f vector_load_load_lat_movdqu_63_xmm;
bench2_f vector_load_load_lat_vmovdqu_63_xmm;
bench2_f vector_load_load_lat_lddqu_63_xmm;
bench2_f vector_load_load_lat_vlddqu_63_xmm;
bench2_f vector_load_load_lat_vmovdqu_63_ymm;
bench2_f vector_load_load_lat_vmovdqu32_63_zmm;

bench2_f vector_load_load_lat_simple_vmovdqu_0_xmm;
bench2_f vector_load_load_lat_simple_vmovdqu_0_ymm;
bench2_f vector_load_load_lat_simple_vmovdqu32_0_zmm;

bench2_f vector_load_load_lat_double_vmovdqu_0_xmm;
bench2_f vector_load_load_lat_double_vmovdqu_0_ymm;
bench2_f vector_load_load_lat_double_vmovdqu32_0_zmm;
bench2_f vector_load_load_lat_double_vmovdqu_3200_xmm;
bench2_f vector_load_load_lat_double_vmovdqu_3200_ymm;
bench2_f vector_load_load_lat_double_vmovdqu32_3200_zmm;

bench2_f p01_fusion_p1;
bench2_f p01_fusion_p0;

}

#ifdef __AVX2__

/**
 * Specialization of modify for double to use the xmm
 * register type (x86 specific).
 */
HEDLEY_ALWAYS_INLINE
static void modify(__m256i& x) {
    __asm__ volatile (" " :"+x"(x)::);
}

HEDLEY_NEVER_INLINE
long intrinsic_bench(uint64_t iters, void*) {

    __m256i total = _mm256_setzero_si256(), addend = _mm256_set1_epi32(1);

    do {
        for (int i = 0; i < 4; i++) {
            total = _mm256_add_epi32(total, addend);
            modify(total);
        }
    } while (--iters);

    return _mm256_extract_epi32(total, 0);
}

#endif

template <typename TIMER>
void register_vector(GroupList& list) {
    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("vector/bypass", "Vector unit bypass latency");
        list.push_back(group);

        auto maker = DeltaMaker<TIMER>(group.get(), 100000);

        maker.template make<bypass_vmovdqa_latency>("movdqa", "movdqa [mem] -> paddb latency", 1);
        maker.template make<bypass_vmovdqu_latency>("movdqu", "movdqu [mem] -> paddb latency", 1);
        maker.template make<bypass_vmovups_latency>("movups", "movups [mem] -> paddb latency", 1);
        maker.template make<bypass_vmovupd_latency>("movupd", "movupd [mem] -> paddb latency", 1);

        maker.template make<bypass_movd_latency>("movd",   "movq rax,xmm0 -> xmm0,rax lat", 1);
        maker.template make<bypass_movq_latency>("movq",   "movq rax,xmm0 -> xmm0,rax lat", 1);

    }

    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("vector/load-load", "Vector load-load latency");
        list.push_back(group);

        auto maker = DeltaMaker<TIMER>(group.get(), 100000).setFeatures({AVX2});
        auto m512  = maker.setFeatures({AVX512F});

        maker.template make< vector_load_load_lat_movdqu_0_xmm  >(  "movdqu-aligned"    ,    "aligned  movdqu xmm load lat", 1);
        maker.template make<vector_load_load_lat_vmovdqu_0_xmm  >( "vmovdqu-aligned"    ,    "aligned vmovdqu xmm load lat", 1);
        maker.template make<  vector_load_load_lat_lddqu_0_xmm  >(   "lddqu-aligned"    ,    "aligned   lddqu xmm load lat", 1);
        maker.template make< vector_load_load_lat_vlddqu_0_xmm  >(  "vlddqu-aligned"    ,    "aligned  vlddqu xmm load lat", 1);
        maker.template make<vector_load_load_lat_vmovdqu_0_ymm  >( "vmovdqu-aligned-ymm",    "aligned vmovdqu ymm load lat", 1);
        m512 .template make<vector_load_load_lat_vmovdqu32_0_zmm>( "vmovdqu-aligned-zmm",    "aligned vmovdqu zmm load lat", 1);

        maker.template make< vector_load_load_lat_movdqu_63_xmm   >( "movdqu-misaligned"    , "misaligned  movdqu xmm load lat", 1);
        maker.template make<vector_load_load_lat_vmovdqu_63_xmm   >("vmovdqu-misaligned"    , "misaligned vmovdqu xmm load lat", 1);
        maker.template make<  vector_load_load_lat_lddqu_63_xmm   >(  "lddqu-misaligned"    , "misaligned   lddqu xmm load lat", 1);
        maker.template make< vector_load_load_lat_vlddqu_63_xmm   >( "vlddqu-misaligned"    , "misaligned  vlddqu xmm load lat", 1);
        maker.template make<vector_load_load_lat_vmovdqu_63_ymm   >("vmovdqu-misaligned-ymm", "misaligned vmovdqu ymm load lat", 1);
        m512 .template make<vector_load_load_lat_vmovdqu32_63_zmm>("vmovdqu-misaligned-zmm", "misaligned vmovdqu zmm load lat", 1);

        maker.template make<vector_load_load_lat_simple_vmovdqu_0_xmm  >( "simple vmovdqu xmm",    "aligned vmovdqu xmm load lat", 1);
        maker.template make<vector_load_load_lat_simple_vmovdqu_0_ymm  >( "simple vmovdqu ymm",    "aligned vmovdqu ymm load lat", 1);
        m512 .template make<vector_load_load_lat_simple_vmovdqu32_0_zmm>( "simple vmovdqu zmm",    "aligned vmovdqu zmm load lat", 1);

        maker.template make<vector_load_load_lat_double_vmovdqu_0_xmm  >   ("chained vmovdqu xmm",  "gp load -> vmovdqu x load lat", 1);
        maker.template make<vector_load_load_lat_double_vmovdqu_0_ymm  >   ("chained vmovdqu ymm",  "gp load -> vmovdqu y load lat", 1);
        m512 .template make<vector_load_load_lat_double_vmovdqu32_0_zmm>   ("chained vmovdqu zmm",  "gp load -> vmovdqu z load lat", 1);
        maker.template make<vector_load_load_lat_double_vmovdqu_3200_xmm  >("chained complex vmovdqu xmm", "gp load -> vmovdqu x load lat", 1);
        maker.template make<vector_load_load_lat_double_vmovdqu_3200_ymm  >("chained complex vmovdqu ymm", "gp load -> vmovdqu y load lat", 1);
        m512 .template make<vector_load_load_lat_double_vmovdqu32_3200_zmm>("chained complex vmovdqu zmm", "gp load -> vmovdqu z load lat", 1);
    }

    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("vector/misc", "Miscellaneous vector benches");
        list.push_back(group);

        auto m512  = DeltaMaker<TIMER>(group.get(), 1000).setFeatures({AVX512F});

        m512.template make<p01_fusion_p1>("p01-fusion-p1", "check that scalar ops go to p1", 100);
        m512.template make<p01_fusion_p0>("p01-fusion-p0", "check that scalar ops go to p0", 100);

#ifdef __AVX2__
        auto maker = DeltaMaker<TIMER>(group.get()).setFeatures({AVX2});

        maker.template make<intrinsic_bench>("intrinsic", "demo how to write intrinsic bench", 4);
        maker.useLoopDelta().template make<intrinsic_bench>("intrinsic-loop-delta", "demo with loop delta", 4);
#endif
    }

}

#define REGISTER_ALL(CLOCK) template void register_vector<CLOCK>(GroupList& list);

ALL_TIMERS_X(REGISTER_ALL)

#endif // #if !UARCH_BENCH_PORTABLE


