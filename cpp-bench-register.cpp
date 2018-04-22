/*
 * cpp-bench-register.cpp
 *
 * The registration for the benchmarks written in C++.
 *
 * This is in a separate file to so that the methods calls don't inlined and removed - in the C++
 * methods, you should return a value that depends on the interesting work you do in your benchmark
 * method which will prevent the function calculation itself from being optimized away.
 */

#include "benches.hpp"
#include "cpp-benches.hpp"

template <typename TIMER>
void register_cpp(GroupList& list) {
    using default_maker = BenchmarkMaker<TIMER>;

    std::shared_ptr<BenchmarkGroup> cpp_group = std::make_shared<BenchmarkGroup>("cpp", "Tests written in C++");

    size_t list_ops = LIST_COUNT;

    cpp_group->add(std::vector<Benchmark> {
        default_maker::template make_bench<div64_lat_inline>(cpp_group.get(), "div64-lat-inline",  "  Dependent inline divisions", 1,
                []{ return (void *)100; }),
        default_maker::template make_bench<div64_lat_noinline>(cpp_group.get(), "div64-lat-noinline",  "  Dependent 64-bit divisions", 1,
                        []{ return (void *)100; }),
        default_maker::template make_bench<div64_tput_inline>(cpp_group.get(), "div64-tput-inline", "Independent inline divisions", 1),
        default_maker::template make_bench<div64_tput_noinline>(cpp_group.get(), "div64-tput-noinline", "Independent divisions", 1),

        // linked list tests
        default_maker::template make_bench<linkedlist_sentinel>(cpp_group.get(), "linkedlist-sentinel", "Linked-list w/ Sentinel", list_ops,
                []{ return nullptr; }, 1000),
        default_maker::template make_bench<linkedlist_counter>(cpp_group.get(), "linkedlist-counter", "Linked-list w/ count", list_ops,
                []{ return nullptr; }, 1000),
    });

    list.push_back(cpp_group);
}

#define REG_DEFAULT(CLOCK) template void register_cpp<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)
