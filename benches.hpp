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

#include "hedley.h"

#include "bench-declarations.h"
#include "timer-info.hpp"
#include "timers.hpp"
#include "context.hpp"

typedef std::function<void *()> arg_provider_t;

extern const arg_provider_t null_provider;

class BenchmarkGroup;

struct BenchArgs {
    const BenchmarkGroup* parent;
    std::string id;
    std::string description;
    /* how many operations are involved in one iteration of the benchmark loop */
    uint32_t ops_per_loop;

    BenchArgs(const BenchmarkGroup* parent, const std::string& id, const std::string& description, uint32_t ops_per_loop) :
    parent{parent},
    id{id},
    description{description},
    ops_per_loop{ops_per_loop}
    {}
};

constexpr int  NAME_WIDTH =  30;
constexpr int  COLUMN_PAD =  3;

template <typename T>
// the width is either the expected max width of the value, or the with of the name, plus COLUMN_PAD
static void printOneMetric(Context &c, const T& metric) {
    unsigned int width = std::max(c.getTimerInfo().getMetricNames().size(), 4UL + c.getPrecision()) + COLUMN_PAD;
    c.out() << std::setw(width) << metric;
}

template <typename T>
static void printAlignedMetrics(Context &c, const std::vector<T>& metrics) {
    assert(c.getTimerInfo().getMetricNames().size() == metrics.size());
    for (auto& metric : metrics) {
        printOneMetric(c, metric);
    }
}

/**
 * Print the Name column to context, usually the first column of the result header.
 */
void printNameHeader(Context& c);
/**
 * Print the entire header: name metric1 metric2 ... for the TimerInfo associated with the given context.
 */
void printResultHeader(Context& c);

class BenchmarkBase {
protected:

    BenchArgs args;

    BenchmarkBase(BenchArgs args);

    BenchArgs& getArgs() {
        return args;
    }

public:

    /** get the longer, human-readable descripton for the group */
    std::string getDescription() const { return args.description; }

    /** the short, command-line-friendly, ID for the group */
    std::string getId() const { return args.id; }

    /** the unique group to which this */
    const BenchmarkGroup& getGroup() const { return *args.parent; }

    /* Run the benchmark and return the normalized TimingResult.
     *
     * Some flavors of benchmark may not implement this and will throw an exception, e.g., those
     * that don't have a meaningful aggregated single result.
     */
    virtual TimingResult run() = 0;

    /* Print the results to context - every benchmark should implement this */
    virtual void runAndPrint(Context& c) = 0;

    /* the full "path" of the benchmark, which is the group id and the benchmark id, like group-name/bench-name */
    std::string getPath() const;

    virtual ~BenchmarkBase() = default;
};

/*
 * In the past Benchmark was declared as std::shared_ptr<BenchmarkBase>, but in practice these are singleton objects
 * that live the entire duration of the process, so let's just use dumb pointers and not free them. This removes
 * a bunch of generated code from the benchmark registration functions, and slightly improves compile times,
 * among other things.
 */
using Benchmark = BenchmarkBase *;

/* a predicate which can select a benchmark, given the fully qualified ID and Benchmark object */
using predicate_t = std::function<bool(const Benchmark&)>;


void printBenchName(Context& c, const std::string& name);
void printBenchName(Context& c, const Benchmark& b);
void printResultLine(Context& c, const Benchmark& b, const TimingResult& result);

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
        assert(&bench->getGroup() == this);
        assert(std::find_if(benches_.begin(), benches_.end(),
                [&bench](const Benchmark& o){ return bench->getId() == o->getId(); }) == benches_.end()); // duplicate ID
        benches_.push_back(bench);
    }

    virtual const std::vector<Benchmark>& getBenches() const {
        return benches_;
    }

    /**
     * Print the header for the tests in this group - override it if you don't use the default layout.
     */
    virtual void printGroupHeader(Context& c) {
        printResultHeader(c);
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
     * Print a single benchmark description to the given output stream.
     */
    static void printBench(std::ostream& out, const Benchmark& bench);
};

using GroupList = std::vector<std::shared_ptr<BenchmarkGroup>>;

static TimingResult normalize(const TimingResult& result, const BenchArgs& args, size_t loop_count) {
    return result * (1.0 / ((uint64_t)loop_count * args.ops_per_loop));
}

/**
 * By providing an appropriate traits class ALGO, you can generate a benchmark implementation using
 * this template.
 */
template <typename TIMER, typename ALGO>
class BenchTemplate : public BenchmarkBase {
public:
    using raw_f      = typename ALGO::raw_f;

protected:
    using raw_result = typename ALGO::raw_result;

    size_t loop_count;
    raw_f  *raw_func;
    arg_provider_t arg_provider;

protected:

    raw_result get_raw() {
        void *arg = arg_provider();
        return raw_func(loop_count, arg);
    }

    TimingResult handle_raw(const raw_result& raw) {
        TimingResult result = TIMER::to_result(ALGO::aggregate(raw));
        // normalize to time per op
        return normalize(result, BenchmarkBase::args, loop_count);
    }

public:

    BenchTemplate(BenchArgs args, size_t loop_count, raw_f *raw_func, arg_provider_t arg_provider) : BenchmarkBase(std::move(args)),
    loop_count{loop_count}, raw_func{raw_func}, arg_provider{arg_provider} {}


    virtual TimingResult run() override {
        raw_result raw = get_raw();
        return handle_raw(raw);
    }

    virtual void runAndPrint(Context& c) override {
        TimingResult result = run();
        printResultLine(c, this, result);
    }
};

template <typename DELTA_T, size_t SIZE>
struct DeltaRaw {
    using one_result = std::array<DELTA_T, SIZE>;
    one_result base, bench;
};

template <typename TIMER, int samples, bench2_f METHOD>
HEDLEY_NEVER_INLINE
__attribute__((optimize("no-stack-protector")))
std::array<typename TIMER::delta_t,samples> time_one(size_t loop_count, void* arg) {
    std::array<typename TIMER::delta_t,samples> result;
    for (int i = 0; i < samples; i++) {
        auto t0 = TIMER::now();
        METHOD(loop_count, arg);
        auto t1 = TIMER::now();
        result[i] = TIMER::delta(t1, t0);
    }
    return result;
}

/** just like time_one, except that it runs an untimed WARMUP method before each METHOD invocation */
template <typename TIMER, int samples, bench2_f METHOD, bench2_f WARMUP>
HEDLEY_NEVER_INLINE
__attribute__((optimize("no-stack-protector")))
std::array<typename TIMER::delta_t,samples> time_one_warm(size_t loop_count, void* arg) {
    std::array<typename TIMER::delta_t,samples> result;
    for (int i = 0; i < samples; i++) {
        WARMUP(loop_count, arg);
        auto t0 = TIMER::now();
        METHOD(loop_count, arg);
        auto t1 = TIMER::now();
        result[i] = TIMER::delta(t1, t0);
    }
    return result;
}

template <typename TIMER>
struct DeltaAlgo {
    static constexpr int warmup_samples =  2;
    static constexpr int total_samples  = 35;

    static_assert(warmup_samples < total_samples, "warmup samples must be less than total");

    using delta_t    = typename TIMER::delta_t;
    using raw_result = DeltaRaw<delta_t, total_samples>;
    using one_result = typename raw_result::one_result;

    typedef raw_result (raw_f)(size_t loop_count, void *arg);


    template <bench2_f BENCH_METHOD, bench2_f BASE_METHOD>
    static raw_result delta_bench(size_t loop_count, void *arg) {
        raw_result result;
        result.base  = time_one<TIMER, total_samples, BASE_METHOD> (loop_count, arg);
        result.bench = time_one<TIMER, total_samples, BENCH_METHOD>(loop_count, arg);
        return result;
    }

    static typename TIMER::delta_t aggregate_one(const one_result& result) {
        // For now just choose the minimum element on the idea that there will be slower deviations from
        // the true speed (e.g., cache misses, interrupts), but no negative deviations (how can the CPU
        // run faster than idea?). For more complex cases this assumption doesn't always hold.
        return TimerHelper<TIMER>::min(result.begin() + warmup_samples, result.end());
    }

    static typename TIMER::delta_t aggregate(const raw_result& result) {
        return TIMER::delta(aggregate_one(result.bench), aggregate_one(result.base));
    }
};


template <typename TIMER>
class MakerBase {
protected:
    BenchmarkGroup* parent;
    uint32_t loop_count;

    MakerBase(BenchmarkGroup* parent, uint32_t loop_count) : parent{parent}, loop_count{loop_count} {}

    template <typename ALGO>
    HEDLEY_NEVER_INLINE
    Benchmark make_bench_from_raw(
            const std::string& id,
            const std::string& description,
            uint32_t ops_per_loop,
            typename ALGO::raw_f raw_func,
            const arg_provider_t& arg_provider)
    {
        BenchArgs args(parent, id, description, ops_per_loop);
        return new BenchTemplate<TIMER, ALGO>(args, loop_count, raw_func, arg_provider);
    }
};

/**
 * A factory for delta benchmarks.
 *
 * The effective time is the difference
 * between BASE_METHOD and BENCH_METHOD - essentially we are timing only the difference between the
 * two methods. This is useful when the core work of a benchmark includes overhead that you don't want
 * to include (i.e., it is possible to create "sparse" benchmarks where the code under test is surrounded
 * by code that shouldn't contribute to the result).
 */
template <typename TIMER>
class DeltaMaker : public MakerBase<TIMER> {
public:

    DeltaMaker(BenchmarkGroup* parent, uint32_t loop_count = default_loop_count) : MakerBase<TIMER>(parent, loop_count) {}

    static constexpr uint32_t default_loop_count = 10000;
    static constexpr int                 samples =    33;

    /**
     * Makes a benchmark with the given BASE_METHOD and BENCH_METHOD, and adds it to the group associated with
     * this maker object.
     */
    template <bench2_f BENCH_METHOD, bench2_f BASE_METHOD = dummy_bench>
    void make(
                const std::string& id,
                const std::string& description,
                uint32_t ops_per_loop,
                const arg_provider_t& arg_provider = null_provider)
    {
        Benchmark b = make_only<BENCH_METHOD, BASE_METHOD>(id, description, ops_per_loop, arg_provider);
        this->parent->add(b);
    }

    template <bench2_f BENCH_METHOD, bench2_f BASE_METHOD = dummy_bench>
    Benchmark make_only(
            const std::string& id,
            const std::string& description,
            uint32_t ops_per_loop,
            const arg_provider_t& arg_provider = null_provider)
    {
        typename BenchTemplate<TIMER, DeltaAlgo<TIMER>>::template raw_f *f = DeltaAlgo<TIMER>::template delta_bench<BENCH_METHOD, BASE_METHOD>;
        return MakerBase<TIMER>::template make_bench_from_raw<DeltaAlgo<TIMER>>(id, description, ops_per_loop, f, arg_provider);
    }
};

template <typename TIMER>
class StaticMaker {
public:

    /**
     * Just a thin static wrapper around DeltaMaker<TIMER>(parent, loop_count).make(...).
     */
    template <bench2_f BENCH_METHOD, bench2_f BASE_METHOD = dummy_bench>
    static Benchmark make_bench(
            BenchmarkGroup* parent,
            const std::string& id,
            const std::string& description,
            uint32_t ops_per_loop,
            const arg_provider_t& arg_provider = null_provider,
            uint32_t loop_count = DeltaMaker<TIMER>::default_loop_count) {
        auto maker = DeltaMaker<TIMER>(parent, loop_count);
        return maker.template make_only<BENCH_METHOD, BASE_METHOD>(id, description, ops_per_loop, arg_provider);
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

template <typename TIMER>
void register_vector(GroupList& list);

template <typename TIMER>
void register_call(GroupList& list);

template <typename TIMER>
void register_oneshot(GroupList& list);

void printResultHeader(Context& c, const TimerInfo& ti);



#endif /* BENCHES_HPP_ */
