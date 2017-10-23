/*
 * All timers are declared here, and benchmark implementations will generally need to include this file in order
 * to explicitly instantiate the benchmark code for all defined timers.
 *
 * timers.hpp
 */

#ifndef TIMERS_HPP_
#define TIMERS_HPP_

#include <chrono>
#include <string>
#include <iomanip>

#include "timer-info.hpp"
#include "context.hpp"

#if USE_LIBPFC
#include "libpfc-timer.hpp"
#endif


/*
 * This class measures cycles indirectly by measuring the wall-time for each test, and then converting
 * that to a cycle count based on a calibration loop performed once at startup.
 */
template <typename CLOCK>
class ClockTimerT : public TimerBase<ClockTimerT<CLOCK>> {
public:

    typedef int64_t now_t;
    typedef int64_t delta_t;

    ClockTimerT(std::string clock_name) : TimerBase<ClockTimerT<CLOCK>>("clock",
            std::string("Use the system clock (" + clock_name +
            ") to measure wall-clock time, and convert to cycles using a calibration loop"),
            {"Cycles", "Nanos"}) {}

    void init(Context &c) override {
        c.out() << "Median CPU speed: " << std::fixed << std::setw(4) << std::setprecision(3)
        << getGHz() << " GHz" << std::endl;
    }

    static int64_t now() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(CLOCK::now().time_since_epoch()).count();
    }

    static TimingResult to_result(int64_t nanos) {
        return TimingResult({nanos * getGHz(), (double)nanos});
    }

    /*
     * Return the delta of a and b, that is a minus b.
     */
    static int64_t delta(int64_t a, int64_t b) {
        return a - b;
    }

    static int64_t aggregate(const int64_t *begin, const int64_t *end) {
        // For now just choose the minimum element on the idea that there will be slower deviations from
        // the true speed (e.g., cache misses, interrupts), but no negative deviations (how can the CPU
        // run faster than idea?). For more complex cases this assumption doesn't always hold.
        return *std::min_element(begin, end);
    }

    /* return the statically calculated clock speed of the CPU in ghz for this clock */
    static double getGHz();
};

// the default ClockTimer will use high_resolution_clock
using DefaultClockTimer = ClockTimerT<std::chrono::high_resolution_clock>;


// this x macro lists all the timers in use, which can be useful for example to explicitly instantiate a benchmark
// class for all possible timers
#if USE_LIBPFC
#define ALL_TIMERS_X(FN) \
        FN(DefaultClockTimer) \
        FN(LibpfcTimer)
#else
#define ALL_TIMERS_X(FN) \
        FN(DefaultClockTimer)
#endif


#endif /* TIMERS_HPP_ */
