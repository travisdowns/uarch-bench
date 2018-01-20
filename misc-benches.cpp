/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benches.hpp"

extern "C" {
/* misc benches */
bench2_f misc_add_loop32;
bench2_f misc_add_loop64;
bench2_f misc_port7;
bench2_f bmi_tzcnt;
bench2_f bmi_lzcnt;
bench2_f bmi_popcnt;

bench2_f retpoline_dense_call_lfence;
bench2_f retpoline_dense_call_pause;
bench2_f retpoline_sparse_call_base;
bench2_f retpoline_sparse_indep_call_lfence;
bench2_f retpoline_sparse_indep_call_pause;
bench2_f retpoline_sparse_dep_call_lfence;
bench2_f retpoline_sparse_dep_call_pause;
bench2_f indirect_dense_call_pred;
bench2_f indirect_dense_call_unpred;
}

template <typename TIMER>
void register_misc(GroupList& list) {
    std::shared_ptr<BenchmarkGroup> misc_group = std::make_shared<BenchmarkGroup>("misc", "Miscellaneous tests");

    using default_maker = BenchmarkMaker<TIMER>;

    const uint32_t iters = 10*1000*1000;
    auto benches = std::vector<Benchmark> {
        default_maker::template make_bench<misc_add_loop32>(misc_group.get(), "add-32", "32-bit add-loop", 1,
                []{ return nullptr; }, iters),
        default_maker::template make_bench<misc_add_loop64>(misc_group.get(), "add-64", "64-bit add-loop", 1,
                []{ return nullptr; }, iters),
        default_maker::template make_bench<misc_port7>(misc_group.get(), "port7", "Can port7 be used by loads", 1,
                        []{ return nullptr; }, iters)
    };

    misc_group->add(benches);
    list.push_back(misc_group);

    std::shared_ptr<BenchmarkGroup> bmi_group = std::make_shared<BenchmarkGroup>("bmi", "BMI false-dependency tests");

    bmi_group->add(std::vector<Benchmark> {
        default_maker::template make_bench<bmi_tzcnt>(bmi_group.get(), "dep-tzcnt", "dest-dependent tzcnt", 128),
        default_maker::template make_bench<bmi_lzcnt>(bmi_group.get(), "dep-lzcnt", "dest-dependent lzcnt", 128),
        default_maker::template make_bench<bmi_popcnt>(bmi_group.get(),"dep-popcnt", "dest-dependent popcnt", 128)
    });
    list.push_back(bmi_group);

    std::shared_ptr<BenchmarkGroup> retpoline_group = std::make_shared<BenchmarkGroup>("misc/retpoline", "retpoline tests");
    retpoline_group->add(std::vector<Benchmark> {
        default_maker::template make_bench<retpoline_dense_call_pause> (retpoline_group.get(), "retp-call-pause", "Dense retpoline call  pause", 32),
        default_maker::template make_bench<retpoline_dense_call_lfence>(retpoline_group.get(), "retp-call-lfence", "Dense retpoline call lfence", 32),
        default_maker::template make_bench<indirect_dense_call_pred>(retpoline_group.get(),    "ibra-call-pred", "Dense indirect pred calls", 32),
        default_maker::template make_bench<indirect_dense_call_unpred>(retpoline_group.get(),  "ibra-call-unpred", "Dense indirect unpred calls", 32),
        default_maker::template make_bench<retpoline_sparse_call_base, retpoline_sparse_indep_call_pause> (retpoline_group.get(), "retp-sparse-indep-call-pause", "Sparse retpo indep call  pause", 8),
        default_maker::template make_bench<retpoline_sparse_call_base, retpoline_sparse_indep_call_lfence>(retpoline_group.get(), "retp-sparse-indep-call-lfence", "Sparse retpo indep call lfence", 8),
        default_maker::template make_bench<retpoline_sparse_call_base, retpoline_sparse_dep_call_pause> (retpoline_group.get(),   "retp-sparse-dep-call-pause", "Sparse retpo dep call  pause", 8),
        default_maker::template make_bench<retpoline_sparse_call_base, retpoline_sparse_dep_call_lfence>(retpoline_group.get(),   "retp-sparse-dep-call-lfence", "Sparse retpo dep call lfence", 8)
    });
    list.push_back(retpoline_group);
}

#define REG_DEFAULT(CLOCK) template void register_misc<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



