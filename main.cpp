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

#if USE_LIBPFC
#include "libpfc/include/libpfc.h"
#include "libpfc-timer.hpp"
#endif

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



/*
 * This class measures cycles indirectly by measuring the wall-time for each test, and then converting
 * that to a cycle count based on a calibration loop performed once at startup.
 */
template <typename CLOCK>
class ClockTimerT : public TimerInfo {

	/* aka 'cycles per nanosecond */
	static double ghz;

public:

	ClockTimerT(std::string clock_name) : TimerInfo("ClockTimer", string("Use the system clock (" + clock_name
			+ ") to measure wall-clock time, and convert to cycles using a calibration loop"),
	        {"Cycles", "Nanos"}) {}

	virtual void init(Context &context) {
	}

	static int64_t now() {
		return duration_cast<nanoseconds>(CLOCK::now().time_since_epoch()).count();
	}

	static TimingResult to_result(int64_t nanos) {
		return TimingResult({nanos * ghz, (double)nanos});
	}

	/* return the statically calculated clock speed of the CPU in ghz for this clock */
	static double getGHz() {
		return ghz;
	}
};

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


typedef std::function<int64_t (size_t)> time_method_t;  // given a loop count, returns a raw timing result
typedef std::function<void * ()>         arg_method_t;  // generates the argument for the benchmarking function
typedef TimingResult (time_to_result_t)(int64_t);

template <typename TIMER>
class Timing {
public:
	template <bench_f METHOD>
	static int64_t time_method(size_t loop_count) {
		auto t0 = TIMER::now();
		METHOD(loop_count);
		auto t1 = TIMER::now();
		return t1 - t0;
	}
};

/*
 * Like Timing, this implements a time_method_t, but as a member function since it wraps an argument provider
 * method
 */
template <typename TIMER, bench2_f METHOD>
class Timing2 {
	arg_method_t arg_method_;
	void* arg_;
public:
	Timing2(arg_method_t arg_method) :
		arg_method_(arg_method), arg_(arg_method()) {}

	int64_t operator()(size_t loop_count) {
		return time_inner(loop_count, arg_);
	}

	static int64_t time_inner(size_t loop_count, void* arg) {
		auto t0 = TIMER::now();
		METHOD(loop_count, arg);
		auto t1 = TIMER::now();
		return t1 - t0;
	}
};

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

class Benchmark final {
	static constexpr int loop_count = 1000;
	static constexpr int    samples = 33;

	std::string name_;
	/* how many operations are involved in one iteration of the benchmark loop */
	size_t ops_per_loop_;
	time_method_t bench_method_;
	time_to_result_t *time_to_result_;

protected:
	time_method_t getBench() const {
		return bench_method_;
	}

public:
	Benchmark(std::string name, size_t ops_per_loop, time_method_t bench_method, time_to_result_t *time_to_result) :
		name_(name), ops_per_loop_(ops_per_loop), bench_method_(bench_method), time_to_result_(time_to_result) {}

	std::string getName() const {
		return name_;
	}

	/* get the raw timings for a full run of the underlying benchmark, doesn't normalize for loop_count or ops_per_loop */
	TimingResult getTimings() {
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

	/* like getTimings, except that everything is normalized, so the results should reflect the cost for a single operation */
	TimingResult run() {
		TimingResult timings = getTimings();
		double multiplier = 1.0 / (ops_per_loop_ * loop_count); // normalize to time / op
		return timings * multiplier;
	}

	void runAndPrint(Context& c) {
		TimingResult timing = run();
		printResultLine(c, getName(), timing);
	}
};

/**
 * Interface for a group of benchmarks. The group itself has a name, and can run and output all the contained
 * benchmarks.
 */
class BenchmarkGroup {
	std::string name_;
	std::vector<Benchmark> benches_;

public:
	BenchmarkGroup(std::string name) : name_(name) {}

	virtual ~BenchmarkGroup() {}

	virtual void runAll(Context &context, const TimerInfo &ti) {
		context.out() << endl << "** Running benchmark group " << getName() << " **" << endl;
		printResultHeader(context, ti);
		for (auto b : benches_) {
			b.runAndPrint(context);
		}
	}

	virtual void add(const std::vector<Benchmark> &more) {
		benches_.insert(benches_.end(), more.begin(), more.end());
	}

	virtual void add(const Benchmark &bench) {
		benches_.push_back(bench);
	}

	virtual const std::vector<Benchmark>& getAllBenches() const {
		return benches_;
	}

	const std::vector<Benchmark>& getBenches() const {
		return benches_;
	}

	const std::string& getName() const {
		return name_;
	}
};

template <template<typename> class TIME_METHOD, typename TIMER>
class BenchmarkMaker {
public:
	template <bench_f BENCH_METHOD>
	static Benchmark make_bench(const char *name, size_t ops_per_loop) {
		return Benchmark{name, ops_per_loop, TIME_METHOD<TIMER>::template time_method<BENCH_METHOD>, TIMER::to_result};
	}

	template <bench2_f BENCH_METHOD>
	static Benchmark make_bench(const std::string name, size_t ops_per_loop, std::function<void * ()> arg_provider) {
		Timing2<TIMER,BENCH_METHOD> timing(arg_provider);
		return Benchmark{name, ops_per_loop, timing, TIMER::to_result};
	}
};

/*
 * A specialization of BenchmarkGroup that outputs its results in a 4 x 16 grid for all 64 possible
 * offsets within a 64B cache line.
 */
class LoadStoreGroup : public BenchmarkGroup {
	static constexpr unsigned DEFAULT_ROWS =  4;
	static constexpr unsigned DEFAULT_COLS = 16;

	unsigned rows_, cols_, total_cells_, op_size_;
public:
	LoadStoreGroup(string name, unsigned op_size, unsigned rows, unsigned cols)
	: BenchmarkGroup(name), rows_(rows), cols_(cols), total_cells_(rows * cols), op_size_(op_size) {
		assert(rows < 10000 && cols < 10000);
	}

	template<typename TIMER, bench2_f METHOD>
	static shared_ptr<LoadStoreGroup> make(string name, unsigned op_size) {
		shared_ptr<LoadStoreGroup> group = make_shared<LoadStoreGroup>(name, op_size, DEFAULT_ROWS, DEFAULT_COLS);
		using maker = BenchmarkMaker<Timing, TIMER>;
		for (ssize_t misalign = 0; misalign < 64; misalign++) {
			std::stringstream ss;
			ss << "Misaligned " << (op_size * 8) << "-bit " << name << " [" << setw(2) << misalign << "]";
			group->add(maker::template make_bench<METHOD>(ss.str(),  128,
					[misalign]() { return misaligned_ptr(64, 64,  misalign); }));
		}
		return group;
	}

	virtual void runAll(Context& c, const TimerInfo &ti) override {
	    std::ostream& os = c.out();
		os << endl << "** Inverse throughput for " << getName() << " **" << endl;

		// column headers
		os << "offset  ";
		for (unsigned col = 0; col < cols_; col++) {
			os << setw(5) << col;
		}
		os << endl;

		auto benches = getBenches();
		assert(benches.size() == rows_ * cols_);

		// collect all the results up front, before any output
		vector<double> results(benches.size());
		for (size_t i = 0; i < benches.size(); i++) {
			results[i] = benches[i].run().getCycles();
		}

		for (unsigned row = 0, i = 0; row < rows_; row++) {
			os << setw(3) << (row * cols_) << " :   ";
			for (unsigned col = 0; col < cols_; col++, i++) {
				os << setprecision(1) << fixed << setw(5) << results[i];
			}
			os << endl;
		}
	}
};

constexpr unsigned LoadStoreGroup::DEFAULT_ROWS;
constexpr unsigned LoadStoreGroup::DEFAULT_COLS;

using BenchmarkList = std::vector<shared_ptr<BenchmarkGroup>>;


template <typename TIMER>
BenchmarkList make_benches() {

	BenchmarkList groupList;

	shared_ptr<BenchmarkGroup> default_group = std::make_shared<BenchmarkGroup>("default");

	using default_maker = BenchmarkMaker<Timing, TIMER>;

	auto benches = std::vector<Benchmark>{
		default_maker::template make_bench<dep_add_rax_rax>  ("Dependent add chain",       128),
		default_maker::template make_bench<indep_add>        ("Independent add chain",  50 * 8),
		default_maker::template make_bench<dep_imul128_rax>  ("Dependent imul 64->128",    128),
		default_maker::template make_bench<dep_imul64_rax>   ("Dependent imul 64->64",     128),
		default_maker::template make_bench<indep_imul128_rax>("Independent imul 64->128",  128),
		default_maker::template make_bench<store_same_loc>   ("Same location stores",      128),
		default_maker::template make_bench<store64_disjoint> ("Disjoint location stores",  128)
	};

	default_group->add(benches);
	groupList.push_back(default_group);

	// load throughput benches
	groupList.push_back(LoadStoreGroup::make<TIMER,  load16_any>("load/16-bit",  2));
	groupList.push_back(LoadStoreGroup::make<TIMER,  load32_any>("load/32-bit",  4));
	groupList.push_back(LoadStoreGroup::make<TIMER,  load64_any>("load/64-bit",  8));
	groupList.push_back(LoadStoreGroup::make<TIMER, load128_any>("load/128-bit", 16));
	groupList.push_back(LoadStoreGroup::make<TIMER, load256_any>("load/256-bit", 32));

	// store throughput
	groupList.push_back(LoadStoreGroup::make<TIMER,  store16_any>( "store/16-bit",  2));
	groupList.push_back(LoadStoreGroup::make<TIMER,  store32_any>( "store/32-bit",  4));
	groupList.push_back(LoadStoreGroup::make<TIMER,  store64_any>( "store/64-bit",  8));
	groupList.push_back(LoadStoreGroup::make<TIMER, store128_any>("store/128-bit", 16));
	groupList.push_back(LoadStoreGroup::make<TIMER, store256_any>("store/256-bit", 32));

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


