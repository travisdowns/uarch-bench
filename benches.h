/*
 * benches.h
 *
 * Include this header in any file which defines benchmarks. It provides facilities to instantiate
 * benchmarks will all compiled-in timers.
 *
 * To have your benchmarks included, declare a register method here and call it in main.cpp.
 */

#ifndef BENCHES_H_
#define BENCHES_H_

#include <vector>
#include <memory>

#include "timer-info.hpp"
#include "timers.hpp"
#include "context.h"
#include "bench-declarations.hpp"

typedef std::function<int64_t (size_t)> time_method_t;  // given a loop count, returns a raw timing result
typedef TimingResult (time_to_result_t)(int64_t);


class Benchmark final {
    static constexpr int loop_count = 1000;
    static constexpr int    samples = 33;

    std::string name_;
    /* how many operations are involved in one iteration of the benchmark loop */
    size_t ops_per_loop_;
    time_method_t bench_method_;
    time_to_result_t *time_to_result_;

protected:
    time_method_t getBench() const {
        return bench_method_;
    }

public:
    Benchmark(std::string name, size_t ops_per_loop, time_method_t bench_method, time_to_result_t *time_to_result) :
        name_(name), ops_per_loop_(ops_per_loop), bench_method_(bench_method), time_to_result_(time_to_result) {}

    std::string getName() const {
        return name_;
    }

    /* get the raw timings for a full run of the underlying benchmark, doesn't normalize for loop_count or ops_per_loop */
    TimingResult getTimings();

    /* like getTimings, except that everything is normalized, so the results should reflect the cost for a single operation */
    TimingResult run();

    void runAndPrint(Context& c);
};

/**
 * Interface for a group of benchmarks. The group itself has a name, and can run and output all the contained
 * benchmarks.
 */
class BenchmarkGroup {
    std::string name_;
    std::vector<Benchmark> benches_;

public:
    BenchmarkGroup(std::string name) : name_(name) {}

    virtual ~BenchmarkGroup() {}

    virtual void runAll(Context &context, const TimerInfo &ti);

    virtual void add(const std::vector<Benchmark> &more) {
        benches_.insert(benches_.end(), more.begin(), more.end());
    }

    virtual void add(const Benchmark &bench) {
        benches_.push_back(bench);
    }

    virtual const std::vector<Benchmark>& getAllBenches() const {
        return benches_;
    }

    const std::vector<Benchmark>& getBenches() const {
        return benches_;
    }

    const std::string& getName() const {
        return name_;
    }
};

using BenchmarkList = std::vector<std::shared_ptr<BenchmarkGroup>>;

template <template<typename> class TIME_METHOD, typename TIMER>
class BenchmarkMaker {
public:
    template <bench_f BENCH_METHOD>
    static Benchmark make_bench(const char *name, size_t ops_per_loop) {
        return Benchmark{name, ops_per_loop, TIME_METHOD<TIMER>::template time_method<BENCH_METHOD>, TIMER::to_result};
    }

    template <bench2_f BENCH_METHOD>
    static Benchmark make_bench(const std::string name, size_t ops_per_loop, std::function<void * ()> arg_provider) {
        Timing2<TIMER,BENCH_METHOD> timing(arg_provider);
        return Benchmark{name, ops_per_loop, timing, TIMER::to_result};
    }
};


template <typename TIMER>
void register_loadstore(BenchmarkList& list);

template <typename TIMER>
void register_default(BenchmarkList& list);

#endif /* BENCHES_H_ */
