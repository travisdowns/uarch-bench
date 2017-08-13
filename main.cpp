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
#include "timer-info.hpp"
#include "context.h"
#include "benches.h"
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

/*
 * Calculate the frequency of the CPU based on timing a tight loop that we expect to
 * take one iteration per cycle.
 *
 * ITERS is the base number of iterations to use: the calibration routine is actually
 * run twice, once with ITERS iterations and once with 2*ITERS, and a delta is used to
 * remove measurement overhead.
 */
template <size_t ITERS, typename CLOCK, size_t TRIES = 10, size_t WARMUP = 100>
double CalcCpuFreq() {
    std::array<nanoseconds::rep, TRIES> results;

    for (size_t w = 0; w < WARMUP + 1; w++) {
        for (size_t r = 0; r < TRIES; r++) {
            auto t0 = CLOCK::now();
            add_calibration(ITERS);
            auto t1 = CLOCK::now();
            add_calibration(ITERS * 2);
            auto t2 = CLOCK::now();
            results[r] = duration_cast<nanoseconds>((t2 - t1) - (t1 - t0)).count();
        }
    }

    DescriptiveStats stats = get_stats(results.begin(), results.end());

    double ghz = ((double)ITERS / stats.getMedian());
    return ghz;
}

template <typename CLOCK>
double ClockTimerT<CLOCK>::ghz = CalcCpuFreq<10000,CLOCK,1000>();


template <typename T>
static void printAlignedMetrics(Context &c, const std::vector<T> metrics) {
    const TimerInfo &ti = c.getTimerInfo();
    assert(ti.getMetricNames().size() == metrics.size());
    for (size_t i = 0; i < metrics.size(); i++) {
        // the width is either the expected max width of the value, or the with of the name, plus COLUMN_PAD
        unsigned int width = std::max(ti.getMetricNames().size(), 4UL + c.getPrecision()) + COLUMN_PAD;
        c.out() << setw(width) << metrics[i];
    }
}


static void printResultLine(Context& c, const std::string& benchName, const TimingResult &result) {
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


TimingResult Benchmark::getTimings() {
    auto b = getBench();

    std::array<int64_t, samples> raw_results;
    // warmup
    b(loop_count);
    b(loop_count);
    for (int i = 0; i < samples; i++) {
        raw_results[i] = b(loop_count);
    }

    auto aggr = *std::min_element(raw_results.begin(), raw_results.end());
    return time_to_result_(aggr);
}

TimingResult Benchmark::run() {
    TimingResult timings = getTimings();
    double multiplier = 1.0 / (ops_per_loop_ * loop_count); // normalize to time / op
    return timings * multiplier;
}

void Benchmark::runAndPrint(Context& c) {
    TimingResult timing = run();
    printResultLine(c, getName(), timing);
}

void BenchmarkGroup::runAll(Context &context, const TimerInfo &ti) {
    context.out() << std::endl << "** Running benchmark group " << getName() << " **" << std::endl;
    printResultHeader(context, ti);
    for (auto b : benches_) {
        b.runAndPrint(context);
    }
}








template <typename TIMER>
BenchmarkList make_benches() {

	BenchmarkList groupList;

	register_default<TIMER>(groupList);
	register_loadstore<TIMER>(groupList);

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

	void runAll(Context &c) {
		cout << "Running " << getBenches().size() << " benchmark groups using timer " << timer_info_->getName() << endl;
		for (auto& group : getBenches()) {
			group->runAll(c, getTimerInfo());
		}
	}

	template <typename TIMER>
	static TimeredList create(const TIMER& ti) {
		return TimeredList(std::unique_ptr<TIMER>(new TIMER(ti)), make_benches<TIMER>());
	}


	static std::vector<TimeredList>& getAll() {
	    if (all_.empty()) {
	        all_.push_back(TimeredList::create(ClockTimerT<high_resolution_clock>("high_resolution_clock")));
#if USE_LIBPFC
	        all_.push_back(TimeredList::create(LibpfcTimer()));
#endif
	    }
	    return all_;
	}
};

std::vector<TimeredList> TimeredList::all_;

TimeredList& getForTimer(args::ValueFlag<std::string>& timerArg) {
	std::string timerName = timerArg ? timerArg.Get() : "ClockTimer";
	std::vector<TimeredList>& all = TimeredList::getAll();
	for (auto& i : all) {
		if (i.getTimerInfo().getName() == timerName) {
			return i;
		}
	}

	throw args::UsageError(string("No timer with name ") + timerName);
}

void Context::listBenches() {
	auto benchList = TimeredList::getAll().front().getBenches();
	cout << "Listing " << benchList.size() << " benchmark groups" << endl << endl;
	for (auto& group : benchList) {
		cout << "Benchmark group: " << group->getName() << endl;
		for (auto& bench : group->getAllBenches()) {
			cout << bench.getName() << endl;
		}
		cout << endl;
	}
}

void printClockOverheads() {
	constexpr int cw = 22;
	cout << "Clock overhead: " << setw(cw) << "system_clock" << setw(cw) << "steady_clock" << setw(cw) << "hi_res_clock" << endl;
	cout << "min/med/avg/max ";
	cout << setw(cw) << CalcClockRes<100,system_clock>().getString4(1);
	cout << setw(cw) << CalcClockRes<100,steady_clock>().getString4(1);
	cout << setw(cw) << CalcClockRes<100,high_resolution_clock>().getString4(1);
	cout << endl;
}

/* list the avaiable timers on stdout */
void Context::listTimers() {
	cout << endl << "Available timers:" << endl << endl;
	for (auto& tl : TimeredList::getAll()) {
		auto& ti = tl.getTimerInfo();
		cout << ti.getName() << endl << "\t" << ti.getDesciption() << endl << endl;
	}
}

int Context::run() {
    if (arg_listtimers) {
        listTimers();
    } else if (arg_listbenches) {
        listBenches();
    } else if (arg_clockoverhead) {
		printClockOverheads();
    } else {
        TimeredList& toRun = getForTimer(arg_timer);
        timer_info_ = &toRun.getTimerInfo();
        timer_info_->init(*this);
        toRun.runAll(*this);
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
	cout << "Welcome to uarch-bench (" << GIT_VERSION << ")" << endl;

	try {
		Context context(argc, argv, &std::cout);

		cout << "Median CPU speed: " << fixed << setw(4) << setprecision(3) << ClockTimerT<high_resolution_clock>::getGHz() << " GHz" << endl;

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


