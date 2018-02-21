/*
 * lifpfc-timer.hpp
 */

#ifndef LIFPFC_TIMER_HPP_
#define LIFPFC_TIMER_HPP_

#include <limits>

#include "libpfc/include/libpfc.h"
#include "hedley.h"

#include "timer-info.hpp"
#include "libpfm4-support.hpp"

#define FIXED_COUNTERS 3
#define    GP_COUNTERS 4
#define TOTAL_COUNTERS (FIXED_COUNTERS + GP_COUNTERS)

/* this code is used in the appropriate fixed counter slow to enable the associated counter */
#define FIXED_COUNTER_ENABLE 0x2

struct LibpfcNow {
    PFC_CNT cnt[TOTAL_COUNTERS];
    /* return the unhalted clock cycles value (PFC_FIXEDCNT_CPU_CLK_UNHALTED) */
    PFC_CNT getClk() const { return cnt[PFC_FIXEDCNT_CPU_CLK_UNHALTED]; }
};

class LibpfcTimer : public TimerBase<LibpfcTimer> {

public:

    typedef LibpfcNow   now_t;
    typedef LibpfcNow delta_t;

    LibpfcTimer(Context &c);

    virtual void init(Context &) override;

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

    static TimingResult to_result(LibpfcNow delta);

    static void addCustomArgs(args::ArgumentParser& parser);

    static void customRunHandler(Context& c);

    /*
     * Return the delta of a and b, that is a minus b.
     */
    static LibpfcNow delta(const LibpfcNow& a, const LibpfcNow& b);

    static PFC_CNT aggr_value(const LibpfcNow& now) {
        return now.getClk();
    }

    static LibpfcNow aggregate(const LibpfcNow *begin, const LibpfcNow *end);

private:

    static bool is_init;
};

extern "C" {
/** libpfc raw functions implement this function and result the results in result */
typedef void (libpfc_raw1)(size_t loop_count, void *arg, LibpfcNow* results);
}

template <int samples, libpfc_raw1 METHOD>
std::array<LibpfcNow, samples> libpfc_raw_adapt(size_t loop_count, void *arg) {
    std::array<LibpfcNow, samples> result = {};
    for (int i = 0; i < samples; i++) {
        METHOD(loop_count, arg, &result[i]);
    }
    return result;
}

#endif /* LIFPFC_TIMER_HPP_ */
