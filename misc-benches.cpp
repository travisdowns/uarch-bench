/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "asm_methods.h"
#include "benches.hpp"

extern "C" {
/* misc benches */
bench2_f misc_add_loop32;
bench2_f misc_add_loop64;
}

template <typename TIMER>
void register_misc(BenchmarkList& list) {
    std::shared_ptr<BenchmarkGroup> default_group = std::make_shared<BenchmarkGroup>("misc");

    using default_maker = BenchmarkMaker<TIMER>;

    auto benches = std::vector<Benchmark> {
        default_maker::template make_bench<misc_add_loop32>("32-bit add-loop", 1, []{ return nullptr; }, 100000),
        default_maker::template make_bench<misc_add_loop64>("64-bit add-loop", 1, []{ return nullptr; }, 100000)
    };

    default_group->add(benches);
    list.push_back(default_group);
}

#define REG_DEFAULT(CLOCK) template void register_misc<CLOCK>(BenchmarkList& list);

ALL_TIMERS_X(REG_DEFAULT)



