/*
 * timer_info.hpp
 */

#ifndef TIMER_INFO_HPP_
#define TIMER_INFO_HPP_

#include <vector>
#include <string>
#include <functional>
#include <array>

#include "args.hxx"

#include "stats.hpp"
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
    TimingResult operator*(double multipler) const {
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

/**
 * Passed to init() with any "global" args that are meant for the individual timer
 */
struct TimerArgs {
    // a string of requested "extra events" passed via the --extra-events string
    std::string extra_events;
};


/*
 * Contains information about a particular TIMER implementation. Timer implementations inherit from this class
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

	// this static method can be overridden to implement custom behavior at the start of the benchmark run
	static void customRunHandler(Context& c) {}

	// if the --list-events command line argument is specified, this will be called on your timer (if your timer
	// is selected), and you should ouutput any additional supported events to Context.out()
	virtual void listEvents(Context& c) = 0;

	/***************************
	 * Implementations of this class should additionally
	 * implement the following static methods which called directly
	 * from the measurement methods (and should ideally be inline or declared within the class).

    // You want ALWAYS_INLINE to enforce consistency of behavior more than "it's faster" - if you don't say anything about
    // the inlining, some benchamrks may inline the now() calls and some not, based on opaque compiler heuristics. For example,
    // I found that the ::now calls for one-shot timers were being inlined in all the actual benchmarks, but not
    // in the overhead calculation, meaning that a overhead-removed benchmark of dummy_bench showed up as -1 cycles, not 0.
    HEDLEY_ALWAYS_INLINE
	static now_t now() {
        // return the current now_t
        // this is the key timer method that will be inlined into the innermost loop when generating the
        // core benchmarking code, so it should be as efficient and fast as possible
    }

    static TimingResult to_result(TimerInfo& ti, delta_t now) {
        // generate and return a TimingResult from now
        // this method is called out of the core benchmark code, so doesn't have to be fast
        // The ti object passes is the actual TimerInfo instance
    }


    * Given a delta_t object, return a comparable value used for aggregation (usually the number of clocks or time
    * taken).
    static VALUE aggr_value(const delta_t& delta);

	**************************/
};


/**
 * Implements some useful method when instantiated on a timer.
 */
template <typename TIMER>
struct TimerHelper {

    using delta_t = typename TIMER::delta_t;

    static bool less(const delta_t& left, const delta_t& right) {
        return TIMER::aggr_value(left) < TIMER::aggr_value(right);
    }

    template <typename IT>
    static delta_t min(IT&& begin, IT&& end) { return *std::min_element(begin, end, less); }

    template <typename IT>
    static delta_t max(IT&& begin, IT&& end) { return *std::max_element(begin, end, less); }

    template <typename IT>
    static delta_t median(IT&& begin, IT&& end) { return Stats::medianf(begin, end, less); }


};





#endif /* TIMER_INFO_HPP_ */
