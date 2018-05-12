/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benches.hpp"

extern "C" {

bench2_f bypass_vmovdqa_latency;
bench2_f bypass_vmovdqu_latency;
bench2_f bypass_vmovups_latency;
bench2_f bypass_vmovupd_latency;

bench2_f bypass_movd_latency;
bench2_f bypass_movq_latency;

bench2_f vector_load_load_lat_movdqu_0;
bench2_f vector_load_load_lat_vmovdqu_0;
bench2_f vector_load_load_lat_lddqu_0;
bench2_f vector_load_load_lat_vlddqu_0;

bench2_f vector_load_load_lat_movdqu_63;
bench2_f vector_load_load_lat_vmovdqu_63;
bench2_f vector_load_load_lat_lddqu_63;
bench2_f vector_load_load_lat_vlddqu_63;


}

template <typename TIMER>
void register_vector(GroupList& list) {
    {
        std::shared_ptr<BenchmarkGroup> vector_group = std::make_shared<BenchmarkGroup>("vector/bypass", "Vector unit bypass latency");

        using default_maker = BenchmarkMaker<TIMER>;

        auto benches = std::vector<Benchmark> {
            default_maker::template make_bench<bypass_vmovdqa_latency>(vector_group.get(), "movdqa", "movdqa [mem] -> paddb latency", 1, []{ return nullptr; }, 100000),
                    default_maker::template make_bench<bypass_vmovdqu_latency>(vector_group.get(), "movdqu", "movdqu [mem] -> paddb latency", 1, []{ return nullptr; }, 100000),
                    default_maker::template make_bench<bypass_vmovups_latency>(vector_group.get(), "movups", "movups [mem] -> paddb latency", 1, []{ return nullptr; }, 100000),
                    default_maker::template make_bench<bypass_vmovupd_latency>(vector_group.get(), "movupd", "movupd [mem] -> paddb latency", 1, []{ return nullptr; }, 100000),

                    default_maker::template make_bench<bypass_movd_latency>(vector_group.get(), "movd",   "movq rax,xmm0 -> xmm0,rax lat", 1, []{ return nullptr; }, 100000),
                    default_maker::template make_bench<bypass_movq_latency>(vector_group.get(), "movq",   "movq rax,xmm0 -> xmm0,rax lat", 1, []{ return nullptr; }, 100000)
        };

        vector_group->add(benches);
        list.push_back(vector_group);
    }

    {
        std::shared_ptr<BenchmarkGroup> vector_group = std::make_shared<BenchmarkGroup>("vector/load-load", "Vector load-load latency");

        using default_maker = BenchmarkMaker<TIMER>;

        auto benches = std::vector<Benchmark> {
            default_maker::template make_bench< vector_load_load_lat_movdqu_0>(vector_group.get(),  "movdqu-aligned",    "aligned  movdqu load lat", 1, []{ return nullptr; }, 100000),
            default_maker::template make_bench<vector_load_load_lat_vmovdqu_0>(vector_group.get(), "vmovdqu-aligned",   "aligned vmovdqu load lat", 1, []{ return nullptr; }, 100000),
            default_maker::template make_bench<  vector_load_load_lat_lddqu_0>(vector_group.get(),   "lddqu-aligned",     "aligned   lddqu load lat", 1, []{ return nullptr; }, 100000),
            default_maker::template make_bench< vector_load_load_lat_vlddqu_0>(vector_group.get(),  "vlddqu-aligned",    "aligned  vlddqu load lat", 1, []{ return nullptr; }, 100000),

            default_maker::template make_bench< vector_load_load_lat_movdqu_63>(vector_group.get(),  "movdqu-misaligned",  "misaligned  movdqu load lat", 1, []{ return nullptr; }, 100000),
            default_maker::template make_bench<vector_load_load_lat_vmovdqu_63>(vector_group.get(), "vmovdqu-misaligned", "misaligned vmovdqu load lat", 1, []{ return nullptr; }, 100000),
            default_maker::template make_bench<  vector_load_load_lat_lddqu_63>(vector_group.get(),   "lddqu-misaligned",   "misaligned   lddqu load lat", 1, []{ return nullptr; }, 100000),
            default_maker::template make_bench< vector_load_load_lat_vlddqu_63>(vector_group.get(),  "vlddqu-misaligned",  "misaligned  vlddqu load lat", 1, []{ return nullptr; }, 100000),

        };

        vector_group->add(benches);
        list.push_back(vector_group);
    }
}

#define REG_DEFAULT(CLOCK) template void register_vector<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



