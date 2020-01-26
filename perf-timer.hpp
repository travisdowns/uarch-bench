/*
 * perf-timer.hpp
 */

#ifndef PERF_TIMER_HPP_
#define PERF_TIMER_HPP_

#if USE_PERF_TIMER

#include <limits>
#include <memory>

#include "hedley.h"

#include "timer-info.hpp"

#define MAX_EXTRA_EVENTS 8

struct PerfNow {
    constexpr static unsigned READING_COUNT = MAX_EXTRA_EVENTS + 1;
    /* in principle the readings are unsigned values, but since we subtract two values to get a delta,
     * there is a good chance of getting a small negative value (e.g., if the dummy run had more of a
     * given event than the actual run) which if unsigned produces a large nonsense value.
     */
    int64_t readings[READING_COUNT];
    /* return the unhalted clock cycles value (PFC_FIXEDCNT_CPU_CLK_UNHALTED) */
    uint64_t getClk() const { return readings[0]; }
};

class PerfTimer : public TimerInfo {

public:

    typedef PerfNow   now_t;
    typedef PerfNow delta_t;

    PerfTimer(Context &c);

    virtual void init(Context &) override;

//    HEDLEY_ALWAYS_INLINE
    static now_t now();

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

    virtual ~PerfTimer();
};

struct NamedEvent {
    std::string name;
    std::string header;

    NamedEvent(const std::string& name);

    NamedEvent(const std::string& name, const std::string& header);

    bool operator==(const NamedEvent& e) const {
        return name == e.name && header == e.header;
    }
};


/* parse an --extra-events string of perf events
 * Basically split events on commas, but commas in-between / characters
 * are ignored for splitting.
 * So 'cpu/123,456/,cpu/xxx,yyy/' is split as two tokens:
 * 'cpu/123,456' and 'cpu/xxx,yyy/'
 */
std::vector<NamedEvent> parsePerfEvents(const std::string& event_string);

#endif

#endif /* PERF_TIMER_HPP_ */
