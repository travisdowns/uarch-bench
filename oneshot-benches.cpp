/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "oneshot.hpp"
#include "libpfc-timer.hpp"
#include "libpfc-raw-helpers.hpp"
#include "boost/preprocessor/repeat.hpp"

// how many consecutive load requests we make in the MLP tests
#define MLP_MAX 51

extern "C" {
bench2_f dep_add_rax_rax;
bench2_f indep_add;
bench2_f dep_add_noloop_128;
bench2_f dummy_bench_oneshot1;
bench2_f dummy_bench_oneshot2;
bench2_f dummy_bench_oneshot2_touch;
bench2_f oneshot2_touch;
BOOST_PP_REPEAT(MLP_MAX, DECL_BENCH2, mlp_load);
}

template <typename TIMER>
void register_specific(BenchmarkGroup* oneshot) {}

#if USE_LIBPFC

extern "C" {
    libpfc_raw1 store_raw_libpfc;
}

volatile int store_sink;

void oneshot_store_test(size_t loop_count, void *arg, LibpfcNow* results) {
    store_raw_libpfc(loop_count, nullptr, results);
}

template <>
void register_specific<LibpfcTimer>(BenchmarkGroup* oneshot) {
    constexpr int samples = 10;
    auto maker = OneshotMaker<LibpfcTimer, samples>(oneshot);

    maker.
    withOverhead(libpfc_raw_overhead_adapt<raw_rdpmc4_overhead>("raw_rdpmc4")).
    make_raw<libpfc_raw_adapt<samples, oneshot_store_test>>("raw-store", "raw store benchmark", 1);
}

#endif  // USE_LIBPFC

const size_t MLP_REGION_SIZE = 2 * 2048 * 2048;

bench2_f flush_warmer;

long flush_warmer(size_t iters, void *arg) {
    flush_region(arg, MLP_REGION_SIZE);
    return 0;
}

template <typename TIMER>
void register_oneshot(GroupList& list) {
    std::shared_ptr<BenchmarkGroup> oneshot = std::make_shared<OneshotGroup>("oneshot", "Oneshot Group");
    list.push_back(oneshot);

    auto maker = OneshotMaker<TIMER,20>(oneshot.get());

    maker.template make<dummy_bench>("oneshot-dummy", "Empty oneshot bench", 1);

    maker.template make<dep_add_rax_rax>  ("dep-add-oneshot", "Oneshot dep add chain",       128);
    maker.template make<indep_add>        ("indep-add-oneshot", "Oneshot indep add chain",  50 * 8);
    maker.template make<dep_add_noloop_128>("dep-add128", "128 dependent add instructions", 128);

    // order here is important: oneshot2 follows oneshot1 in the file, so if you run them in the other order
    // prefetching will apparently grab the line for oneshot1 as a result of executing oneshot1, so we do
    // it in the other order
    maker.template make<dummy_bench_oneshot1>("oneshot-dummy-notouch", "Empty untouched oneshot bench", 1);

    maker.template withWarmOnce<dummy_bench_oneshot2_touch>().template make<dummy_bench_oneshot2>("oneshot-dummy-touch",
            "Empty touched oneshot bench", 1);

#define MAKE_MLPLOAD(z, n, _) \
    maker.  template withWarmEvery<flush_warmer>().                \
            template make<mlp_load ## n>("mlp" #n, "mlp" #n, 1,         \
            [](){ return aligned_ptr(4096, MLP_REGION_SIZE); });   \

    BOOST_PP_REPEAT(MLP_MAX, MAKE_MLPLOAD, _);

    register_specific<TIMER>(oneshot.get());
}

#define REGISTER(CLOCK) template void register_oneshot<CLOCK>(GroupList& list);

ALL_TIMERS_X(REGISTER)



