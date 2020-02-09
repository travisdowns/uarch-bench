/*
 * benches.h
 *
 * Include this header in any file which defines benchmarks. It provides facilities to instantiate
 * benchmarks will all compiled-in timers.
 *
 * To have your benchmarks included, declare a register method here and call it in main.cpp.
 */

#ifndef BENCHMARK_HPP_
#define BENCHMARK_HPP_

#include <vector>
#include <memory>
#include <cassert>

#include "hedley.h"

#include "bench-declarations.h"
#include "timer-info.hpp"
#include "timers.hpp"
#include "context.hpp"
#include "isa-support.hpp"

#if defined(__GNUC__) && !defined(__clang__)
#define NO_STACK_PROTECTOR __attribute__((optimize("no-stack-protector")))
#else
#define NO_STACK_PROTECTOR
#endif


typedef std::function<void *()> arg_provider_t;

/** always provides the given value */
arg_provider_t constant(void *value);

extern const arg_provider_t null_provider;

/**
 * Empty bench2_f function that can be inlined, so it will generally be optimized away completely at
 * the call site. Useful mostly as a "no-op" function
 * in cases where you don't want to do anything (e.g,. as a default for TOUCH).
 */
bench2_f inlined_empty;
inline long inlined_empty(uint64_t iters, void *arg) {
    return 0;
}

class BenchmarkGroup;

typedef std::string tag_t;
typedef std::vector<tag_t>      taglist_t;
typedef std::vector<x86Feature> featurelist_t;

struct BenchArgs {
    const BenchmarkGroup* parent;
    std::string id;
    std::string description;
    taglist_t tags;
    featurelist_t features;
    /* how many operations are involved in one iteration of the benchmark loop */
    uint32_t ops_per_loop;

    BenchArgs(
            const BenchmarkGroup* parent,
            const std::string& id,
            const std::string& description,
            taglist_t tags,
            featurelist_t features,
            uint32_t ops_per_loop
            );
};

/** the max width for the description for any benchmark - longer will cause jagged tables */
constexpr int  DESC_WIDTH =  40;
/** the max width for the id for any benchmark - longer will cause jagged tables */
constexpr int  COLUMN_PAD =  3;

template <typename T>
// the width is either the expected max width of the value, or the with of the name, plus COLUMN_PAD
static void printOneMetric(Context &c, const T& metric) {
    unsigned int width = std::max(c.getTimerInfo().getMetricNames().size(), 4UL + c.getPrecision()) + COLUMN_PAD;
    c.out() << " " << std::setw(width) << metric;
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

    /** the short, command-line-friendly, ID for the benchmark */
    std::string getId() const { return args.id; }

    /** return the list of zero or more tags associated with the benchmark */
    taglist_t getTags() const { return args.tags; }

    /** return the list of zero or more required ISA features associated with the benchmark */
    featurelist_t getFeatures() const { return args.features; }

    /** the unique group to which this */
    const BenchmarkGroup& getGroup() const { return *args.parent; }

    /* Run the benchmark and return the normalized TimingResult.
     *
     * Some flavors of benchmark may not implement this and will throw an exception, e.g., those
     * that don't have a meaningful aggregated single result.
     */
    virtual TimingResult run(const TimerInfo& ti) = 0;

    /* Print the results to context - every benchmark should implement this */
    virtual void runAndPrintInner(Context& c) = 0;

    /* Print the results to context - does some generic logic such as checking if the ISA is supported
     * and then defers to runAndPrintInner which is implemented by the Benchmark class */
    void runAndPrint(Context& c);

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

inline predicate_t pred_and(const predicate_t& left, const predicate_t& right) {
    return [=](const Benchmark& b){ return left(b) && right(b); };
}


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
    /* human friendly plain text description, may have spaces and other command-line unfriendly chars */
    std::string desc;
    std::vector<Benchmark> benches_;

public:
    BenchmarkGroup(const std::string& id, const std::string& desc) : id{id}, desc{desc} {}

    virtual ~BenchmarkGroup() {}

    /** run benchmarks matching the predicate, using the given timer info and context */
    virtual void runIf(Context &context, const predicate_t& predicate);

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
};

using GroupList = std::vector<std::shared_ptr<BenchmarkGroup>>;

static inline TimingResult normalize(const TimingResult& result, const BenchArgs& args, size_t loop_count) {
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
    raw_f  raw_func;
    arg_provider_t arg_provider;

protected:

    raw_result get_raw() {
        void *arg = arg_provider();
        return raw_func(loop_count, arg);
    }

    TimingResult handle_raw(const raw_result& raw, const TimerInfo& ti) {
        TimingResult result = TIMER::to_result(static_cast<const TIMER &>(ti), ALGO::aggregate(raw));
        return normalize(result, BenchmarkBase::args, loop_count);
    }

public:

    BenchTemplate(BenchArgs args, size_t loop_count, raw_f raw_func, arg_provider_t arg_provider) : BenchmarkBase(std::move(args)),
    loop_count{loop_count}, raw_func{raw_func}, arg_provider{std::move(arg_provider)} {}


    virtual TimingResult run(const TimerInfo& ti) override {
        raw_result raw = get_raw();
        return handle_raw(raw, ti);
    }

    virtual void runAndPrintInner(Context& c) override {
        TimingResult result = run(c.getTimerInfo());
        printResultLine(c, this, result);
    }
};

template <typename DELTA_T, size_t SIZE>
struct DeltaRaw {
    using one_result = std::array<DELTA_T, SIZE>;
    one_result base, bench;
};

/**
 * The core benchmark loop, which performs samples calls to METHOD and returns an array of the timing for
 * each one.
 *
 * TIMER      - the TIMER to use for timing.
 * METHOD     - the method being timed
 * WARM_ONE   - an untimed warmup method called once before any timing takes place
 * WARM_EVERY - an untimed warmup method that is called before every sample is taken
 */
template <typename TIMER, int samples, bench2_f METHOD, bench2_f WARM_ONCE = inlined_empty, bench2_f WARM_EVERY = inlined_empty>
HEDLEY_NEVER_INLINE
NO_STACK_PROTECTOR
std::array<typename TIMER::delta_t,samples> time_one(size_t loop_count, void* arg) {
    WARM_ONCE(loop_count, arg);
    std::array<typename TIMER::delta_t,samples> result;
    for (int i = 0; i < samples; i++) {
        WARM_EVERY(loop_count, arg);
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

    using raw_f      = raw_result (*)(size_t loop_count, void *arg);

    template <bench2_f BENCH_METHOD>
    static raw_result delta_loop_bench(size_t loop_count, void *arg) {
        raw_result result;
        result.base  = time_one<TIMER, total_samples, BENCH_METHOD>(loop_count,     arg);
        result.bench = time_one<TIMER, total_samples, BENCH_METHOD>(loop_count * 2, arg);
        return result;
    }

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


template <typename TIMER, typename DERIVED>
class MakerBase {
protected:
    BenchmarkGroup* parent;
    uint32_t loop_count;
    taglist_t tags;
    featurelist_t features;

    MakerBase(BenchmarkGroup* parent, uint32_t loop_count) : parent{parent}, loop_count{loop_count}, tags{} {}

    template <typename ALGO>
    HEDLEY_NEVER_INLINE
    Benchmark make_bench_from_raw(
            const std::string& id,
            const std::string& description,
            uint32_t ops_per_loop,
            typename ALGO::raw_f raw_func,
            const arg_provider_t& arg_provider)
    {
        return new BenchTemplate<TIMER, ALGO>(make_args(id, description, ops_per_loop), loop_count, raw_func, arg_provider);
    }

    BenchArgs make_args(const std::string& id, const std::string& description, uint32_t ops_per_loop) {
        return {parent, id, description, tags, features, ops_per_loop};
    }

public:

    const BenchmarkGroup& getGroup() const {
        return *parent;
    }

    /* returns a COPY of this object with the given tags */
    DERIVED setTags(taglist_t tags) {
        DERIVED ret(*static_cast<DERIVED*>(this));
        ret.tags = std::move(tags);
        return ret;
    }

    /* return the configured loop count for this maker object */
    uint32_t getLoopCount() {
        return loop_count;
    }


    /* returns a COPY of this object with the given loop count */
    DERIVED setLoopCount(uint32_t loop_count) {
        DERIVED ret(*static_cast<DERIVED*>(this));
        ret.loop_count = loop_count;
        return ret;
    }

    /* return the set of configured features for this maker */
    featurelist_t getFeatures() {
        return features;
    }

    /* returns a COPY of this object has the given requires features list */
    DERIVED setFeatures(featurelist_t features) {
        DERIVED ret(*static_cast<DERIVED*>(this));
        ret.features = std::move(features);
        return ret;
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
template <typename TIMER, bool use_loop_delta = false>
class DeltaMaker : public MakerBase<TIMER, DeltaMaker<TIMER>> {
public:

    using this_t = DeltaMaker<TIMER>;
    using base_t = MakerBase<TIMER, DeltaMaker<TIMER>>;

    template <bool uld>
    DeltaMaker(const DeltaMaker<TIMER, uld>& rhs) : base_t{rhs} {
        // if DeltaMaker has state we need to fix the copy ctor, etc
        static_assert(sizeof(this_t) == sizeof(base_t), "DeltaMaker shouldn't have state");
    }

    DeltaMaker(BenchmarkGroup* parent, uint32_t loop_count = default_loop_count) :
        base_t(parent, loop_count) {}

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
        typename BenchTemplate<TIMER, DeltaAlgo<TIMER>>::raw_f f;
        if (use_loop_delta) {
            f = DeltaAlgo<TIMER>::template delta_loop_bench<BENCH_METHOD>;
        } else {
            f = DeltaAlgo<TIMER>::template delta_bench<BENCH_METHOD, BASE_METHOD>;
        }
        return base_t::template make_bench_from_raw<DeltaAlgo<TIMER>>(id, description, ops_per_loop, f, arg_provider);
    }

    /* If useLoopDelta is set to true, the benchmark results are calculated by running the benchmark twice, once with
     * loop_count iterations, and once with loop_count * 2 iterations, and taking the delta. This is as opposted to
     * the default which runs two different bench methods.
     */
    DeltaMaker<TIMER, true> useLoopDelta() {
        DeltaMaker<TIMER, true> ret(*this);
        return ret;
    }
};

template <typename TIMER>
class StaticMaker {
public:

    /**
     * Just a thin static wrapper around DeltaMaker<TIMER>(parent, loop_count).make(...).
     *
     * This doesn't support modern feartures like tags, so you should considering using DeltaMaker<> instead.
     *
     * You should consider replacing:
     *
     * using maker = StaticMaker<TIMER>;
     * group->add(maker::template make_bench<METHOD>(group.get(), ..., [])(){ arg provider lambda }, loop_count));
     *
     * with
     * auto maker = DeltaMaker<TIMER>(group.get(), loop_count);
     * maker.template make<METHOD>(...);  // remove group.get() and loop_count from arg list
     *
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

template <typename TIMER>
void register_syscall(GroupList& list);

template <typename TIMER>
void register_rstalls(GroupList& list);

void printResultHeader(Context& c, const TimerInfo& ti);



#endif /* BENCHMARK_HPP_ */
