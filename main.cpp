/*
 * main.cpp
 */

#include <iostream>
#include <iomanip>
#include <memory>
#include <chrono>
#include <cstddef>
#include <new>
#include <cassert>
#include <type_traits>
#include <sstream>
#include <functional>
#include <cstring>
#include <cstdlib>

#include <sys/mman.h>

#if USE_BACKWARD_CPP
#include "backward-cpp/backward.hpp"
#endif

#include "stats.hpp"
#include "version.hpp"
#include "args.hxx"
#include "benches.hpp"
#include "timer-info.hpp"
#include "context.hpp"
#include "util.hpp"
#include "timers.hpp"

using namespace std;
using namespace std::chrono;
using namespace Stats;

template <size_t ITERS, typename CLOCK>
DescriptiveStats CalcClockRes() {
    std::array<nanoseconds::rep, ITERS> results;

    for (int r = 0; r < 3; r++) {
        for (size_t i = 0; i < ITERS; i++) {
            auto t0 = CLOCK::nanos();
            auto t1 = CLOCK::nanos();
            results[i] = t1 - t0;
        }
    }

    return get_stats(results.begin(), results.end());
}

volatile int64_t sink;

template <size_t ITERS, typename CLOCK>
DescriptiveStats CalcClockCost() {
    std::array<double, ITERS> results;
    using timer = DefaultClockTimer;

    for (int r = 0; r < 3; r++) {
        for (size_t i = 0; i < ITERS; i++) {
            int64_t sum = 0;
            int64_t before = timer::now();
            for (int j = 0; j < 1000; j++) {
                sum += CLOCK::nanos();
            }
            results[i] = (timer::now() - before) / 1000.0;
            sink = sum;
        }
    }

    return get_stats(results.begin(), results.end());
}



template <typename TIMER>
GroupList make_benches() {

    GroupList groupList;

    register_default<TIMER>(groupList);
    register_loadstore<TIMER>(groupList);
    register_mem<TIMER>(groupList);
    register_misc<TIMER>(groupList);
    register_cpp<TIMER>(groupList);
    register_vector<TIMER>(groupList);
    register_call<TIMER>(groupList);
    register_oneshot<TIMER>(groupList);

    return groupList;
}

/*
 * This object binds together a particular TIMER implementation (and its corresponding ClockInfo object)
 */
class TimeredList {

    static std::vector<TimeredList> all_;

    std::unique_ptr<TimerInfo> timer_info_;
    GroupList groups_;

public:
    TimeredList(std::unique_ptr<TimerInfo>&& timer_info, const GroupList& benches)
: timer_info_(std::move(timer_info)), groups_(benches) {}

    TimeredList(const TimeredList &) = delete;
    TimeredList(TimeredList &&) = default;

    ~TimeredList() = default;

    TimerInfo& getTimerInfo() {
        return *timer_info_;
    }

    const GroupList& getGroups() const {
        return groups_;
    }

    void runIf(Context &c, const predicate_t& predicate) {
        cout << "Running benchmarks groups using timer " << timer_info_->getName() << endl;
        for (auto& group : getGroups()) {
            group->runIf(c, getTimerInfo(), predicate);
        }
    }

    template <typename TIMER, typename... Args>
    static TimeredList create(Args&&... args) {
        auto t = new TIMER(std::forward<Args>(args)...);
        return TimeredList(std::unique_ptr<TIMER>(t), make_benches<TIMER>());
    }


    static std::vector<TimeredList>& getAll(Context& c) {
        if (all_.empty()) {
            all_.push_back(TimeredList::create<DefaultClockTimer>("high_resolution_clock"));
#if USE_LIBPFC
            all_.push_back(TimeredList::create<LibpfcTimer>(c));
#endif
        }
        return all_;
    }
};

std::vector<TimeredList> TimeredList::all_;

TimeredList& getForTimer(Context &c) {
    std::string timerName = c.getTimerName();
    std::vector<TimeredList>& all = TimeredList::getAll(c);
    for (auto& i : all) {
        if (i.getTimerInfo().getName() == timerName) {
            return i;
        }
    }

    throw args::UsageError(string("No timer with name ") + timerName);
}

void listBenches(Context& c) {
    std::ostream& out = c.out();
    // note that we get the timer-specific list, since not all timers have identical benchmark lists mostly
    // due to "raw" benchmarks which are written with a specific timer in mind
    auto benchList = getForTimer(c).getGroups();
    out << "Listing " << benchList.size() << " benchmark groups" << endl << endl;
    for (auto& group : benchList) {
        out << "-------------------------------------\n";
        out << "Benchmark group: " << group->getId() << "\n" << group->getDescription() << endl;
        out << "-------------------------------------\n";
        group->printBenches(out);
        out << endl;
    }
}


template <typename CLOCK>
void printOneClock(std::ostream& out, const char* name) {
    out << setw(48) << name << setw(28) << CalcClockRes<100,CLOCK>().getString4(5,1);
    out << setw(30) << CalcClockCost<100,CLOCK>().getString4(5,1) << endl;

}

struct DumbClock {
    static int64_t nanos() { return 0; }
};

void printClockOverheads(Context& c) {
    std::ostream& out = c.out();
    out << "----- Clock Stats --------\n";
    out << "                                                      Resolution (ns)               Runtime (ns)" << endl;
    out << "                           Name                        min/  med/  avg/  max         min/  med/  avg/  max" << endl;
#define PRINT_CLOCK(clock) printOneClock< clock >(out, #clock);
    PRINT_CLOCK(StdClockAdapt<system_clock>);
    PRINT_CLOCK(StdClockAdapt<steady_clock>);
    PRINT_CLOCK(StdClockAdapt<high_resolution_clock>);

    PRINT_CLOCK(GettimeAdapter<CLOCK_REALTIME>);
    PRINT_CLOCK(GettimeAdapter<CLOCK_REALTIME_COARSE>);
    PRINT_CLOCK(GettimeAdapter<CLOCK_MONOTONIC>);
    PRINT_CLOCK(GettimeAdapter<CLOCK_MONOTONIC_COARSE>);
    PRINT_CLOCK(GettimeAdapter<CLOCK_MONOTONIC_RAW>);
    PRINT_CLOCK(GettimeAdapter<CLOCK_PROCESS_CPUTIME_ID>);
    PRINT_CLOCK(GettimeAdapter<CLOCK_THREAD_CPUTIME_ID>);
    PRINT_CLOCK(GettimeAdapter<CLOCK_BOOTTIME>);

    PRINT_CLOCK(DumbClock);

    out << endl;
}

/* list the avaiable timers on stdout */
void listTimers(Context& c) {
    cout << endl << "Available timers:" << endl << endl;
    for (auto& tl : TimeredList::getAll(c)) {
        auto& ti = tl.getTimerInfo();
        c.out() << ti.getName() << endl << "\t" << ti.getDesciption() << endl << endl;
    }
}

/*
 * Each timer might have specific arguments it wants to expose to the user, here we add them to the parser.
 */
void addTimerSpecificArgs(args::ArgumentParser& parser) {
#define ADD_TIMER(TIMER) TIMER::addCustomArgs(parser);
    ALL_TIMERS_X(ADD_TIMER);
}

/*
 * Allow each timer to handle their specific args before starting any benchmark - e.g., for arguments that
 * just want to list some configuration then exit (throw SilentSuccess in this case).
 */
void handleTimerSpecificRun(Context& c) {
#define HANDLE_TIMER(TIMER) TIMER::customRunHandler(c);
    ALL_TIMERS_X(HANDLE_TIMER);
}

Context::Context(int argc, char **argv, std::ostream *out)
        : err_(out), log_(out), out_(out), argc_(argc), argv_(argv) {
    try {
        addTimerSpecificArgs(parser);
        parser.ParseCLI(argc, argv);
    } catch (args::Help&) {
        log() << parser;
        throw SilentSuccess();
    } catch (args::ParseError& e) {
        err() << "ERROR: " << e.what() << std::endl << parser;
        throw SilentFailure();
    } catch (args::UsageError & e) {
        err() << "ERROR: " << e.what() << std::endl;
        throw SilentFailure();
    }
}

static bool tag_matches(const Benchmark& b, const std::string& pattern) {
    for (auto& tag : b->getTags()) {
        if (wildcard_match(tag, pattern)) {
            return true;
        }
    }
    return false;
}

void Context::run() {

    handleTimerSpecificRun(*this);

    if (arg_listtimers) {
        listTimers(*this);
    } else if (arg_listbenches) {
        listBenches(*this);
    } else if (arg_clockoverhead) {
        printClockOverheads(*this);
    } else if (arg_internal_dump_timer) {
        std::cout << getTimerName();
        throw SilentSuccess();
    } else {
        TimeredList& toRun = getForTimer(*this);
        timer_info_ = &toRun.getTimerInfo();
        timer_info_->init(*this);
        predicate_t pred;
        if (!arg_test_tag && !arg_test_name) {
            // no predicates specified on the command line, use tag=* as default predicate
            pred = [](const Benchmark& b){ return tag_matches(b, "default"); };
        } else {
            // otherwise AND-together all set predicates
            pred = [](const Benchmark& b){ return true; };
            if (arg_test_name) {
                pred = [this](const Benchmark& b){ return wildcard_match(b->getPath(), arg_test_name.Get()); };
            }
            if (arg_test_tag) {
                pred = pred_and(pred, [this](const Benchmark& b){ return tag_matches(b, arg_test_tag.Get()); });
            }
        }

        toRun.runIf(*this, pred);
    }
}

#if USE_BACKWARD_CPP
backward::SignalHandling sh;
#endif

int main(int argc, char **argv) {
    cout << "Welcome to uarch-bench (" << GIT_VERSION << ")" << endl;

    try {
        Context context(argc, argv, &std::cout);
        context.run();
    } catch (SilentSuccess& e) {
    } catch (SilentFailure& e) {
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
