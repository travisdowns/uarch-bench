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
        maker.template make<div_lat_inline   ##suffix> ("div"#suffix"-lat",    "Dependent   " text "  inline divisions", 1); \
        /*maker.template make<div_lat_noinline ##suffix >("div"#suffix"-lat-ni", "Dependent   " text " noinline divisions", 1); */\
        maker.template make<div_tput_inline  ##suffix >("div"#suffix"-tput",   "Independent " text "  inline divisions", 1); \
        /*maker.template make<div_tput_noinline##suffix> ("div"#suffix"-tput-ni","Independent " text " noinline divisions", 1); */\



template <bench2_f F, typename M>
void make_strided_stores(M& maker, size_t data_size) {
    std::string prefix = std::to_string(data_size * 8);
    mem_args samelocargs{(char *)aligned_ptr(64, 1024), 0, 0};
    maker.  setLoopCount(10000).
                        template make<F>(prefix + "-sameloc-stores",
                        string_format("%2zu bit stores to same location          ", data_size * 8),
                        1,
                        [=]{ return new mem_args(samelocargs); });
    for (size_t stride : {1, 2, 4, 8, 16, 32, 64, 128}) {
        for (size_t kib : {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048}) {
            size_t region_bytes = kib * 1024;
            size_t access_per_region = region_bytes / stride;
            assert(is_pow2(kib));
            auto stride_str = std::to_string(stride);
            std::string region = std::to_string(kib) + " KiB";
            mem_args args{(char *)aligned_ptr(64, (kib + 1) * 1024), stride, region_bytes - 1};
            maker.  setLoopCount(10 * access_per_region).
                    template make<F>(prefix + "-strided-" + stride_str + "-stores-" + std::to_string(kib) + "-kib",
                    string_format("%2zu bit stores, stride %3zu, size %4zu KiB", data_size * 8, stride, kib),
                    1,
                    [=]{ return new mem_args(args); });
        }
    }
}

template <typename TIMER>
void register_cpp(GroupList& list) {

    std::shared_ptr<BenchmarkGroup> cpp_group = std::make_shared<BenchmarkGroup>("cpp", "Tests written in C++");
    list.push_back(cpp_group);

    {
        auto maker = DeltaMaker<TIMER>(cpp_group.get()).useLoopDelta();

        DIV_REG_X(DIV_REG)

        maker.template make<gettimeofday_bench>("gettimeofday",    "gettimeofday() libc call", 1);
        maker.template make<crc8_bench>("crc8",  "crc8 loop", 4096);
        maker.setLoopCount(1000).template make<sum_halves_bench>("sum-halves",  "Sum 16-bit halves of array elems", 2048);
        maker.setLoopCount(1000).template make<mul_by_bench>("mul-4", "Four multiplications", 4096);
        maker.setLoopCount(1000).template make<mul_chain_bench>("mul-chain",  "Chained multiplications", 4096);
        maker.setLoopCount(1000).template make<mul_chain4_bench>("mul-chain4", "Chained multiplications, 4 chains", 4096);
        maker.setLoopCount(1000).template make<add_indirect      >("add-indirect",       "Indirect adds from memory", 2048);
        maker.setLoopCount(1000).template make<add_indirect_shift>("add-indirect-shift", "Indirect adds from memory, tricky", 2048);
    }

    {
        // linked list tests
        auto maker = DeltaMaker<TIMER>(cpp_group.get(), 1000);
        size_t list_ops = LIST_COUNT;
        maker.template make<linkedlist_sentinel>("linkedlist-sentinel", "Linked-list w/ sentinel", list_ops);
        maker.template make<linkedlist_counter>  ("linkedlist-counter",  "Linked-list w/ count", list_ops);
    }

    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("memory/cpp/store", "Strided stores");
        list.push_back(group);
        auto maker        = DeltaMaker<TIMER>(group.get());

        make_strided_stores<strided_stores_1byte>(maker, 1);
        make_strided_stores<strided_stores_4byte>(maker, 4);
        make_strided_stores<strided_stores_8byte>(maker, 8);
    }
}

#define REG_DEFAULT(CLOCK) template void register_cpp<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)
