/*
 * vector-benches.cpp
 *
 * Benchmarks relating to SSE and AVX instructions.
 */

#include "benchmark.hpp"

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


}

template <typename TIMER>
void register_vector(GroupList& list) {
#if !UARCH_BENCH_PORTABLE
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
    }

#endif // #if !UARCH_BENCH_PORTABLE
}

#define REG_DEFAULT(CLOCK) template void register_vector<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



