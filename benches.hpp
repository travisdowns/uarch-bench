/*
 * benches.h
 *
 * Include this header in any file which defines benchmarks. It provides facilities to instantiate
 * benchmarks will all compiled-in timers.
 *
 * To have your benchmarks included, declare a register method here and call it in main.cpp.
 */

#ifndef BENCHES_HPP_
#define BENCHES_HPP_

#include <vector>
#include <memory>

#include "bench-declarations.h"
#include "timer-info.hpp"
#include "timers.hpp"
#include "context.hpp"

class Benchmark final {
    static constexpr int loop_count = 1000;
    static constexpr int    samples = 33;

    std::string name_;
    /* how many operations are involved in one iteration of the benchmark loop */
    size_t ops_per_loop_;
    full_bench_t full_bench_;

protected:


public:
    Benchmark(const std::string& name, size_t ops_per_loop, full_bench_t full_bench);

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
    BenchmarkGroup(const std::string& name) : name_(name) {}

    virtual ~BenchmarkGroup() {}

    virtual void runAll(Context &context, const TimerInfo &ti);

    virtual void add(const std::vector<Benchmark> &more) {
        benches_.insert(benches_.end(), more.begin(), more.end());
    }

    void add(const Benchmark &bench) {
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

template <typename TIMER>
class BenchmarkMaker {
public:
    template <bench2_f BENCH_METHOD>
    static Benchmark make_bench(const std::string& name, size_t ops_per_loop,
            std::function<void * ()> arg_provider = []{ return nullptr; }) {
        Timing2<TIMER,BENCH_METHOD> timing(arg_provider);
        return Benchmark{name, ops_per_loop, TIMER::make_bench_method(timing)};
    }
};

template <typename TIMER>
void register_loadstore(BenchmarkList& list);

template <typename TIMER>
void register_default(BenchmarkList& list);

#endif /* BENCHES_HPP_ */
