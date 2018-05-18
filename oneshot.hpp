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

#include "benches.hpp"

class OneshotGroup : public BenchmarkGroup {
public:
    OneshotGroup(const std::string& id, const std::string& desc) : BenchmarkGroup(id, desc) {}

    virtual void printGroupHeader(Context& c) override {}
};

template <typename TIMER, int samples>
struct OneshotAlgo {
    using delta_t    = typename TIMER::delta_t;
    using raw_result = std::array<delta_t, samples>;
    using raw_f =  raw_result (size_t loop_count, void *arg);


    template <bench2_f METHOD>
    static raw_result bench(size_t loop_count, void *arg) {
        return time_one<TIMER, samples, METHOD>(loop_count, arg);
    }

    template <bench2_f METHOD, bench2_f TOUCH>
    static raw_result bench(size_t loop_count, void *arg) {
        TOUCH(0, nullptr);
        return bench<METHOD>(loop_count, arg);
    }

    template <bench2_f METHOD, bench2_f WARMUP>
    static raw_result bench_warm(size_t loop_count, void *arg) {
        return time_one_warm<TIMER, samples, METHOD, WARMUP>(loop_count, arg);
    }
};

template <typename TIMER, int samples>
class OneshotBench : public BenchmarkBase {
public:
    using overhead_f = typename TIMER::delta_t(Context &c);

private:

    using ALGO       = OneshotAlgo<TIMER, samples>;
    using raw_result = typename ALGO::raw_result;
    using raw_f      = typename ALGO::raw_f;


//    using baseline   = typename TIMER::delta_t;
//    using baseline_f = void (*)(uint32_t, void*);

    uint32_t loop_count;
    raw_f* raw_func;
    void*  func_addr;
    arg_provider_t arg_provider;
    overhead_f* overhead_func;

    /*
     * If true, we try to subtract out the measurement overhead by subtracting from the measured metric
     * values the same metric from an empty benchmark.
     */
//    bool remove_overhead;

public:

    OneshotBench(
            BenchArgs args,
            uint32_t loop_count,
            raw_f* raw_func,
            void* func_addr,
            arg_provider_t arg_provider,
            overhead_f* overhead_func = defaultGetOverhead) :
                BenchmarkBase(std::move(args)),
                loop_count{loop_count},
                raw_func{raw_func},
                func_addr{func_addr},
                arg_provider{std::move(arg_provider)},
                overhead_func{overhead_func}
                {}


    virtual TimingResult run() override {
        throw new std::logic_error("oneshot doesn't do run()");
    }

    virtual void printHeader(Context& c) {
        c.out() << getId() << " @ 0x" << func_addr << std::endl;
        printNameHeader(c);
        printOneMetric(c, "Sample");
        printAlignedMetrics(c, c.getTimerInfo().getMetricNames());
        c.out() << std::endl;
    }

    virtual void runAndPrint(Context& c) override {
        void *arg = arg_provider();
        raw_result raw = raw_func(loop_count, arg);
        removeOverhead(c, raw);
        printHeader(c);
        for (int i = 0; i < samples; i++) {
            TimingResult result = normalize(TIMER::to_result(raw[i]), this->args, loop_count);
            printBenchName(c, this);
            printOneMetric(c, i + 1);
            printAlignedMetrics(c, result.getResults());
            c.out() << std::endl;
        }
        c.out() << std::endl;
    }

    static typename TIMER::delta_t defaultGetOverhead(Context &c) {
        using O_ALGO = OneshotAlgo<TIMER, OVERHEAD_TOTAL>;

        typename O_ALGO::raw_result raw = O_ALGO::template bench<dummy_bench>(0, nullptr);

        c.out() << "\n---------- Oneshot calibration start --------------\n";

        printResultHeader(c);

        auto b = std::begin(raw) + OVERHEAD_WARMUP;
        auto e = b + OVERHEAD_REAL;
        printOne(c, "min   ", TimerHelper<TIMER>::min(b,e));
        printOne(c, "median (used)", TimerHelper<TIMER>::median(b,e));
        printOne(c, "max   ", TimerHelper<TIMER>::max(b,e));

        c.out() << "---------- Oneshot calibration end   --------------\n\n";
        return TimerHelper<TIMER>::median(b,e);
    }

private:

    /** number of ignored warmup samples for overhead adjustment */
    constexpr static int OVERHEAD_WARMUP =  3;
    /** number of real samples used following the warmup, should be odd for simple median */
    constexpr static int OVERHEAD_REAL = 11;
    constexpr static int OVERHEAD_TOTAL = OVERHEAD_WARMUP + OVERHEAD_REAL;

    static void printOne(Context& c, const std::string& name, const typename TIMER::delta_t& delta) {
        TimingResult result = TIMER::to_result(delta);
        printBenchName(c, std::string("Oneshot overhead ") + name);
        printAlignedMetrics(c, result.getResults());
        c.out() << std::endl;
    }


    /**
     * Subtract out the overhead of an empty run.
     */
    void removeOverhead(Context& c, raw_result& results) {
        if (overhead_func) {
            static typename TIMER::delta_t overhead = overhead_func(c);
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
template <typename TIMER, int SAMPLES = 10>
class OneshotMaker : public MakerBase<TIMER> {
    using bench = OneshotBench<TIMER, SAMPLES>;
    using overhead_f = typename bench::overhead_f;

    typename bench::overhead_f* overhead_func;
public:

    static constexpr int samples = SAMPLES;

    using ALGO = OneshotAlgo<TIMER, samples>;

    OneshotMaker(BenchmarkGroup* parent, overhead_f* overhead_func = bench::defaultGetOverhead, size_t loop_count = 1) :
        MakerBase<TIMER>(parent, loop_count),
        overhead_func{overhead_func}
    {}

    template <bench2_f METHOD>
    void make(
            const std::string& id,
            const std::string& description,
            uint32_t ops_per_invocation,
            const arg_provider_t& arg_provider = null_provider)
    {
        typename ALGO::raw_f *f = ALGO::template bench<METHOD>;
        make2(BenchArgs{this->parent, id, description, ops_per_invocation}, f, (void *)METHOD, arg_provider);
    }

    /**
     * This variant of make accepts a TOUCH method that will be called once before the benchmark starts, useful
     * perhaps to touch the same cache line as the primary function to bring it into L1I.
     */
    template <bench2_f METHOD, bench2_f TOUCH>
    void make(
            const std::string& id,
            const std::string& description,
            uint32_t ops_per_invocation,
            const arg_provider_t& arg_provider = null_provider)
    {
        typename ALGO::raw_f *f = ALGO::template bench<METHOD, TOUCH>;
        make2(BenchArgs{this->parent, id, description, ops_per_invocation}, f, (void *)METHOD, arg_provider);
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
        make2(BenchArgs{this->parent, id, description, ops_per_invocation}, METHOD, (void *)METHOD, arg_provider);
    }

    /**
     * This variant of make accepts a WARMUP method that will be called before *each sample* of the benchmark starts,
     * useful perhaps to put various predictors into a known state.
     */
    template <bench2_f METHOD, bench2_f WARMUP>
    void make_warm(
            const std::string& id,
            const std::string& description,
            uint32_t ops_per_invocation,
            const arg_provider_t& arg_provider = null_provider)
    {
        typename ALGO::raw_f *f = ALGO::template bench_warm<METHOD, WARMUP>;
        make2(BenchArgs{this->parent, id, description, ops_per_invocation}, f, (void *)METHOD, arg_provider);
    }


private:
    void make2(
            const BenchArgs& args,
            typename ALGO::raw_f* f,
            void* f_addr,
            const arg_provider_t& arg_provider = null_provider)
    {
        Benchmark b = new OneshotBench<TIMER, samples>(args, this->loop_count, f, f_addr, arg_provider, overhead_func);
        this->parent->add(b);
    }
};





#endif /* ONESHOT_HPP_ */
