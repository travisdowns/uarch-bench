/*
 * timer_info.hpp
 */

#ifndef TIMER_INFO_HPP_
#define TIMER_INFO_HPP_

#include <iostream>
#include <vector>
#include <string>

class Context;

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





#endif /* TIMER_INFO_HPP_ */
