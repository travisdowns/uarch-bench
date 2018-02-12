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

}

template <typename TIMER>
void register_vector(GroupList& list) {
    std::shared_ptr<BenchmarkGroup> vector_group = std::make_shared<BenchmarkGroup>("vector", "Vector unit bypass latency");

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

#define REG_DEFAULT(CLOCK) template void register_vector<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



