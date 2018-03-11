/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benches.hpp"

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
    const BenchmarkGroup* group_ptr = call_group.get();

    using default_maker = BenchmarkMaker<TIMER>;

    auto benches = std::vector<Benchmark> {
        default_maker::template make_bench<sparse0_calls>    (group_ptr, "sparse0-call", "calls sparsed by 0",  16),
        default_maker::template make_bench<sparse1_calls>    (group_ptr, "sparse1-call", "calls sparsed by 1",  16),
        default_maker::template make_bench<sparse2_calls>    (group_ptr, "sparse2-call", "calls sparsed by 2",  16),
        default_maker::template make_bench<sparse3_calls>    (group_ptr, "sparse3-call", "calls sparsed by 3",  16),
        default_maker::template make_bench<sparse4_calls>    (group_ptr, "sparse4-call", "calls sparsed by 4",  16),
        default_maker::template make_bench<sparse5_calls>    (group_ptr, "sparse5-call", "calls sparsed by 5",  16),
        default_maker::template make_bench<sparse6_calls>    (group_ptr, "sparse6-call", "calls sparsed by 6",  16),
        default_maker::template make_bench<sparse7_calls>    (group_ptr, "sparse7-call", "calls sparsed by 7",  16),

        default_maker::template make_bench<chained0_calls>    (group_ptr, "chained0-call", "calls chained by 0",  16),
        default_maker::template make_bench<chained1_calls>    (group_ptr, "chained1-call", "calls chained by 1",  16),
        default_maker::template make_bench<chained2_calls>    (group_ptr, "chained2-call", "calls chained by 2",  16),
        default_maker::template make_bench<chained3_calls>    (group_ptr, "chained3-call", "calls chained by 3",  16),


        default_maker::template make_bench<pushpop_calls>    (group_ptr, "pushpop-call", "calls to pushpop fn",  16),
        default_maker::template make_bench<addrsp0_calls>     (group_ptr, "addrsp0-call", "calls to addrsp0 fn",  16),
        default_maker::template make_bench<addrsp8_calls>     (group_ptr, "addrsp8-call", "calls to addrsp8 fn",  16),
    };

    call_group->add(benches);
    list.push_back(call_group);
}

#define REGISTER(CLOCK) template void register_call<CLOCK>(GroupList& list);

ALL_TIMERS_X(REGISTER)



