/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benchmark.hpp"

extern "C" {
bench2_f dense_calls;
bench2_f sparse0_calls;
bench2_f sparse1_calls;
bench2_f sparse2_calls;
bench2_f sparse3_calls;
bench2_f sparse4_calls;
bench2_f sparse5_calls;
bench2_f sparse6_calls;
bench2_f sparse7_calls;

bench2_f chained0_calls;
bench2_f chained1_calls;
bench2_f chained2_calls;
bench2_f chained3_calls;

bench2_f pushpop_calls;
bench2_f addrsp0_calls;
bench2_f addrsp8_calls;
}




template <typename TIMER>
void register_call(GroupList& list) {
    std::shared_ptr<BenchmarkGroup> call_group = std::make_shared<BenchmarkGroup>("call", "Call/ret benchmarks");
    list.push_back(call_group);

    auto default_maker = DeltaMaker<TIMER>(call_group.get());

    default_maker.template make<sparse0_calls>    ("sparse0-call", "calls sparsed by 0",  16);
    default_maker.template make<sparse1_calls>    ("sparse1-call", "calls sparsed by 1",  16);
    default_maker.template make<sparse2_calls>    ("sparse2-call", "calls sparsed by 2",  16);
    default_maker.template make<sparse3_calls>    ("sparse3-call", "calls sparsed by 3",  16);
    default_maker.template make<sparse4_calls>    ("sparse4-call", "calls sparsed by 4",  16);
    default_maker.template make<sparse5_calls>    ("sparse5-call", "calls sparsed by 5",  16);
    default_maker.template make<sparse6_calls>    ("sparse6-call", "calls sparsed by 6",  16);
    default_maker.template make<sparse7_calls>    ("sparse7-call", "calls sparsed by 7",  16);

    default_maker.template make<chained0_calls>   ("chained0-call", "calls chained by 0",  16);
    default_maker.template make<chained1_calls>   ("chained1-call", "calls chained by 1",  16);
    default_maker.template make<chained2_calls>   ("chained2-call", "calls chained by 2",  16);
    default_maker.template make<chained3_calls>   ("chained3-call", "calls chained by 3",  16);


    default_maker.template make<pushpop_calls>    ("pushpop-call", "calls to pushpop fn",  16);
    default_maker.template make<addrsp0_calls>    ("addrsp0-call", "calls to addrsp0 fn",  16);
    default_maker.template make<addrsp8_calls>    ("addrsp8-call", "calls to addrsp8 fn",  16);
}

#define REGISTER(CLOCK) template void register_call<CLOCK>(GroupList& list);

ALL_TIMERS_X(REGISTER)



