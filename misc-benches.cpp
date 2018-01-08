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
bench2_f bmi_tzcnt;
bench2_f bmi_lzcnt;
bench2_f bmi_popcnt;
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
}

#define REG_DEFAULT(CLOCK) template void register_misc<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



