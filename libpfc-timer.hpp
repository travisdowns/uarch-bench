/*
 * lifpfc-timer.hpp
 */

#ifndef LIFPFC_TIMER_HPP_
#define LIFPFC_TIMER_HPP_

#include <limits>

#include "timer-info.hpp"


class LibpfcTimer : public TimerInfo {

public:

    typedef int64_t now_t;

    LibpfcTimer() : TimerInfo("LibpfcTimer",
            "A timer which directly reads the CPU performance counters for accurate cycle measurements.") {}

    virtual void init(Context &context);

    static now_t now() {
        PFC_CNT cnt[7];
        cnt[PFC_FIXEDCNT_CPU_CLK_UNHALTED] = 0;

        // in principle, PFCEND is supposed to be used with a matched pair of END/START pairs, like
        //
        // PFCSTART(cnt);
        // ... code under test
        // PFCEND(cnt);
        //
        // The pairs both read the counters, except that START subtracts from cnt, and END subtracts, so you end up with
        // the delta "automatically". We just have a single now() function, so just return the positive (END) value each
        // time, which should be fine (we can calculate the delta outside the critical region).
        PFCEND(cnt);
        return cnt[PFC_FIXEDCNT_CPU_CLK_UNHALTED];
    }

    static TimingResult to_result(now_t delta) {
        return TimingResult(delta);
    }

private:


    static bool is_init;
};

#endif /* LIFPFC_TIMER_HPP_ */
