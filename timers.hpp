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

#include "timer-info.hpp"
#include "context.hpp"
#include "hedley.h"

#include "libpfc-timer.hpp"
#include "perf-timer.hpp"

#if USE_LIBPFC
#define IF_LIBPFC(x) x
#else
#define IF_LIBPFC(x)
#endif

#if USE_PERF_TIMER
#define IF_PERF_TIMER(x) x
#else
#define IF_PERF_TIMER(x)
#endif



/**
 * Adapt any of the clocks offered by clock_gettime into a clock suitable for use by ClockTimerT
 */
template <int CLOCK>
struct GettimeAdapter {
    static int64_t nanos() {
        struct timespec ts;
        clock_gettime(CLOCK, &ts);
        return (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
    }
};


template <typename STD_CLOCK>
struct StdClockAdapt {
    static int64_t nanos() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(STD_CLOCK::now().time_since_epoch()).count();
    }
};

/*
 * This class measures cycles indirectly by measuring the wall-time for each test, and then converting
 * that to a cycle count based on a calibration loop performed once at startup.
 */
template <typename CLOCK>
class ClockTimerT : public TimerInfo {
public:

    typedef int64_t now_t;
    typedef int64_t delta_t;

    ClockTimerT(std::string clock_name) : TimerInfo("clock",
            std::string("Use the system clock (" + clock_name +
            ") to measure wall-clock time, and convert to cycles using a calibration loop"),
            {"Cycles", "Nanos"}) {}

    void init(Context &c) override;

    HEDLEY_ALWAYS_INLINE
    static int64_t now() {
        return CLOCK::nanos();
    }

    static TimingResult to_result(const ClockTimerT<CLOCK>& ti, int64_t nanos) {
        return TimingResult({nanos * getGHz(), (double)nanos});
    }

    /*
     * Return the delta of a and b, that is a minus b.
     */
    static int64_t delta(int64_t a, int64_t b) {
        return a - b;
    }

    static int64_t aggr_value(int64_t delta) {
        return delta;
    }

    /* return the statically calculated clock speed of the CPU in ghz for this clock */
    static double getGHz();

    virtual void listEvents(Context& c) override {
        c.out() << "The clock timer doesn't have any supported extra events";
    }
};

// the default ClockTimer will use high_resolution_clock
using DefaultClockTimer = ClockTimerT<StdClockAdapt<std::chrono::high_resolution_clock>>;

// print a variety of information about system clock overheads to the given ostream
void printClockOverheads(std::ostream& out);


// this x macro lists all the timers in use, which can be useful for example to explicitly instantiate a benchmark
// class for all possible timers
#define ALL_TIMERS_X(FN) \
                      FN(DefaultClockTimer)  \
        IF_LIBPFC(    FN(LibpfcTimer      )) \
        IF_PERF_TIMER(FN(PerfTimer        )) \


#endif /* TIMERS_HPP_ */
