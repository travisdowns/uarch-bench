/*
 * timer_info.hpp
 */

#ifndef TIMER_INFO_HPP_
#define TIMER_INFO_HPP_

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <array>

#include "args.hxx"

#include "bench-declarations.h"

class Context;

typedef std::function<void * ()>         arg_method_t;  // generates the argument for the benchmarking function

/**
 * A timing operation ultimately returns a result of this type, regardless of the
 * underlying timer.
 */
class TimingResult {

    std::vector<double> results_;
public:
    TimingResult(std::vector<double> results) : results_(std::move(results)) {}

    /* multiply all values by the given value, useful when normalizing */
    TimingResult operator*(double multipler) {
        TimingResult result(*this);
        for (double& r : result.results_) {
            r *= multipler;
        }
        return result;
    }

    double getCycles() const {
        return results_.at(0);
    }

    const std::vector<double>& getResults() const {
        return results_;
    }

};

typedef std::function<TimingResult (size_t)> full_bench_t;  // a full timing method

/*
 * Contains information about a particular TIMER implementation. Timer implementations inherit this class
 * and should also implement the duck-typing static TIMER methods now(), and so on. That is, an timer classes
 * both provide the virtual methods and state via instantiation at rutnime, as well as being used as a TIMER
 * template parameter to such that their static methods are compiled directly into the benchmark methods.
 */
class TimerInfo {
	std::string name_, description_;
protected:
	std::vector<std::string> metric_names_;
public:

	TimerInfo(std::string name, std::string description, std::vector<std::string> metric_names) :
		name_(name), description_(description), metric_names_(metric_names) {}

	virtual std::string getName() const {
		return name_;
	}

	virtual std::string getDesciption() const {
		return description_;
	}

	/* return a list of the names for 1 or more metrics that are recorded by this time */
	virtual const std::vector<std::string>& getMetricNames() const {
	    return metric_names_;
	}

	/*
	 * Do any initialization required prior to using the timer. Of course, you can put initialization
	 * in the constructor as well, but slow initialization, or init that might fail can usefully go
	 * here since timers may be created but never used in various cases (e.g., all timers are generally
	 * created even though only a specific one will usually be used in any given run).
	 *
	 * Really this is just a poor design though and we should factor out the simple timer state, from the
	 * "ready to measure" state which might require complicated init such as calibration. One day...
	 */
	virtual void init(Context &context) = 0;

	virtual ~TimerInfo() = default;

	// this static method can be overridden in subclasses to expose timer-specific command line arguments
	static void addCustomArgs(args::ArgumentParser& parser) {}

	// this static method can be overriden to implement custom behavior at the start of the benchmark run
	static void customRunHandler(Context& c) {}

	/***************************
	 * Implementations of this class should additionally
	 * implement the following static methods which called directly
	 * from the measurement methods (and should ideally be inline or declared within the class).

	static int64_t now() {
        return duration_cast<nanoseconds>(CLOCK::now().time_since_epoch()).count();
    }

    static TimingResult to_result(int64_t nanos) {
        return {nanos * ghz, (double)nanos};
    }

	**************************/
};


/**
 * Normally you implement a Timer by inheriting from this class, templatized on your implementation
 * class in the CRTP pattern. This class uses various static methods in your class to implement
 * the TimerInfo interface, and also to wrap bare benchmark methods in the static scaffolding turning
 * them into a full benchmark.
 */
template <typename TIMER_INFO>
class TimerBase : public TimerInfo {
public:

    static constexpr int warmup_samples =  2;
    static constexpr int total_samples   = 35;

    static_assert(warmup_samples < total_samples, "warmup samples must be less than total");

//    using now_t2 = typename TIMER_INFO::now_t;

    TimerBase(const std::string& name, const std::string& description, const std::vector<std::string>& metric_names)
            : TimerInfo(name, description, metric_names) {}

    template <typename TIMING>
    static full_bench_t make_bench_method(TIMING t) {
        auto l = [t](int loop_count){ return doTiming(loop_count, t); };
        return l;
    }

    template <typename TIMING>
    static TimingResult doTiming(size_t loop_count, TIMING t) {
        std::array<typename TIMER_INFO::now_t, total_samples> raw_results;
        // warmup
        for (int i = 0; i < total_samples; i++) {
            raw_results[i] = t(loop_count);
        }

//        auto aggr = *std::min_element(raw_results.begin() + warmup_samples, raw_results.end());
        typename TIMER_INFO::delta_t aggr = TIMER_INFO::aggregate(raw_results.begin() + warmup_samples, raw_results.end());
        return TIMER_INFO::to_result(aggr);
    }

};

/*
 * A timing implementation that simply calls the METHOD once bracketed by calls to Timer::now().
 * The downside is that the overhead of the timer calls is not cancelled out, and there is no
 * mechanism to cancel out overhead within the METHOD call itself.
 *
 * The METHOD accepts two arguments: a loop counter, and an arbitrary benchmark-specific void *
 * which is most often used to use the same benchmark code repeatedly with different input values.
 */
template <typename TIMER, bench2_f METHOD>
class Timing2 {
    arg_method_t arg_method_;
    void* arg_;
public:
    Timing2(arg_method_t arg_method) :
        arg_method_(arg_method), arg_(arg_method()) {}

    typename TIMER::delta_t operator()(size_t loop_count) {
        return time_inner(loop_count, arg_);
    }

    static typename TIMER::delta_t time_inner(size_t loop_count, void* arg) {
        auto t0 = TIMER::now();
        METHOD(loop_count, arg);
        auto t1 = TIMER::now();
        return TIMER::delta(t1, t0);
    }
};





#endif /* TIMER_INFO_HPP_ */
