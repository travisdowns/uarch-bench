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

    std::shared_ptr<BenchmarkGroup> cpp_group = std::make_shared<BenchmarkGroup>("cpp", "Tests written in C++");
    list.push_back(cpp_group);

    {
        auto maker = DeltaMaker<TIMER>(cpp_group.get());

        maker.template make<div64_lat_inline>   ("div64-lat-inline",    "  Dependent inline divisions", 1, []{ return (void *)100; });
        maker.template make<div64_lat_noinline> ("div64-lat-noinline",  "  Dependent 64-bit divisions", 1, []{ return (void *)100; });
        maker.template make<div64_tput_inline>  ("div64-tput-inline",   "Independent inline divisions", 1);
        maker.template make<div64_tput_noinline>("div64-tput-noinline", "Independent divisions",        1);
    }

    {
        // linked list tests
        auto maker = DeltaMaker<TIMER>(cpp_group.get(), 1000);
        size_t list_ops = LIST_COUNT;
        maker.template make<linkedlist_sentinel>("linkedlist-sentinel", "Linked-list w/ sentinel", list_ops, []{ return nullptr; });
        maker.template make<linkedlist_counter>  ("linkedlist-counter",  "Linked-list w/ count", list_ops, []{ return nullptr; });
    }
}

#define REG_DEFAULT(CLOCK) template void register_cpp<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)
