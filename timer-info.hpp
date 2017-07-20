/*
 * timer_info.hpp
 */

#ifndef TIMER_INFO_HPP_
#define TIMER_INFO_HPP_

#include "context.h"

/**
 * A timing operation ultimately returns a result of this type, regardless of the
 * underlying timer.
 */
class TimingResult {
    bool hasCycles_, hasNanos_;
    double cycles_, nanos_;

public:
    TimingResult(double cycles, double nanos) :
        hasCycles_(true), cycles_(cycles), nanos_(nanos) {}

    TimingResult(double cycles) :
            hasCycles_(true), hasNanos_(false), cycles_(cycles), nanos_(std::numeric_limits<double>::quiet_NaN()) {}

    /* multiply all values by the given value, useful when normalizing */
    TimingResult operator*(double multipler) {
        TimingResult result(*this);
        result.nanos_ *= multipler;
        result.cycles_ *= multipler;
        return result;
    }

    double getCycles() const {
        return cycles_;
    }

    double getNanos() const {
        return nanos_;
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
public:

	TimerInfo(std::string name, std::string description) :
		name_(name), description_(description) {}

	virtual std::string getName() const {
		return name_;
	}

	virtual std::string getDesciption() const {
		return description_;
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
