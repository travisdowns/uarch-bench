/*
 * timer_info.hpp
 */

#ifndef TIMER_INFO_HPP_
#define TIMER_INFO_HPP_

#include <iostream>
#include <vector>
#include <string>
#include <functional>

#include "bench-declarations.hpp"

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

/*
 * Contains information about a particular TIMER implementation. Timer implementations inherit this class
 * and should also implement the duck-typing static TIMER methods now(), and so on. That is, an timer classes
 * both provide the virtual methods and state via instantiation at rutnime, as well as being used as a TIMER
 * template parameter to such that their static methods are compiled directly into the benchmark methods.
 */
class TimerInfo {
	std::string name_, description_;
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
	 * Do any initialization required prior to using the timer
	 */
	virtual void init(Context &context) = 0;

	virtual ~TimerInfo() = default;

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

/*
 * A timing implementation that simply calls the METHOD once bracketed by calls to Timer::now().
 * The downside is that the overhead of the timer calls is not cancelled out, and there is no
 * mechanism to cancel out overhead within the METHOD call itself.
 */
template <typename TIMER>
class Timing {
public:
    template <bench_f METHOD>
    static int64_t time_method(size_t loop_count) {
        auto t0 = TIMER::now();
        METHOD(loop_count);
        auto t1 = TIMER::now();
        return t1 - t0;
    }
};

/*
 * Like Timing, this implements a time_method_t, but as a member function since it wraps an argument provider
 * method
 */
template <typename TIMER, bench2_f METHOD>
class Timing2 {
    arg_method_t arg_method_;
    void* arg_;
public:
    Timing2(arg_method_t arg_method) :
        arg_method_(arg_method), arg_(arg_method()) {}

    int64_t operator()(size_t loop_count) {
        return time_inner(loop_count, arg_);
    }

    static int64_t time_inner(size_t loop_count, void* arg) {
        auto t0 = TIMER::now();
        METHOD(loop_count, arg);
        auto t1 = TIMER::now();
        return t1 - t0;
    }
};





#endif /* TIMER_INFO_HPP_ */
