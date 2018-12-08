/*
 * context.cpp
 */

#include "context.hpp"
#include "benchmark.hpp"
#include "timers.hpp"
#include "matchers.hpp"

#include <iostream>

using namespace std;

/*
 * Each timer might have specific arguments it wants to expose to the user, here we add them to the parser.
 */
void addTimerSpecificArgs(args::ArgumentParser& parser) {
#define ADD_TIMER(TIMER) TIMER::addCustomArgs(parser);
    ALL_TIMERS_X(ADD_TIMER);
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

    verbose_ = arg_verbose;
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
    register_syscall<TIMER>(groupList);

    return groupList;
}

/*
 * A list of benchmarks which binds together a particular TIMER implementation (and its corresponding ClockInfo object), with all
 * benchmarks.
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
            group->runIf(c, predicate);
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
#if USE_PERF_TIMER
            all_.push_back(TimeredList::create<PerfTimer>(c));
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


/* list the avaiable timers on stdout */
void listTimers(Context& c) {
    cout << endl << "Available timers:" << endl << endl;
    for (auto& tl : TimeredList::getAll(c)) {
        auto& ti = tl.getTimerInfo();
        c.out() << ti.getName() << endl << "\t" << ti.getDesciption() << endl << endl;
    }
}

/*
 * Allow each timer to handle their specific args before starting any benchmark - e.g., for arguments that
 * just want to list some configuration then exit (throw SilentSuccess in this case).
 */
void handleTimerSpecificRun(Context& c) {
#define HANDLE_TIMER(TIMER) TIMER::customRunHandler(c);
    ALL_TIMERS_X(HANDLE_TIMER);
}

TimerArgs Context::getTimerArgs() {
    return { arg_extraevents.Get() };
}

/** get the first available CPU based on the affinity mask */
int getFirstAvailableCpu() {
    cpu_set_t cpu_set;
    if (sched_getaffinity(0, sizeof(cpu_set), &cpu_set)) {
        throw std::runtime_error("failed while getting existing cpu affinity: " + errno_to_str(errno));
    }
    for (int cpu = 0; cpu < CPU_SETSIZE; cpu++) {
        if (CPU_ISSET(cpu, &cpu_set)) {
            return cpu;
        }
    }
    throw std::runtime_error("not allowed to run on any CPUs - impossible?");
}

void pinToThread(Context& c, int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset)) {
        throw std::runtime_error("failed to pin: " + errno_to_str(errno));
    }
    c.log() << "Pinned to CPU " << cpu << endl;
}

void Context::run() {

    handleTimerSpecificRun(*this);

    if (arg_listtimers) {
        listTimers(*this);
    } else if (arg_listbenches) {
        listBenches(*this);
    } else if (arg_clockoverhead) {
        printClockOverheads(this->out());
    } else if (arg_internal_dump_timer) {
        std::cout << getTimerName();
        throw SilentSuccess();
    } else {
        // pinning should happen early since some timers rely on it in their init phase
        pinToThread(*this, arg_pincpu ? arg_pincpu.Get() : getFirstAvailableCpu());

        TimeredList& toRun = getForTimer(*this);
        timer_info_ = &toRun.getTimerInfo();

        // after this point, the timer_info_ field is initialized, so if your behavior needs that, put it here
        if (arg_listevents) {
            timer_info_->listEvents(*this);
        } else {
            timer_info_->init(*this);
            predicate_t pred;
            if (!arg_test_tag && !arg_test_name) {
                // no predicates specified on the command line, use tag=* as default predicate
                TagMatcher matcher{"default"};
                pred = [matcher](const Benchmark& b){ return matcher(b->getTags()); };
            } else {
                // otherwise AND-together all set predicates
                pred = [](const Benchmark& b){ return true; };
                if (arg_test_name) {
                    pred = [this](const Benchmark& b){ return wildcard_match(b->getPath(), arg_test_name.Get()); };
                }
                if (arg_test_tag) {
                    TagMatcher matcher{arg_test_tag.Get()};
                    pred = pred_and(pred, [=](const Benchmark& b){ return matcher(b->getTags()); });
                }
            }

            toRun.runIf(*this, pred);
        }
    }
}




