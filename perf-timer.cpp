/*
 * perf-timer.cpp
 */

#if USE_PERF_TIMER

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>

#include <linux/perf_event.h>
#include <sched.h>

extern "C" {
#include "pmu-tools/jevents/jevents.h"
#include "pmu-tools/jevents/rdpmc.h"
}

#include "table.hpp"
#include "timers.hpp"
#include "perf-timer.hpp"
#include "util.hpp"
#include "context.hpp"


using namespace std;

struct PerfEvent {
    enum Type { RAW, PERF } type;
    std::string name, perf_string, desc, pmu;
};

struct RunningEvent {
    rdpmc_ctx ctx;
    std::string name;
    std::string header;

    RunningEvent(const rdpmc_ctx& ctx, const std::string& name) :
        ctx{ctx}, name{name}, header{make_header(name)}
    {}

    static std::string make_header(std::string name) {
        return name.substr(0, std::min((size_t)6, name.length()));
    }
};

static int init_count;
static vector<RunningEvent> running_events;


int do_read_events() {
    return read_events(nullptr);
}

typedef int (*walker_callback_t)(void *data, char *name, char *event, char *desc);
typedef int (*walker_t)(walker_callback_t callback, void *data);

std::vector<PerfEvent> get_events(PerfEvent::Type type, walker_t walker) {
    vector<PerfEvent> events;
    walker([](void *data, char *name, char *event, char *desc) -> int {
        ((vector<PerfEvent>*)data)->push_back(PerfEvent{PerfEvent::RAW, name, event, desc});
        return 0;
    }, &events);
    return events;
}

std::vector<PerfEvent> get_raw_events() {
    return get_events(PerfEvent::RAW, walk_events);
}

std::vector<PerfEvent> get_perf_events() {
    return get_events(PerfEvent::PERF, walk_perf_events);
}

std::vector<PerfEvent> get_all_events() {
    auto  raw = get_raw_events();
    auto perf = get_perf_events();
    std::vector<PerfEvent> ret(raw.begin(), raw.end());
    ret.insert(ret.end(), perf.begin(), perf.end());
    return ret;
}

/**
 * Take a perf_event_attr objects and return a string representation suitable
 * for use as an event for perf, or just for display.
 */
std::string perf_attr_to_string(perf_event_attr* attr) {
    std::string ret;
    char* pmu = resolve_pmu(attr->type);
    ret += std::string(pmu ? pmu : "???") + "/";

#define APPEND_IF_NZ1(field) APPEND_IF_NZ2(field,field)
#define APPEND_IF_NZ2(name, field) if (attr->field) ret += std::string(#name) + "=" + std::to_string(attr->field) + ","

    APPEND_IF_NZ1(config);
    APPEND_IF_NZ2(period, sample_period);


    ret.at(ret.length() - 1) = '/';
    return ret;
}



PerfTimer::PerfTimer(Context& c) : TimerInfo(
        "perf",
        "A timer which uses rdpmc and the perf_events subsystem for accurate cycle measurements",
        {})
{}

PerfNow PerfTimer::delta(const PerfNow& a, const PerfNow& b) {
    PerfNow ret;
    for (unsigned i = 0; i < PerfNow::READING_COUNT; i++) {
        ret.readings[i] = a.readings[i] - b.readings[i];
    }
    return ret;
}

TimingResult PerfTimer::to_result(const PerfTimer& ti, PerfNow delta) {
    size_t count = running_events.size();
    vector<double> results;
    results.resize(count);
    for (size_t i = 0; i < count; i++) {
        results[i] = delta.readings[i];
    }
    return { results };
}

/**
 * modify the event after getting in back from resolve event, e.g.,
 * to remove the period, exclude kernel mode if necessary, etc
 */
void fixup_event(perf_event_attr* attr, bool user_only) {
    attr->sample_period = 0;
    attr->pinned = 1;
    if (user_only) {
        attr->exclude_kernel = 1;
    }
}

void PerfTimer::init(Context &c, const TimerArgs& args) {
    assert(init_count++ == 0);

    int res = do_read_events();
    if (res) {
        throw std::runtime_error("jevents failed while reading events: " + errno_to_str(res));
    }

    bool user_only = false;

    { // init cycles event
        rdpmc_ctx ctx{};
        struct perf_event_attr cycles_attr = {
            .type = PERF_TYPE_HARDWARE,
            .size = PERF_ATTR_SIZE_VER0,
            .config = PERF_COUNT_HW_CPU_CYCLES
        };
        if (rdpmc_open_attr(&cycles_attr, &ctx, nullptr)) {
            // maybe it failed because we try to get kernel cycles too, and perf_event_paranoid is >= 2
            // so try again excluding kernel to see if that works
            cycles_attr.exclude_kernel = 1;
            if (rdpmc_open_attr(&cycles_attr, &ctx, nullptr)) {
                throw std::runtime_error("rdpmc_open cycles failed, checked stderr for more");
            } else {
                c.out() << "Counting user-mode events only: set /proc/sys/kernel/perf_event_paranoid to 1 or less to count kernel events\n";
                user_only = true;
            }
        }
        running_events.emplace_back(ctx, "Cycles");
    }

    for (auto&e : args.extra_events) {
        if (running_events.size() == PerfNow::READING_COUNT) {
            c.err() << "Event '" << e << "' - check the available events with --list-events" << endl;
        }

        perf_event_attr attr = {};
        if (resolve_event(e.c_str(), &attr)) {
            c.out() << "Unable to resolve event '" << e << "' - check the available events with --list-events" << endl;
        } else {
            fixup_event(&attr, user_only);
            rdpmc_ctx ctx{};
            if (rdpmc_open_attr(&attr, &ctx, nullptr)) {
                c.err() << "Failed to program event '" << e << "' (resolved to '" << perf_attr_to_string(&attr) << "')\n";
            } else {
                c.out() << "Resolved and programmed event '" << e << "' to '" << perf_attr_to_string(&attr) << "', caps:" <<
                        " R:" << ctx.buf->cap_user_rdpmc <<
                        " UT:" << ctx.buf->cap_user_time <<
                        " ZT:" << ctx.buf->cap_user_time_zero << endl;

                running_events.emplace_back(ctx, e);
            }
        }
    }

    assert(running_events.size() <= PerfNow::READING_COUNT);

    for (auto& e : running_events) {
        metric_names_.push_back(e.header);
    }
}

template <typename E>
void printEvents(Context& c, E event_func) {
    table::Table t;
    auto& header_row = t.newRow().add("Name").add("Event string");
    if (c.verbose()) {
        header_row.add("Description");
    }
    auto events  = event_func();
    for (auto e : events) {
        auto& event_row = t.newRow().add(e.name).add(e.perf_string);
        if (c.verbose()) {
            event_row.add(e.desc);
        }
    }
    c.out() << t.str();
}

void PerfTimer::listEvents(Context& c) {
    const char *dashes = "----------------------------------------------\n";
    c.out() << dashes << "Generic perf HW events\n" << dashes << endl;
    printEvents(c, get_perf_events);
    c.out() << "\n\n";
    c.out() << dashes << "Raw CPU-specific PMU events\n" << dashes << endl;
    printEvents(c, get_raw_events);
}

PerfTimer::now_t PerfTimer::now() {
    // there is an unfortunate mismatch between the one TimerInfo instance that is created, and the fact
    // that ::now is static, see issue #62
    PerfNow ret;

    unsigned i = 0;
    for (RunningEvent& e : running_events) {
        ret.readings[i++] = rdpmc_read(&e.ctx);
    }
    return ret;
}

PerfTimer::~PerfTimer() = default;

#endif // USE_PERF_TIMER


