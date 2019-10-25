/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benchmark.hpp"
#include "util.hpp"
#include "boost/preprocessor/repetition/repeat_from_to.hpp"

extern "C" {

bench2_f rs_dep_add;
bench2_f rs_dep_add4;
bench2_f rs_dep_imul;
bench2_f rs_split_stores;
bench2_f rs_dep_fsqrt;

#define MAX_RATIO 10
#define DECL_FSQRT_OP(op) BOOST_PP_REPEAT_FROM_TO(0, MAX_RATIO, DECL_BENCH2, rs_fsqrt_ ## op)

DECL_FSQRT_OP(nop)
DECL_FSQRT_OP(add)
DECL_FSQRT_OP(xorzero)
DECL_FSQRT_OP(load)
DECL_FSQRT_OP(store)
DECL_FSQRT_OP(paddb)
DECL_FSQRT_OP(vpaddb)
DECL_FSQRT_OP(add_padd)

}


template <typename TIMER>
void register_rstalls(GroupList& list) {
#if !UARCH_BENCH_PORTABLE

    std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("studies/resource-stalls", "Test RESOURCE_STALLS events");
    list.push_back(group);

    auto maker = DeltaMaker<TIMER>(group.get(), 1000);

    maker.template make<rs_dep_add     >("dep-add" ,    "Dependent adds (RS limit)",  128);
    maker.template make<rs_dep_add4    >("dep-add4",    "Inependent adds (? limit)",  128);
    maker.template make<rs_dep_imul    >("dep-imul",    "Dependent imuls (RS limit)", 128);
    maker.template make<rs_split_stores>("split-store", "Split stores (SB limit)",    128);
    maker.template make<rs_dep_fsqrt>   ("fsqrt",       "Dependent fqrt (RS limit)",  128);

    // a macro to call maker.make on test that mix fsqrt and antoher op in a variery of ratios
#define MAKE_FSQRT_OP(z, n, op) maker.template make<rs_fsqrt_ ## op ## n>("fsqrt-sqrt-" #op "-" #n, \
        "fqrt:" #op " in 1:" #n " ratio", 32);

    BOOST_PP_REPEAT_FROM_TO(0, MAX_RATIO, MAKE_FSQRT_OP, nop)
    BOOST_PP_REPEAT_FROM_TO(0, MAX_RATIO, MAKE_FSQRT_OP, add)
    BOOST_PP_REPEAT_FROM_TO(0, MAX_RATIO, MAKE_FSQRT_OP, xorzero)
    BOOST_PP_REPEAT_FROM_TO(0, MAX_RATIO, MAKE_FSQRT_OP, load)
    BOOST_PP_REPEAT_FROM_TO(0, MAX_RATIO, MAKE_FSQRT_OP, store)
    BOOST_PP_REPEAT_FROM_TO(0, MAX_RATIO, MAKE_FSQRT_OP, paddb)
    BOOST_PP_REPEAT_FROM_TO(0, MAX_RATIO, MAKE_FSQRT_OP, vpaddb)
    BOOST_PP_REPEAT_FROM_TO(0, MAX_RATIO, MAKE_FSQRT_OP, add_padd)


#endif // #if !UARCH_BENCH_PORTABLE

}

#define REG_THIS(CLOCK) template void register_rstalls<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_THIS)



