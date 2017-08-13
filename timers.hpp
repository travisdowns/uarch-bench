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

#if USE_LIBPFC
#include "libpfc/include/libpfc.h"
#include "libpfc-timer.hpp"
#endif

/*
 * This class measures cycles indirectly by measuring the wall-time for each test, and then converting
 * that to a cycle count based on a calibration loop performed once at startup.
 */
template <typename CLOCK>
class ClockTimerT : public TimerInfo {

    /* aka 'cycles per nanosecond */
    static double ghz;

public:

    ClockTimerT(std::string clock_name) : TimerInfo("ClockTimer", std::string("Use the system clock (" + clock_name
            + ") to measure wall-clock time, and convert to cycles using a calibration loop"),
            {"Cycles", "Nanos"}) {}

    virtual void init(Context &context) {
    }

    static int64_t now() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(CLOCK::now().time_since_epoch()).count();
    }

    static TimingResult to_result(int64_t nanos) {
        return TimingResult({nanos * ghz, (double)nanos});
    }

    /* return the statically calculated clock speed of the CPU in ghz for this clock */
    static double getGHz() {
        return ghz;
    }
};


// this x macro lists all the timers in use, which can be useful for example to explicitly instantiate a benchmark
// class for all possible timers
#if USE_LIBPFC
#define ALL_TIMERS_X(FN) \
        FN(ClockTimerT<std::chrono::high_resolution_clock>) \
        FN(LibpfcTimer)
#else
#define ALL_TIMERS_X(FN) \
        FN(ClockTimerT<std::chrono::high_resolution_clock>)
#endif


#endif /* TIMERS_HPP_ */
