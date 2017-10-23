/*
 * libpfc-timer.cpp
 */

#include <vector>
#include <assert.h>

#include <sched.h>
#include <x86intrin.h> // for rdtsc


#include "args.hxx"
#include "libpfc/include/libpfc.h"
#include "libpfc-timer.hpp"
#include "util.hpp"
#include "asm_methods.h"  // for add_calibration
#include "context.hpp"

using namespace std;

#define CALIBRATION_NANOS (1000L * 1000L * 1000L)
#define CALIBRATION_ADDS (CALIBRATION_NANOS * 3) // estimate 3GHz

bool LibpfcTimer::is_init = false;

static args::Group pfc_args("libpfc timer specific arguments");
static args::Flag arg_listevents{pfc_args, "list-events", "Dislay the available available PMU events", {"list-events"}};
static args::ValueFlag<std::string> arg_extraevents{pfc_args, "extra-events", "A comma separated list of extra PMU events to track", {"extra-events"}};

static std::vector<PmuEvent> all_events{PmuEvent("Cycles", FIXED_COUNTER_ENABLE, PFC_FIXEDCNT_CPU_CLK_UNHALTED)};

LibpfcTimer::LibpfcTimer(Context& c) : TimerBase<LibpfcTimer>(
        "libpfc",
        "A timer which directly reads the CPU performance counters for accurate cycle measurements.",
        {})
{
    for (auto &event : all_events) {
        metric_names_.push_back(event.short_name);
    }
}

LibpfcNow LibpfcTimer::delta(const LibpfcNow& a, const LibpfcNow& b) {
    LibpfcNow ret;
    for (int i = 0; i < TOTAL_COUNTERS; i++) {
        ret.cnt[i] = a.cnt[i] - b.cnt[i];
    }
    return ret;
}

LibpfcNow LibpfcTimer::aggregate(const LibpfcNow *begin, const LibpfcNow *end) {
    return *std::min_element(begin, end,
            [](const LibpfcNow& left, const LibpfcNow& right) { return left.getClk() < right.getClk(); }
    );
}

TimingResult LibpfcTimer::to_result(LibpfcNow delta) {
    vector<double> results;
    results.reserve(all_events.size());
    for (auto& event : all_events) {
        unsigned slot = event.slot;
        results.push_back(delta.cnt[slot]);
    }
    return TimingResult(std::move(results));
}

static void inner_init(Context &context) {
    auto err = pfcInit();
    if (err) {
        const char* msg = pfcErrorString(err);
        throw std::runtime_error(std::string("pfcInit() failed (error ") + std::to_string(err) + ": " + msg + ")");
    }

    err = pfcPinThread(0);
    if (err) {
        // let's treat this as non-fatal, it could occur if, for example
        context.err() << "WARNING: Pinning to CPU 0 failed, continuing without pinning" << endl;
    } else {
        context.log() << "Pinned to CPU 0" << endl;
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

    context.log() << "lipfc init OK" << std::endl;

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

void LibpfcTimer::init(Context &context) {
    if (!is_init) {
        inner_init(context);
        is_init = true;
    }
}

void LibpfcTimer::addCustomArgs(args::ArgumentParser& parser) {
    parser.Add(pfc_args);
}

void LibpfcTimer::customRunHandler(Context& c) {
    if (arg_listevents) {
        listPfm4Events(c);
        throw SilentSuccess();
    } else if (arg_extraevents) {
        auto extra_events = parseExtraEvents(c, arg_extraevents.Get());
        unsigned ecount = 0;
        for (auto& event : extra_events) {
            if (ecount >= GP_COUNTERS) {
                c.err() << "Too many events requested, event " << event.full_name << " won't be recorded";
            } else {
                // assign slots consecutively starting after FIXED_COUNTERS slots
                event.slot = FIXED_COUNTERS + ecount;
                all_events.push_back(event);
                ecount++;
            }
        }
    }
}




