/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benches.hpp"

extern "C" {
bench2_f dep_add_rax_rax;
bench2_f indep_add;
bench2_f dep_imul128_rax;
bench2_f dep_imul64_rax;
bench2_f indep_imul128_rax;
bench2_f store_same_loc;
bench2_f store64_disjoint;
}

template <typename TIMER>
void register_default(GroupList& list) {
    std::shared_ptr<BenchmarkGroup> default_group = std::make_shared<BenchmarkGroup>("default", "Default Group");
    const BenchmarkGroup* group_ptr = default_group.get();

    using default_maker = BenchmarkMaker<TIMER>;

    auto benches = std::vector<Benchmark> {
        default_maker::template make_bench<dep_add_rax_rax>  (group_ptr, "dep-add", "Dependent add chain",       128),
        default_maker::template make_bench<indep_add>        (group_ptr, "indep-add", "Independent add chain",  50 * 8),
        default_maker::template make_bench<dep_imul128_rax>  (group_ptr, "dep-mul128", "Dependent imul 64->128",    128),
        default_maker::template make_bench<dep_imul64_rax>   (group_ptr, "dep-mul64",  "Dependent imul 64->64",     128),
        default_maker::template make_bench<indep_imul128_rax>(group_ptr, "indep-mul128", "Independent imul 64->128",  128),
        default_maker::template make_bench<store_same_loc>   (group_ptr, "same-stores", "Same location stores",      128),
        default_maker::template make_bench<store64_disjoint> (group_ptr, "disjoin-stores", "Disjoint location stores",  128)
    };

    default_group->add(benches);
    list.push_back(default_group);
}

#define REG_DEFAULT(CLOCK) template void register_default<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



