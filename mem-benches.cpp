/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benches.hpp"
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

extern "C" {
/* misc benches */
BENCH_DECL_X(load_loop)
BENCH_DECL_X(prefetcht0_bench)
BENCH_DECL_X(prefetcht1_bench)
BENCH_DECL_X(prefetcht2_bench)
BENCH_DECL_X(prefetchnta_bench)
BENCH_DECL_X(serial_load_bench)

bench2_f fwd_lat_delay_0;
bench2_f fwd_lat_delay_1;
bench2_f fwd_lat_delay_2;
bench2_f fwd_lat_delay_3;
bench2_f fwd_lat_delay_4;
bench2_f fwd_lat_delay_5;

bench2_f fwd_tput_conc_1;
bench2_f fwd_tput_conc_2;
bench2_f fwd_tput_conc_3;
bench2_f fwd_tput_conc_4;
bench2_f fwd_tput_conc_5;
bench2_f fwd_tput_conc_6;
bench2_f fwd_tput_conc_7;
bench2_f fwd_tput_conc_8;
bench2_f fwd_tput_conc_9;
bench2_f fwd_tput_conc_10;
}

template <typename TIMER, bench2_f FUNC>
static Benchmark make_load_bench(const BenchmarkGroup* parent, size_t kib, const std::string &suffix) {
    using default_maker = BenchmarkMaker<TIMER>;
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

        using default_maker = BenchmarkMaker<TIMER>;

#define LAT_DELAY_BENCH(delay) \
        default_maker::template make_bench<dummy_bench, fwd_lat_delay_ ## delay >(fwd_group.get(), "latency-" #delay, \
                    "Store forward latency delay " #delay, 1, []{ return nullptr; /*aligned_ptr(4096, 2048 * 1024);*/ })

#define TPUT_BENCH(conc) \
        default_maker::template make_bench<dummy_bench, fwd_tput_conc_ ## conc >(fwd_group.get(), "concurrency-" #conc, \
                    "Store fwd tput concurrency " #conc, conc, []{ return nullptr; /*aligned_ptr(4096, 2048 * 1024);*/ })


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


}

#define REG_DEFAULT(CLOCK) template void register_mem<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



