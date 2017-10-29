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

#include "stats.hpp"
#include "version.hpp"
#include "asm_methods.h"
#include "args.hxx"
#include "benches.hpp"
#include "timer-info.hpp"
#include "context.hpp"
#include "util.hpp"
#include "timers.hpp"

using namespace std;
using namespace std::chrono;
using namespace Stats;

constexpr int  NAME_WIDTH = 30;
constexpr int  COLUMN_PAD =  3;

template <typename T>
static inline bool is_pow2(T x) {
    static_assert(std::is_unsigned<T>::value, "must use unsigned integral types");
    return x && !(x & (x - 1));
}

const int MAX_ALIGN = 4096;
const int STORAGE_SIZE = 4 * MAX_ALIGN;  // * 4 because we overalign the pointer in order to guarantee minimal alignemnt
unsigned char unaligned_storage[STORAGE_SIZE];

/*
 * Returns a pointer that is minimally aligned to base_alignment. That is, it is
 * aligned to base_alignment, but *not* aligned to 2 * base_alignment.
 */
void *aligned_ptr(size_t base_alignment, size_t required_size) {
    assert(is_pow2(base_alignment));
    void *p = unaligned_storage;
    size_t space = STORAGE_SIZE;
    void *r = std::align(base_alignment, required_size, p, space);
    assert(r);
    assert((((uintptr_t)r) & (base_alignment - 1)) == 0);
    return r;
}

/**
 * Returns a pointer that is first *minimally* aligned to the given base alignment (per
 * aligned_ptr()) and then is offset by the about given in misalignment.
 */
void *misaligned_ptr(size_t base_alignment, size_t required_size, ssize_t misalignment) {
    char *p = static_cast<char *>(aligned_ptr(base_alignment, required_size));
    return p + misalignment;
}


template <size_t ITERS, typename CLOCK>
DescriptiveStats CalcClockRes() {
    std::array<nanoseconds::rep, ITERS> results;

    for (int r = 0; r < 3; r++) {
        for (size_t i = 0; i < ITERS; i++) {
            auto t0 = CLOCK::now();
            auto t1 = CLOCK::now();
            results[i] = duration_cast<nanoseconds>(t1 - t0).count();
        }
    }

    return get_stats(results.begin(), results.end());
}


template <typename T>
static void printAlignedMetrics(Context &c, const std::vector<T>& metrics) {
    const TimerInfo &ti = c.getTimerInfo();
    assert(ti.getMetricNames().size() == metrics.size());
    for (size_t i = 0; i < metrics.size(); i++) {
        // the width is either the expected max width of the value, or the with of the name, plus COLUMN_PAD
        unsigned int width = std::max(ti.getMetricNames().size(), 4UL + c.getPrecision()) + COLUMN_PAD;
        c.out() << setw(width) << metrics[i];
    }
}


static void printResultLine(Context& c, const std::string& benchName, const TimingResult& result) {
    std::ostream& os = c.out();
    os << setprecision(c.getPrecision()) << fixed << setw(NAME_WIDTH) << benchName;
    printAlignedMetrics(c, result.getResults());
    os << endl;
}


static void printResultHeader(Context& c, const TimerInfo& ti) {
    // "Benchmark", "Cycles", "Nanos"
    c.out() << setw(NAME_WIDTH) << "Benchmark";
    printAlignedMetrics(c, ti.getMetricNames());
    c.out() << endl;
}


Benchmark::Benchmark(const std::string& name, size_t ops_per_loop, full_bench_t full_bench, uint32_t loop_count) :
        name_(name), ops_per_loop_(ops_per_loop), full_bench_(full_bench), loop_count_(loop_count) {}

TimingResult Benchmark::getTimings() {
    return full_bench_(getLoopCount());
}

TimingResult Benchmark::run() {
    TimingResult timings = getTimings();
    double multiplier = 1.0 / (ops_per_loop_ * getLoopCount()); // normalize to time / op
    return timings * multiplier;
}

void Benchmark::runAndPrint(Context& c) {
    TimingResult timing = run();
    printResultLine(c, getName(), timing);
}

void BenchmarkGroup::runAll(Context &context, const TimerInfo &ti, const predicate_t& predicate) {
    context.out() << std::endl << "** Running benchmark group " << getName() << " **" << std::endl;
    printResultHeader(context, ti);
    for (auto& b : benches_) {
        if (predicate(b)) {
            b.runAndPrint(context);
        }
    }
}

template <typename TIMER>
BenchmarkList make_benches() {

    BenchmarkList groupList;

    register_default<TIMER>(groupList);
    register_loadstore<TIMER>(groupList);
    register_misc<TIMER>(groupList);

    return groupList;
}

/*
 * This object binds together a particular TIMER implementation (and its corresponding ClockInfo object)
 */
class TimeredList {

    static std::vector<TimeredList> all_;

    std::unique_ptr<TimerInfo> timer_info_;
    BenchmarkList benches_;

public:
    TimeredList(std::unique_ptr<TimerInfo>&& timer_info, const BenchmarkList& benches)
: timer_info_(std::move(timer_info)), benches_(benches) {}

    TimeredList(const TimeredList &) = delete;
    TimeredList(TimeredList &&) = default;

    ~TimeredList() = default;

    TimerInfo& getTimerInfo() {
        return *timer_info_;
    }

    const BenchmarkList& getBenches() const {
        return benches_;
    }

    void runAll(Context &c, const predicate_t& predicate) {
        cout << "Running " << getBenches().size() << " benchmark groups using timer " << timer_info_->getName() << endl;
        for (auto& group : getBenches()) {
            group->runAll(c, getTimerInfo(), predicate);
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
    auto benchList = TimeredList::getAll(c).front().getBenches();
    out << "Listing " << benchList.size() << " benchmark groups" << endl << endl;
    for (auto& group : benchList) {
        out << "Benchmark group: " << group->getName() << endl;
        for (auto& bench : group->getAllBenches()) {
            out << bench.getName() << endl;
        }
        out << endl;
    }
}

void printClockOverheads(Context& c) {
    constexpr int cw = 22;
    std::ostream& out = c.out();
    out << "Clock overhead: " << setw(cw) << "system_clock" << setw(cw) << "steady_clock" << setw(cw) << "hi_res_clock" << endl;
    out << "min/med/avg/max ";
    out << setw(cw) << CalcClockRes<100,system_clock>().getString4(1);
    out << setw(cw) << CalcClockRes<100,steady_clock>().getString4(1);
    out << setw(cw) << CalcClockRes<100,high_resolution_clock>().getString4(1);
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
        if (arg_test_name) {
            pred =  [this](const Benchmark& b){ return b.getName().find(arg_test_name.Get()) != std::string::npos; };
        } else {
            pred =  [](const Benchmark&){ return true; };
        }

        toRun.runAll(*this, pred);
    }
}


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


