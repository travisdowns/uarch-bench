/*
 * libpfc-timer.hpp
 */

#ifndef LIFPFC_TIMER_HPP_
#define LIFPFC_TIMER_HPP_


#include <limits>

#include "hedley.h"

#include "timer-info.hpp"
#include "libpfm4-support.hpp"

#if USE_LIBPFC
#include "libpfc/include/libpfc.h"

#define FIXED_COUNTERS 3
#define    GP_COUNTERS 4
#define TOTAL_COUNTERS (FIXED_COUNTERS + GP_COUNTERS)

/* this code is used in the appropriate fixed counter slot to enable the associated counter */
#define FIXED_COUNTER_ENABLE 0x2

struct LibpfcNow {
    PFC_CNT cnt[TOTAL_COUNTERS];
    /* return the unhalted clock cycles value (PFC_FIXEDCNT_CPU_CLK_UNHALTED) */
    PFC_CNT getClk() const { return cnt[PFC_FIXEDCNT_CPU_CLK_UNHALTED]; }
};

class LibpfcTimer : public TimerInfo {

public:

    typedef LibpfcNow   now_t;
    typedef LibpfcNow delta_t;

    LibpfcTimer(Context &c);

    virtual void init(Context&) override;

    HEDLEY_ALWAYS_INLINE
    static now_t now() {
        LibpfcNow now = {};

        // in principle, the PFCSTART/END macros are supposed to be used with a matched pair of END/START pairs, like
        //
        // PFCSTART(cnt);
        // ... code under test
        // PFCEND(cnt);
        //
        // The pairs both read the counters, except that START subtracts from cnt, and END adds, so you end up with
        // the delta "automatically". We just have a single now() function, so just return the positive (END) value each
        // time, which should be fine (we can calculate the delta outside the critical region).
        PFCEND(now.cnt);
        return now;
    }

    static TimingResult to_result(const LibpfcTimer& ti, LibpfcNow delta);

    /*
     * Return the delta of a and b, that is a minus b.
     */
    static LibpfcNow delta(const LibpfcNow& a, const LibpfcNow& b);

    static PFC_CNT aggr_value(const LibpfcNow& now) {
        return now.getClk();
    }

    static LibpfcNow aggregate(const LibpfcNow *begin, const LibpfcNow *end);

    /////////////////////////
    // TimerInfo overrides //
    /////////////////////////

    virtual void listEvents(Context& c) override;
private:
    std::vector<PmuEvent> all_events;
};

#endif

#endif /* LIFPFC_TIMER_HPP_ */
