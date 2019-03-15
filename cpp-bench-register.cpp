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

#include <cstring>

#define DIV_REG_X(f) \
        f( 32_64, " 32b / 64b") \
        f( 64_64, " 64b / 64b") \
        f(128_64, "128b / 64b") \

#define DIV_REG(suffix, text) \
        maker.template make<div_lat_inline   ##suffix> ("div"#suffix"-lat",    "Dependent   " text "   inline divisions", 1); \
        /*maker.template make<div_lat_noinline ##suffix >("div"#suffix"-lat-ni", "Dependent   " text " noinline divisions", 1); */\
        maker.template make<div_tput_inline  ##suffix >("div"#suffix"-tput",   "Independent " text "   inline divisions", 1); \
        /*maker.template make<div_tput_noinline##suffix> ("div"#suffix"-tput-ni","Independent " text " noinline divisions", 1); */\


/* leaks a pointer to a region of the given size */
region* make_region(size_t size) {
    auto r = new region{size, aligned_ptr(4096, size)}; // leak
    std::memset(r->start, 0, r->size);
    return r;
}

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

    {
        // flush region tests
        std::shared_ptr<BenchmarkGroup> util_group = std::make_shared<BenchmarkGroup>("util", "Benchmarks for utility functions");
        list.push_back(util_group);
        auto maker = DeltaMaker<TIMER>(util_group.get(), 1);  // just one iteration so we test the "in cache" performance

        for (size_t size : {1, 10, 100, 1000, 10000, 100000}) {
            std::string sizestr = std::to_string(size), name = "-region-" + sizestr;
            std::string desc = "of " + sizestr + " KB (per line)";
//            uint32_t loop_count = 10000 / size;
//            if (loop_count <= 5) loop_count = 5;

            maker.template make<flush_region_bench   >("clflush " + name, "clflush " + desc, size * 1024 / 64, [=]{ return make_region(size * 1024); });
            maker.template make<flushopt_region_bench>("clflushopt " + name, "clflushopt " + desc, size * 1024 / 64, [=]{ return make_region(size * 1024); });
        }

    }
}

#define REG_DEFAULT(CLOCK) template void register_cpp<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)
