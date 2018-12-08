/*
 * cpp-bench-register.cpp
 *
 * The registration for the benchmarks written in C++.
 *
 * This is in a separate file to so that the methods calls don't inlined and removed - in the C++
 * methods, you should return a value that depends on the interesting work you do in your benchmark
 * method which will prevent the function calculation itself from being optimized away.
 */

#include "benchmark.hpp"
#include "cpp-benches.hpp"

#define DIV_REG_X(f) \
        f( 32_64, " 32b / 64b") \
        f( 64_64, " 64b / 64b") \
        f(128_64, "128b / 64b") \

#define DIV_REG(suffix, text) \
        maker.template make<div_lat_inline   ##suffix> ("div"#suffix"-lat",    "Dependent   " text "   inline divisions", 1); \
        /*maker.template make<div_lat_noinline ##suffix >("div"#suffix"-lat-ni", "Dependent   " text " noinline divisions", 1); */\
        maker.template make<div_tput_inline  ##suffix >("div"#suffix"-tput",   "Independent " text "   inline divisions", 1); \
        /*maker.template make<div_tput_noinline##suffix> ("div"#suffix"-tput-ni","Independent " text " noinline divisions", 1); */\

template <typename TIMER>
void register_cpp(GroupList& list) {

    std::shared_ptr<BenchmarkGroup> cpp_group = std::make_shared<BenchmarkGroup>("cpp", "Tests written in C++");
    list.push_back(cpp_group);

    {
        auto maker = DeltaMaker<TIMER>(cpp_group.get());

        DIV_REG_X(DIV_REG)

        maker.template make<gettimeofday_bench>   ("gettimeofday",    "gettimeofday() libc call", 1);
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
