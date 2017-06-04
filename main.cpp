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

using namespace std;
using namespace std::chrono;
using namespace Stats;

constexpr int  NAME_WIDTH = 30;
constexpr int CLOCK_WIDTH =  8;
constexpr int NANOS_WIDTH =  8;

template <typename T>
static inline bool is_pow2(T x) {
	static_assert(std::is_unsigned<T>::value, "must use unsigned integral types");
	return x && !(x & (x - 1));
}


struct TimingResult {
	bool hasNanos_, hasCycles_;
	double nanos_, cycles_;
	TimingResult(double cycles, double nanos) :
		hasNanos_(true), hasCycles_(true), cycles_(cycles), nanos_(nanos) {}
};

/*
 * This class measures cycles indirectly by measuring the wall-time for each test, and then converting
 * that to a cycle count based on a calibration loop performed once at startup.
 */
template <typename CLOCK>
class ClockTimerT {

	/* aka 'cycles per nanosecond */
	static double ghz;

public:

	static int64_t now() {
		return duration_cast<nanoseconds>(CLOCK::now().time_since_epoch()).count();
	}

	static TimingResult to_result(int64_t nanos) {
		return {nanos * ghz, (double)nanos};
	}

};

using ClockTimer = ClockTimerT<high_resolution_clock>;

template <size_t ITERS, typename CLOCK>
void CalcClockRes(const char *name) {

	std::array<nanoseconds::rep, ITERS> results;

	for (int r = 0; r < 3; r++) {
		for (int i = 0; i < ITERS; i++) {
			auto t0 = CLOCK::now();
			auto t1 = CLOCK::now();
			results[i] = duration_cast<nanoseconds>(t1 - t0).count();
		}
	}

	DescriptiveStats stats = get_stats(results.begin(), results.end());

	cout << "Overhead for " << name << ": " << stats << endl;
}

/*
 * Calculate the frequency of the CPU based on timing a tight loop that we expect to
 * take one iteration per cycle.
 *
 * ITERS is the base number of iterations to use: the calibration routine is actually
 * run twice, once with ITERS iterations and once with 2*ITERS, and a delta is used to
 * remove measurement overhead.
 */
template <size_t ITERS, typename CLOCK, size_t TRIES = 3>
double CalcCpuFreq() {
	std::array<nanoseconds::rep, TRIES> results;

	for (int r = 0; r < TRIES; r++) {
		auto t0 = CLOCK::now();
		add_calibration(ITERS);
		auto t1 = CLOCK::now();
		add_calibration(ITERS * 2);
		auto t2 = CLOCK::now();
		results[r] = duration_cast<nanoseconds>((t2 - t1) - (t1 - t0)).count();
	}

	DescriptiveStats stats = get_stats(results.begin(), results.end());

	double ghz = ((double)ITERS / stats.getMedian());
	cout << "Median CPU speed: " << fixed << setw(4) << setprecision(3) << ghz << " GHz" << endl;
	return ghz;
}

template <typename CLOCK>
double ClockTimerT<CLOCK>::ghz = CalcCpuFreq<10000,CLOCK,1000>();


typedef std::function<int64_t (size_t)> time_method_t;  // given a loop count, returns a raw timing result
typedef std::function<void * ()>         arg_method_t;  // generates the argument for the benchmarking function
typedef TimingResult (time_to_result_t)(int64_t);

template <typename CLOCK>
class Timing {
public:
	template <bench_f METHOD>
	static int64_t time_method(size_t loop_count) {
		auto t0 = CLOCK::now();
		METHOD(loop_count);
		auto t1 = CLOCK::now();
		return t1 - t0;
	}
};

/*
 * Like Timing, this implements a time_method_t, but as a member function since it wraps an argument provider
 * method
 */
template <typename CLOCK, bench2_f METHOD>
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
		auto t0 = CLOCK::now();
		METHOD(loop_count, arg);
		auto t1 = CLOCK::now();
		return t1 - t0;
	}
};



template <typename N, typename T>
static void printResultLine(std::ostream &os, N name, T clocks, T nanos) {
	os << setprecision(2) << fixed << setw(NAME_WIDTH) << name << setw(CLOCK_WIDTH) << clocks << setw(NANOS_WIDTH) << nanos << endl;
}

class Benchmark {
	static constexpr int loop_count = 1000;
	static constexpr int     samples = 33;

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

	Benchmark(const Benchmark& other) = default;
	void operator=(const Benchmark& other) = delete;

	std::string getName() const {
		return name_;
	}

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

	void runAndPrint(std::ostream &os) {
		TimingResult timings = getTimings();
		double divisor = ops_per_loop_ * loop_count;
		double clocks = timings.cycles_ / divisor;
		double nanos  = timings.nanos_  / divisor;
		printResultLine(os, getName(), clocks, nanos);
	}
};

template <template<typename> class TIME_METHOD, typename CLOCK>
class BenchmarkMaker {
public:
	template <bench_f BENCH_METHOD>
	static Benchmark make_bench(const char *name, size_t ops_per_loop) {
		return Benchmark{name, ops_per_loop, TIME_METHOD<CLOCK>::template time_method<BENCH_METHOD>, CLOCK::to_result};
	}

	template <bench2_f BENCH_METHOD>
	static Benchmark make_bench(const std::string name, size_t ops_per_loop, std::function<void * ()> arg_provider) {
		Timing2<CLOCK,BENCH_METHOD> timing(arg_provider);
		return Benchmark{name, ops_per_loop, timing, CLOCK::to_result};
	}
};

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

template<typename CLOCK, bench2_f METHOD>
void add_loadstore_benches(vector<Benchmark>& benches, unsigned store_size, const std::string &name) {
	using maker = BenchmarkMaker<Timing, CLOCK>;
	for (ssize_t misalign = 0; misalign < 64; misalign++) {
		std::stringstream ss;
		ss << "Misaligned " << (store_size * 8) << "-bit " << name << " [" << setw(2) << misalign << "]";
		benches.push_back(maker::template make_bench<METHOD>(ss.str(),  128,
				[misalign]() { return misaligned_ptr(64, 64,  misalign); }));
	}
}

template <typename CLOCK>
std::vector<Benchmark> make_benches() {

	using maker = BenchmarkMaker<Timing, CLOCK>;

	auto benches = std::vector<Benchmark>{
		maker::template make_bench<dep_add_rax_rax>  ("Dependent add chain",       128),
		maker::template make_bench<indep_add>        ("Independent add chain",  50 * 8),
		maker::template make_bench<dep_imul128_rax>  ("Dependent imul 64->128",    128),
		maker::template make_bench<dep_imul64_rax>   ("Dependent imul 64->64",     128),
		maker::template make_bench<indep_imul128_rax>("Independent imul 64->128",  128),
		maker::template make_bench<store_same_loc>   ("Same location stores",      128),
		maker::template make_bench<store64_disjoint> ("Disjoint location stores",  128)
	};

	// store throughput tests
	add_loadstore_benches<CLOCK,  store16_any>(benches,  2, "store");
	add_loadstore_benches<CLOCK,  store32_any>(benches,  4, "store");
	add_loadstore_benches<CLOCK,  store64_any>(benches,  8, "store");
	add_loadstore_benches<CLOCK, store128_any>(benches, 16, "store");
	add_loadstore_benches<CLOCK, store256_any>(benches, 32, "store");

	// load throughput benches
	add_loadstore_benches<CLOCK,  load16_any>(benches,  2, "load");
	add_loadstore_benches<CLOCK,  load32_any>(benches,  4, "load");
	add_loadstore_benches<CLOCK,  load64_any>(benches,  8, "load");
	add_loadstore_benches<CLOCK, load128_any>(benches, 16, "load");
	add_loadstore_benches<CLOCK, load256_any>(benches, 32, "load");

	return benches;
}


std::vector<Benchmark> benches = make_benches<ClockTimer>();

void listBenches() {
	cout << "Found " << benches.size() << " benchmarks" << endl;
	for (auto& b : benches) {
		cout << b.getName() << endl;
	}
}

void runAll() {
	cout << "Running " << benches.size() << " benchmarks" << endl;
	printResultLine(std::cout, "Benchmark", "Cycles", "Nanos");
	for (auto& b : benches) {
		b.runAndPrint(cout);
	}
}

int main(int argc, char **argv) {
	cout << "Welcome to uarch-bench (" << GIT_VERSION << ")" << endl;
	CalcClockRes<100,system_clock>("system_clock");
	CalcClockRes<100,steady_clock>("steady_clock");
	CalcClockRes<100,high_resolution_clock>("hi_res_clock");

	runAll();

	return EXIT_SUCCESS;
}


