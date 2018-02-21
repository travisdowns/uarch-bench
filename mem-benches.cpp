/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benches.hpp"
#include "oneshot.hpp"
#include "util.hpp"

#define LOAD_LOOP_UNROLL    8

#define BENCH_DECL_X(name) \
    bench2_f name ## 16;    \
    bench2_f name ## 32;    \
    bench2_f name ## 64;    \
    bench2_f name ## 128;   \
    bench2_f name ## 256;   \
    bench2_f name ## 512;   \
    bench2_f name ## 2048;  \

#define FWD_BENCH_DECL(delay) \
        bench2_f fwd_lat_delay_ ## delay ; \
        bench2_f fwd_lat_delay_oneshot_ ## delay ; \
        bench2_f fwd_tput_conc_ ## delay ;

extern "C" {
/* misc benches */
BENCH_DECL_X(load_loop)
BENCH_DECL_X(prefetcht0_bench)
BENCH_DECL_X(prefetcht1_bench)
BENCH_DECL_X(prefetcht2_bench)
BENCH_DECL_X(prefetchnta_bench)
BENCH_DECL_X(serial_load_bench)

FWD_BENCH_DECL(0);
FWD_BENCH_DECL(1);
FWD_BENCH_DECL(2);
FWD_BENCH_DECL(3);
FWD_BENCH_DECL(4);
FWD_BENCH_DECL(5);

bench2_f fwd_tput_conc_6;
bench2_f fwd_tput_conc_7;
bench2_f fwd_tput_conc_8;
bench2_f fwd_tput_conc_9;
bench2_f fwd_tput_conc_10;

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
}

template <typename TIMER, bench2_f FUNC>
static Benchmark make_load_bench(BenchmarkGroup* parent, size_t kib, const std::string &suffix) {
    using default_maker = StaticMaker<TIMER>;
    std::string id = suffix + "-" + std::to_string(kib);
    std::string name = std::to_string(kib) +  "-KiB " + suffix;
    return default_maker::template make_bench<FUNC>(parent, id, name, LOAD_LOOP_UNROLL,
                                    []{ return aligned_ptr(4096, 2048 * 1024); }, 100000);
}

#define MAKE_PARALLEL(kib) \
    make_load_bench<TIMER,load_loop ## kib>        (memload_group.get(), kib,      "parallel-loads"), \
    make_load_bench<TIMER,prefetcht0_bench  ## kib>(memload_group.get(), kib, "parallel-prefetcht0"), \
    make_load_bench<TIMER,prefetcht1_bench  ## kib>(memload_group.get(), kib, "parallel-prefetcht1"), \
    make_load_bench<TIMER,prefetcht2_bench  ## kib>(memload_group.get(), kib, "parallel-prefetcht2"), \
    make_load_bench<TIMER,prefetchnta_bench ## kib>(memload_group.get(), kib, "parallel-prefetchnta")

#define MAKE_SERIAL(kib) \
    make_load_bench<TIMER,serial_load_bench ## kib>        (memload_group.get(), kib,      "serial-loads")

template <typename TIMER>
void register_mem(GroupList& list) {
    {
        std::shared_ptr<BenchmarkGroup> memload_group = std::make_shared<BenchmarkGroup>("memory/load-parallel", "Parallel load/prefetches from fixed-size regions");
        list.push_back(memload_group);

        auto benches = std::vector<Benchmark> {
            MAKE_PARALLEL(16),
            MAKE_PARALLEL(32),
            MAKE_PARALLEL(64),
            MAKE_PARALLEL(128),
            MAKE_PARALLEL(256),
            MAKE_PARALLEL(512),
            MAKE_PARALLEL(2048)
        };

        memload_group->add(benches);
    }

    {
        std::shared_ptr<BenchmarkGroup> memload_group = std::make_shared<BenchmarkGroup>("memory/load-serial", "Serial load/prefetches from fixed-size regions");
        list.push_back(memload_group);

        auto benches = std::vector<Benchmark> {
            MAKE_SERIAL(16),
            MAKE_SERIAL(32),
            MAKE_SERIAL(64),
            MAKE_SERIAL(128),
            MAKE_SERIAL(256),
            MAKE_SERIAL(512),
            MAKE_SERIAL(2048)
        };

        memload_group->add(benches);
    }

    {
        std::shared_ptr<BenchmarkGroup> fwd_group = std::make_shared<BenchmarkGroup>("memory/store-fwd", "Store forwaring latency and throughput");

        using default_maker = StaticMaker<TIMER>;

#define LAT_DELAY_BENCH(delay) \
        default_maker::template make_bench<fwd_lat_delay_ ## delay>(fwd_group.get(), "latency-" #delay, \
                    "Store forward latency delay " #delay, 1)

#define TPUT_BENCH(conc) \
        default_maker::template make_bench<fwd_tput_conc_ ## conc>(fwd_group.get(), "concurrency-" #conc, \
                    "Store fwd tput concurrency " #conc, conc)


        auto benches = std::vector<Benchmark> {
            LAT_DELAY_BENCH(0),
            LAT_DELAY_BENCH(1),
            LAT_DELAY_BENCH(2),
            LAT_DELAY_BENCH(3),
            LAT_DELAY_BENCH(4),
            LAT_DELAY_BENCH(5),

            TPUT_BENCH(1),
            TPUT_BENCH(2),
            TPUT_BENCH(3),
            TPUT_BENCH(4),
            TPUT_BENCH(5),
            TPUT_BENCH(6),
            TPUT_BENCH(7),
            TPUT_BENCH(8),
            TPUT_BENCH(9),
            TPUT_BENCH(10)

        };

        fwd_group->add(benches);
        list.push_back(fwd_group);
    }

    auto buf_provider = []{ return aligned_ptr(4096, 10 * 4096); };

    {
        std::shared_ptr<BenchmarkGroup> oneshot = std::make_shared<OneshotGroup>("memory/store-fwd", "Store forwaring latency and throughput");
        list.push_back(oneshot);

        auto maker = OneshotMaker<TIMER, 20>(oneshot.get());

        maker.template make<dummy_bench>("oneshot-dummy", "Empty oneshot bench", 1);

#define LAT_DELAY_ONESHOT(delay) \
        maker.template make<fwd_lat_delay_oneshot_ ## delay>("oneshot-latency-" #delay, \
                    "StFwd oneshot lat (delay " #delay ")", 1, buf_provider)

        //LAT_DELAY_ONESHOT(5);
        //LAT_DELAY_ONESHOT(4);
        //LAT_DELAY_ONESHOT(3);
        LAT_DELAY_ONESHOT(2);
        LAT_DELAY_ONESHOT(1);
        LAT_DELAY_ONESHOT(0);
    }

    {
        std::shared_ptr<BenchmarkGroup> oneshot = std::make_shared<OneshotGroup>("memory/store-fwd-try", "Store forward attempts");
        list.push_back(oneshot);

        auto maker = OneshotMaker<TIMER, 20>(oneshot.get());

        maker.template make<dummy_bench>("oneshot-dummy", "Empty oneshot bench", 1);
        maker.template make<oneshot_try1>("stfwd-try1", "stfwd-try1", 1, buf_provider);
        maker.template make<oneshot_try2>("stfwd-try2", "stfwd-try2 100 loads", 1, buf_provider);
        maker.template make<oneshot_try2_4>("stfwd-try2-4", "stfwd-try2 4 loads", 1, buf_provider);
        maker.template make<oneshot_try2_10>("stfwd-try2-10", "stfwd-try2 10 loads", 1, buf_provider);
        maker.template make<oneshot_try2_20>("stfwd-try2-20", "stfwd-try2 20 loads", 1, buf_provider);
        maker.template make<oneshot_try2_1000>("stfwd-try2-1000", "stfwd-try2 1000 loads", 1, buf_provider);
        maker.template make_warm<oneshot_try2_1000,dummy_bench>("stfwd-try2-1000w", "stfwd-try2 1000 loads warm", 1, buf_provider);

        maker.template make<oneshot_try2>("stfwd-try2b", "stfwd-try2 100 loads", 1, buf_provider);
        maker.template make_warm<aliasing_loads, train_noalias>("stfwd-try2c", "trained loads", 1, buf_provider);

    }



}

#define REG_DEFAULT(CLOCK) template void register_mem<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



