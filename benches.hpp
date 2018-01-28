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
#include <cassert>

#include "bench-declarations.h"
#include "timer-info.hpp"
#include "timers.hpp"
#include "context.hpp"

class BenchmarkGroup;

class Benchmark final {

    const BenchmarkGroup* parent;
    std::string id;
    std::string description;
    /* how many operations are involved in one iteration of the benchmark loop */
    size_t ops_per_loop_;
    full_bench_t full_bench_;
    uint32_t loop_count_;

protected:

    unsigned getLoopCount() { return loop_count_; }

public:

    static constexpr uint32_t default_loop_count = 10000;
    static constexpr int                 samples =    33;

    Benchmark(const BenchmarkGroup* parent, const std::string& id, const std::string& description,
            size_t ops_per_loop, full_bench_t full_bench, uint32_t loop_count);

    /** get the longer, human-readable descripton for the group */
    std::string getDescription() const { return description; }

    /** the short, command-line-friendly, ID for the group */
    std::string getId() const { return id; }

    /** the unique group to which this */
    const BenchmarkGroup& getGroup() const { return *parent; }

    /* get the raw timings for a full run of the underlying benchmark, doesn't normalize for loop_count or ops_per_loop */
    TimingResult getTimings();

    /* like getTimings, except that everything is normalized, so the results should reflect the cost for a single operation */
    TimingResult run();

    void runAndPrint(Context& c);

    /* the full "path" of the benchmark, which is the group id and the benchmark id, like group-name/bench-name */
    std::string getPath() const;

};

/* a predicate which can select a benchmark, given the fully qualified ID and Benchmark object */
using predicate_t = std::function<bool(const Benchmark&)>;

/**
 * Interface for a group of benchmarks. The group itself has a name, and can run and output all the contained
 * benchmarks.
 */
class BenchmarkGroup {
    /* the ID of this group, command-line friendly */
    std::string id;
    /* human friendly plain text description, may may have spaces and other command-line unfriendly chars */
    std::string desc;
    std::vector<Benchmark> benches_;

public:
    BenchmarkGroup(const std::string& id, const std::string& desc) : id{id}, desc{desc} {}

    virtual ~BenchmarkGroup() {}

    /** run benchmarks matching the predicate, using the given timer info and context */
    virtual void runIf(Context &context, const TimerInfo &ti, const predicate_t& predicate);

    virtual void add(const std::vector<Benchmark> &more) {
        for (auto &b : more) {
            add(b);
        }
    }

    void add(const Benchmark &bench) {
        assert(&bench.getGroup() == this);
        benches_.push_back(bench);
    }

    virtual const std::vector<Benchmark>& getBenches() const {
        return benches_;
    }

    const std::string& getDescription() const {
        return desc;
    }

    const std::string& getId() const {
        return id;
    }

    /**
     * Print benchmark descriptions for the contained benchmarks to the given output stream.
     */
    virtual void printBenches(std::ostream& out) const;

    /**
     * Print a single benchmark to the given output stream.
     */
    static void printBench(std::ostream& out, const Benchmark& bench);
};

using GroupList = std::vector<std::shared_ptr<BenchmarkGroup>>;

template <typename TIMER>
class BenchmarkMaker {
public:
    /**
     * Make a benchmark with the given parameters - this measures absolute time of the single
     * BENCH_METHOD without a mechanism for removing constant overhead.
     */
    template <bench2_f BENCH_METHOD>
    static Benchmark make_bench(
            const BenchmarkGroup* parent,
            const std::string& id,
            const std::string& description,
            size_t ops_per_loop,
            std::function<void * ()> arg_provider = []{ return nullptr; },
            uint32_t loop_count = Benchmark::default_loop_count) {
        TimingAbsolute<TIMER,BENCH_METHOD> timing(arg_provider);
        return Benchmark{parent, id, description, ops_per_loop, TIMER::make_bench_method(timing), loop_count};
    }

    /**
     * Makes a benchmark with the given BASE_METHOD and BENCH_METHOD. The effective time is the difference
     * between BASE_METHOD and BENCH_METHOD - essentially we are timing only the difference between the
     * two methods. This is useful when the core work of a benchmark includes overhead that you don't want
     * to include (i.e., it is possible to create "sparse" benchmarks where the code under test is surrounded
     * by code that shouldn't contribute to the result).
     */
    template <bench2_f BASE_METHOD, bench2_f BENCH_METHOD>
    static Benchmark make_bench(
            const BenchmarkGroup* parent,
            const std::string& id,
            const std::string& description,
            size_t ops_per_loop,
            std::function<void * ()> arg_provider = []{ return nullptr; },
            uint32_t loop_count = Benchmark::default_loop_count) {
        TimingAbsolute<TIMER, BASE_METHOD>   base(arg_provider);
        TimingAbsolute<TIMER,BENCH_METHOD> timing(arg_provider);
        return Benchmark{parent, id, description, ops_per_loop, TIMER::make_delta_method(base, timing), loop_count};
    }
};

template <typename TIMER>
void register_loadstore(GroupList& list);

template <typename TIMER>
void register_default(GroupList& list);

template <typename TIMER>
void register_misc(GroupList& list);

template <typename TIMER>
void register_mem(GroupList& list);

template <typename TIMER>
void register_cpp(GroupList& list);

void printResultHeader(Context& c, const TimerInfo& ti);



#endif /* BENCHES_HPP_ */
