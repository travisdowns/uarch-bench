/*
 * libpfc-timer.hpp
 */

#ifndef PERF_TIMER_HPP_
#define PERF_TIMER_HPP_


#include <limits>

#include "hedley.h"

#include "timer-info.hpp"

#if USE_PERF_TIMER

struct PerfNow {
    uint64_t tsc;
    /* return the unhalted clock cycles value (PFC_FIXEDCNT_CPU_CLK_UNHALTED) */
    uint64_t getClk() const { return tsc; }
};

class PerfTimer : public TimerInfo {

public:

    typedef PerfNow   now_t;
    typedef PerfNow delta_t;

    PerfTimer(Context &c);

    virtual void init(Context &, const TimerArgs& args) override;

    HEDLEY_ALWAYS_INLINE
    static now_t now() {
        return {__rdtsc()};
    }

    static TimingResult to_result(const PerfTimer& ti, PerfNow delta);

    /*
     * Return the delta of a and b, that is a minus b.
     */
    static PerfNow delta(const PerfNow& a, const PerfNow& b);

    static uint64_t aggr_value(const PerfNow& now) {
        return now.getClk();
    }

    static PerfNow aggregate(const PerfNow *begin, const PerfNow *end);

    virtual void listEvents(Context& c) override;
};

#endif

#endif /* PERF_TIMER_HPP_ */
