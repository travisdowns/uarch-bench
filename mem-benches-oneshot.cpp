/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "oneshot.hpp"
#include "util.hpp"
#include "libpfc-raw-helpers.hpp"

#include <random>
#include "benchmark.hpp"

extern "C" {

bench2_f oneshot_try1;
bench2_f oneshot_try2;
bench2_f oneshot_try2_4;
bench2_f oneshot_try2_10;
bench2_f oneshot_try2_20;
bench2_f oneshot_try2_1000;
bench2_f oneshot_try3;
bench2_f oneshot_try4;

bench2_f train_noalias;
bench2_f aliasing_loads;

#if USE_LIBPFC
libpfc_raw1 aliasing_loads_raw;
libpfc_raw1 mixed_loads_raw;
libpfc_raw1 aliasing_loads_raw2;
#endif
}

#define FWD_BENCH_DECL(delay) \
        bench2_f fwd_lat_delay_oneshot_ ## delay ;

extern "C" {
FWD_BENCH_DECL(0);
FWD_BENCH_DECL(1);
FWD_BENCH_DECL(2);
FWD_BENCH_DECL(3);
FWD_BENCH_DECL(4);
FWD_BENCH_DECL(5);
}

auto buf_provider = []{ return aligned_ptr(4096, 10 * 4096); };

template <typename TIMER>
void register_specific_stfwd(BenchmarkGroup* oneshot) {}

#if USE_LIBPFC
template <>
void register_specific_stfwd<LibpfcTimer>(BenchmarkGroup* oneshot) {
    auto maker = OneshotMaker<LibpfcTimer, 20>(oneshot, 1);

    maker.
            withOverhead(libpfc_raw_overhead_adapt<raw_rdpmc4_overhead>("raw_rdpmc4")).
            make_raw<libpfc_raw_adapt<20, aliasing_loads_raw>>("stfwd-raw-trained", "trained loads raw", 1, buf_provider);

    maker.
            withOverhead(libpfc_raw_overhead_adapt<raw_rdpmc4_overhead>("raw_rdpmc4")).
            make_raw<libpfc_raw_adapt<20, mixed_loads_raw>>("stfwd-raw-mixed", "mixed aliasing raw", 1, buf_provider);
}
#endif // USE_LIBPFC

template <typename TIMER>
void register_mem_oneshot(GroupList& list) {

    {
        std::shared_ptr<BenchmarkGroup> oneshot = std::make_shared<OneshotGroup>("memory/store-fwd-oneshot", "Store forwaring latency and throughput");
        list.push_back(oneshot);

        auto maker = OneshotMaker<TIMER, 20>(oneshot.get()).setTags({"noisy"});

#define LAT_DELAY_ONESHOT(delay) \
        maker.template make<fwd_lat_delay_oneshot_ ## delay>("oneshot-latency-" #delay, \
                    "StFwd oneshot lat (delay " #delay ")", 1, buf_provider)

        LAT_DELAY_ONESHOT(5);
        LAT_DELAY_ONESHOT(4);
        LAT_DELAY_ONESHOT(3);
        LAT_DELAY_ONESHOT(2);
        LAT_DELAY_ONESHOT(1);
        LAT_DELAY_ONESHOT(0);
    }

    {
        std::shared_ptr<BenchmarkGroup> stfwd = std::make_shared<OneshotGroup>("studies/store-fwd-try", "Store forward attempts");
        list.push_back(stfwd);

        auto maker = OneshotMaker<TIMER, 20>(stfwd.get());

        maker.template make<dummy_bench>("oneshot-dummy", "Empty oneshot bench", 1);
        maker = maker.setTags({"noisy"});
        maker.template make<oneshot_try1>("stfwd-try1", "stfwd-try1", 1, buf_provider);
        maker.template make<oneshot_try2>("stfwd-try2", "stfwd-try2 100 loads", 1, buf_provider);
        maker.template make<oneshot_try2_4>("stfwd-try2-4", "stfwd-try2 4 loads", 1, buf_provider);
        maker.template make<oneshot_try2_10>("stfwd-try2-10", "stfwd-try2 10 loads", 1, buf_provider);
        maker.template make<oneshot_try2_20>("stfwd-try2-20", "stfwd-try2 20 loads", 1, buf_provider);
        maker.template make<oneshot_try2_1000>("stfwd-try2-1000", "stfwd-try2 1000 loads", 1, buf_provider);
        maker.template withWarm<dummy_bench>().
              template make<oneshot_try2_1000>("stfwd-try2-1000w", "stfwd-try2 1000 loads warm", 1, buf_provider);

        maker.template make<oneshot_try2>("stfwd-try2b", "stfwd-try2 100 loads", 1, buf_provider);

        maker.  template withWarm<train_noalias>().
                template make<aliasing_loads>("stfwd-try2c-trained", "trained loads", 1, buf_provider);
        maker.  /* template withWarm<train_noalias>(). */
                template make<aliasing_loads>("stfwd-try2c-untrained", "untrained loads", 1, buf_provider);

        register_specific_stfwd<TIMER>(stfwd.get());

    }


}

#define REG_DEFAULT(CLOCK) template void register_mem_oneshot<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



