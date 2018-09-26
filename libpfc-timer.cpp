/*
 * libpfc-timer.cpp
 */

#include <vector>
#include <assert.h>

#include <sched.h>


#include "args.hxx"
#include "libpfc/include/libpfc.h"
#include "libpfc-timer.hpp"
#include "util.hpp"
#include "context.hpp"

using namespace std;

#define CALIBRATION_NANOS (1000L * 1000L * 1000L)
#define CALIBRATION_ADDS (CALIBRATION_NANOS * 3) // estimate 3GHz


LibpfcTimer::LibpfcTimer(Context& c) : TimerInfo(
        "libpfc",
        "A timer which directly reads the CPU performance counters for accurate cycle measurements.",
        {})
{}

LibpfcNow LibpfcTimer::delta(const LibpfcNow& a, const LibpfcNow& b) {
    LibpfcNow ret;
    for (int i = 0; i < TOTAL_COUNTERS; i++) {
        ret.cnt[i] = a.cnt[i] - b.cnt[i];
    }
    return ret;
}



TimingResult LibpfcTimer::to_result(const LibpfcTimer& ti, LibpfcNow delta) {
    vector<double> results;
    results.reserve(ti.all_events.size());
    for (auto& event : ti.all_events) {
        unsigned slot = event.slot;
        results.push_back(delta.cnt[slot]);
    }
    return TimingResult(std::move(results));
}

void LibpfcTimer::init(Context& c, const TimerArgs& args) {
    auto err = pfcInit();
    if (err) {
        const char* msg = pfcErrorString(err);
        throw std::runtime_error(std::string("pfcInit() failed (error ") + std::to_string(err) + ": " + msg + ")");
    }

    all_events.push_back(PmuEvent{"Cycles", FIXED_COUNTER_ENABLE, PFC_FIXEDCNT_CPU_CLK_UNHALTED});

    auto extra_events = parseExtraEvents(c, args.extra_events);
    unsigned ecount = 0;
    for (auto& event : extra_events) {
        if (ecount >= GP_COUNTERS) {
            c.err() << "Too many events requested, event " << event.full_name << " won't be recorded" << std::endl;
        } else {
            // assign slots consecutively starting after FIXED_COUNTERS slots
            event.slot = FIXED_COUNTERS + ecount;
            all_events.push_back(event);
            ecount++;
        }
    }

    for (auto& e : all_events) {
        metric_names_.push_back(e.short_name);
    }

    PFC_CFG  cfg[7] = {};

    for (auto& event : all_events) {
        assert(event.slot < TOTAL_COUNTERS);
        cfg[event.slot] = event.code;
    }

    err = pfcWrCfgs(0, sizeof(cfg)/sizeof(cfg[0]), cfg);
    if (err) {
        const char* msg = pfcErrorString(err);
        throw std::runtime_error(std::string("pfcWrCfgs() failed (error ") + std::to_string(err) + ": " + msg + ")");
    }

    c.out()   << "libpfc timer init OK" << endl;

    //    /* calculate CPU frequency using reference cycles */
    //    for (int i = 0; i < 100; i++) {
    //        PFC_CNT cnt[7] = {};
    //
    //        int64_t start = nanos(), end = start + CALIBRATION_NANOS, now;
    //        PFCSTART(cnt);
    //        int64_t tsc =__rdtsc();
    //        add_calibration(CALIBRATION_ADDS);
    //        PFCEND(cnt);
    //        int64_t tsc_delta = __rdtsc() - tsc;
    //        now = nanos();
    //
    //        int64_t delta = now - start;
    //        printf("CPU %d, measured CLK_REF_TSC MHz: %6.2f\n", sched_getcpu(), 1000.0 * cnt[PFC_FIXEDCNT_CPU_CLK_REF_TSC] / delta);
    //        printf("CPU %d, measured rdtsc MHz      : %6.2f\n", sched_getcpu(), 1000.0 * tsc_delta / delta);
    //        printf("CPU %d, measured add   MHz      : %6.2f\n", sched_getcpu(), 1000.0 * CALIBRATION_ADDS / delta);
    //    }
}

void LibpfcTimer::listEvents(Context& c) {
    c.out() << "Events supported by libpfc timer on this hardware:" << endl;
    listPfm4Events(c);
}



