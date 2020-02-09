/*
 * oneshot.hpp
 *
 * The implementation "one shot" type benchmarks. Rather than averaging over many samples (which internally
 * often contain a large loop), "one shot" benchmarks output results for every sample, including the first
 * and often contain no loops at all. The benefit is that you can observe the transient effects that occur
 * with "cold" code, and observe the sometimes gradual transition from cold to hot (e.g., the second sample
 * often isn't hot yet).
 *
 * The downside is that you inevitably deal with all the various factors that impact cold code - not just the
 * ones you want: you might want to see what happens with cold branch predictors, but you'll also usually
 * mix in the effect of a cold i-cache, cold stack engine, etc. Sometimes those effects are larger than
 * the effect you want to measure. You can sometimes avoid this by using a "TOUCH" method that warms up the
 * parts you want to, and leaves the rest code (but this may be tricky or impossible depending on what you
 * want). You can also look at performance counters for the thing you care about, rather than looking at just
 * cycle timings: this can isolate the effect you want directly without trying to back out the unrelated
 * effects from a general timing. For example, look at the branch-mispredict counters to isolate the number
 * of branch mispredictions.
 */

#ifndef ONESHOT_HPP_
#define ONESHOT_HPP_

#include "benchmark.hpp"

class OneshotGroup : public BenchmarkGroup {
public:
    OneshotGroup(const std::string& id, const std::string& desc) : BenchmarkGroup(id, desc) {}

    virtual void printGroupHeader(Context& c) override {}
};

template <typename TIMER, int samples>
struct OneshotAlgo {
    using delta_t    = typename TIMER::delta_t;
    using raw_result = std::array<delta_t, samples>;
    using raw_f =  raw_result(*)(size_t loop_count, void *arg);

    template <bench2_f METHOD, bench2_f WARM_ONCE, bench2_f WARM_EVERY>
    static raw_result bench(size_t loop_count, void *arg) {
        return time_one<TIMER, samples, METHOD, WARM_ONCE, WARM_EVERY>(loop_count, arg);
    }
};

/** number of ignored warmup samples for overhead adjustment */
constexpr static int ONESHOT_OVERHEAD_WARMUP =  3;
/** number of real samples used following the warmup, should be odd for simple median */
constexpr static int ONESHOT_OVERHEAD_REAL = 11;
constexpr static int ONESHOT_OVERHEAD_TOTAL = ONESHOT_OVERHEAD_WARMUP + ONESHOT_OVERHEAD_REAL;

template <typename TIMER>
static void printOne(Context& c, const std::string& name, const typename TIMER::delta_t& delta) {
    TimingResult result = TIMER::to_result(static_cast<const TIMER &>(c.getTimerInfo()), delta);
    printBenchName(c, std::string("Oneshot overhead ") + name);
    printAlignedMetrics(c, result.getResults());
    c.out() << std::endl;
}

/**
 * Take a method that can produce a raw result, and calculate the overhead based on the median
 * result. The first OVERHEAD_WARMUP results are skipped in the calculation.
 */
template <typename TIMER, typename F>
static typename TIMER::delta_t rawToOverhead(Context &c, F f, const std::string &name) {
    auto raw = f();

    c.out() << "\n---------- Oneshot calibration start (" << name << ") --------------\n";

    printResultHeader(c);

    assert(raw.size() > ONESHOT_OVERHEAD_WARMUP);
    auto b = std::begin(raw) + ONESHOT_OVERHEAD_WARMUP;
    auto e = std::end(raw);
    printOne<TIMER>(c, "min   ", TimerHelper<TIMER>::min(b,e));
    printOne<TIMER>(c, "median (used)", TimerHelper<TIMER>::median(b,e));
    printOne<TIMER>(c, "max   ", TimerHelper<TIMER>::max(b,e));

    c.out() << "---------- Oneshot calibration end   (" << name << ") --------------\n\n";
    return TimerHelper<TIMER>::median(b,e);
}

template <typename TIMER, int samples>
class OneshotBench : public BenchmarkBase {
public:
    using overhead_f = std::function<typename TIMER::delta_t(Context &c)>;

private:

    using ALGO       = OneshotAlgo<TIMER, samples>;
    using raw_result = typename ALGO::raw_result;
    using raw_f      = typename ALGO::raw_f;


//    using baseline   = typename TIMER::delta_t;
//    using baseline_f = void (*)(uint32_t, void*);

    uint32_t loop_count;
    raw_f raw_func;
    void*  func_addr;
    arg_provider_t arg_provider;
    overhead_f overhead_func;

    /*
     * If true, we try to subtract out the measurement overhead by subtracting from the measured metric
     * values the same metric from an empty benchmark.
     */
//    bool remove_overhead;

public:

    OneshotBench(
            BenchArgs args,
            uint32_t loop_count,
            raw_f raw_func,
            void* func_addr,
            arg_provider_t arg_provider,
            overhead_f overhead_func) :
                BenchmarkBase(std::move(args)),
                loop_count{loop_count},
                raw_func{raw_func},
                func_addr{func_addr},
                arg_provider{std::move(arg_provider)},
                overhead_func{overhead_func}
                {}


    virtual TimingResult run(const TimerInfo& ti) override {
        throw std::logic_error("oneshot doesn't do run()");
    }

    virtual void printHeader(Context& c) {
        c.out() << getId() << " @ 0x" << func_addr << std::endl;
        printNameHeader(c);
        printOneMetric(c, "Sample");
        printAlignedMetrics(c, c.getTimerInfo().getMetricNames());
        c.out() << std::endl;
    }

    template <typename M>
    void printOneSample(Context& c, const typename TIMER::delta_t& raw, const M& sampleNum) {
        const TimingResult& result = normalize(TIMER::to_result(static_cast<const TIMER &>(c.getTimerInfo()), raw),
                this->args, loop_count);
        printBenchName(c, this);
        printOneMetric(c, sampleNum);
        printAlignedMetrics(c, result.getResults());
        c.out() << std::endl;
    }

    virtual void runAndPrintInner(Context& c) override {
        void *arg = arg_provider();
        raw_result raw = raw_func(loop_count, arg);
        removeOverhead(c, raw);
        printHeader(c);
        for (int i = 0; i < samples; i++) {
            printOneSample(c, raw[i], i + 1);
        }

        // include median
        printOneSample(c, TimerHelper<TIMER>::median(std::begin(raw), std::end(raw)), "median");

        c.out() << std::endl;
    }

    /**
     * Turn a bench2_f method into an overhead calculator. This method will only calculate the
     * overhead once across the entire process for a given METHOD and passes 0 and nullptr for loops
     * and arg (since an overhead calculation that will be used generically shoudln't rely on
     * specific values from the first test that requires overhead compensation).
     */
    template <bench2_f METHOD>
    static typename TIMER::delta_t benchToOverhead_(Context &c, const std::string& name) {
        using O_ALGO = OneshotAlgo<TIMER, ONESHOT_OVERHEAD_TOTAL>;
        auto f = []() { return O_ALGO::template bench<METHOD, inlined_empty, inlined_empty>(0, nullptr); };
        static typename TIMER::delta_t overhead = rawToOverhead<TIMER>(c, f, name);
        return overhead;
    }

    template <bench2_f METHOD>
    static overhead_f benchToOverhead(const std::string& name) {
        return [name](Context &c) { return benchToOverhead_<METHOD>(c, name); };
    }

private:

    /**
     * Subtract out the overhead of an empty run.
     */
    void removeOverhead(Context& c, raw_result& results) {
        if (overhead_func) {
            typename TIMER::delta_t overhead = overhead_func(c);
            for (auto& raw : results) {
                raw = TIMER::delta(raw, overhead);
            }
        }
    }
};

/**
 * A factory for one-shot benchmarks, which run a method only a few times and report individual results (no aggregation)
 * for each run.
 */
template <typename TIMER, int SAMPLES = 10,
        bench2_f WARM_ONCE  = inlined_empty,
        bench2_f WARM_EVERY = inlined_empty>
class OneshotMaker : public MakerBase<TIMER, OneshotMaker<TIMER,SAMPLES,WARM_ONCE,WARM_EVERY>> {
public:
    using base_t = MakerBase<TIMER, OneshotMaker<TIMER,SAMPLES,WARM_ONCE,WARM_EVERY>>;
    using bench = OneshotBench<TIMER, SAMPLES>;
    using overhead_f = typename bench::overhead_f;

private:
    overhead_f overhead;

public:
    static constexpr int samples = SAMPLES;

    using ALGO = OneshotAlgo<TIMER, samples>;

    OneshotMaker(BenchmarkGroup* parent, uint32_t loop_count = 1, overhead_f overhead = bench::template benchToOverhead<dummy_bench>("default")) :
        base_t{parent, loop_count},
        overhead{overhead}
    {}


//    template <typename TIMER_, int SAMPLES_, bench2_f TOUCH_, bench2_f WARMX_>
//    OneshotMaker<TIMER, SAMPLES, TOUCH_, WARMX_> copy() { return {this->parent, overhead_func, this->loop_count}; }

    template <bench2_f NEW_TOUCH>
    OneshotMaker<TIMER, SAMPLES, NEW_TOUCH, WARM_EVERY> withTouch    () { return {this->parent, this->loop_count, this->overhead}; }

    template <bench2_f NEW_WARM>
    OneshotMaker<TIMER, SAMPLES, WARM_ONCE,   NEW_WARM> withWarm     () { return {this->parent, this->loop_count, this->overhead}; }

    OneshotMaker<TIMER, SAMPLES, WARM_ONCE, WARM_EVERY> withOverhead (overhead_f o) {
        return {this->parent, this->loop_count, o};
    }

    template <bench2_f NEW_OVERH>
    OneshotMaker<TIMER, SAMPLES, WARM_ONCE,     WARM_EVERY  > withOverhead (const std::string& name) {
        return withOverhead(bench::template benchToOverhead<NEW_OVERH>(name));
    }


    template <bench2_f METHOD>
    void make(
            const std::string& id,
            const std::string& description,
            uint32_t ops_per_invocation,
            const arg_provider_t& arg_provider = null_provider)
    {
        typename ALGO::raw_f f = ALGO::template bench<METHOD, WARM_ONCE, WARM_EVERY>;
        make2(this->make_args(id, description, ops_per_invocation), f, (void *)METHOD, arg_provider);
    }

    /**
     * This variant of make takes a raw METHOD which itself directly returns a OneShotAlgo::raw_f, rather
     * than the usual bench2_f. This means that METHOD itself is totally responsible for returning the
     * raw result that is normally generated by ALGO::bench. This is probably only possible when the enclosing
     * maker has already been specialized for a specific TIMER, since otherwise the definition of METHOD would
     * depend on TIMER::delta_t, since OneShotAlgo::raw_f depends on it.
     *
     * This will generally be used to create timer-coupled benchmarks which avoid the usually minor pitfalls of
     * generic benchmark methods.
     */
    template <typename ALGO::raw_f METHOD>
    void make_raw(
            const std::string& id,
            const std::string& description,
            uint32_t ops_per_invocation,
            const arg_provider_t& arg_provider = null_provider)
    {
        make2(this->make_args(id, description, ops_per_invocation), METHOD, (void *)METHOD, arg_provider);
    }


private:
    void make2(
            const BenchArgs& args,
            typename ALGO::raw_f f,
            void* f_addr,
            const arg_provider_t& arg_provider = null_provider)
    {
        Benchmark b = new OneshotBench<TIMER, samples>(args, this->loop_count, f, f_addr, arg_provider, overhead);
        this->parent->add(b);
    }
};





#endif /* ONESHOT_HPP_ */
