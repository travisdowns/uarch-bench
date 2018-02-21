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
bench2_f dep_pushpop;
bench2_f indep_pushpop;
}

template <typename TIMER>
void register_default(GroupList& list) {
    std::shared_ptr<BenchmarkGroup> default_group = std::make_shared<BenchmarkGroup>("default", "Default Group");
    list.push_back(default_group);

    auto maker = DeltaMaker<TIMER>(default_group.get());

    maker.template make<dep_add_rax_rax>  ("dep-add", "Dependent add chain",       128);
    maker.template make<indep_add>        ("indep-add", "Independent add chain",  50 * 8);
    maker.template make<dep_imul128_rax>  ("dep-mul128", "Dependent imul 64->128",    128);
    maker.template make<dep_imul64_rax>   ("dep-mul64",  "Dependent imul 64->64",     128);
    maker.template make<indep_imul128_rax>("indep-mul128", "Independent imul 64->128",  128);
    maker.template make<store_same_loc>   ("same-stores", "Same location stores",      128);
    maker.template make<store64_disjoint> ("disjoint-stores", "Disjoint location stores",  128);
    maker.template make<dep_pushpop>      ("dep-push-pop", "Dependent push/pop chain",  128);
    maker.template make<indep_pushpop>    ("indep-push-pop", "Inependent push/pop chain",  128);
}

#define REG_DEFAULT(CLOCK) template void register_default<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



