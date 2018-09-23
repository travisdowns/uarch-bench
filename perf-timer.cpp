/*
 * perf-timer.cpp
 */

#if USE_PERF_TIMER

#include <vector>
#include <iostream>
#include <assert.h>

#include <sched.h>
#include <x86intrin.h> // for rdtsc


#include "args.hxx"
#include "perf-timer.hpp"
#include "util.hpp"
#include "context.hpp"

using namespace std;

PerfTimer::PerfTimer(Context& c) : TimerInfo(
        "perf",
        "A timer which uses rdpmc and the perf_events subsystem for accurate cycle measurements",
        {"Cycles"})
{
}

PerfNow PerfTimer::delta(const PerfNow& a, const PerfNow& b) {
    return PerfNow{a.tsc - b.tsc};
}

TimingResult PerfTimer::to_result(const PerfTimer& ti, PerfNow delta) {
    return TimingResult{ {(double)delta.tsc} };
}

void PerfTimer::init(Context &context, const TimerArgs& args) {
}

void PerfTimer::listEvents(Context& c) {
    c.out() << "Perf timer has no events (yet)" << endl;
}

#endif // USE_PERF_TIMER


