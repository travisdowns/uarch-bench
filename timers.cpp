/*
 * timers.cpp
 *
 * Implementation for some generic timers defined mostly in timers.h.
 */

#include <chrono>
#include <iostream>

#include "timers.hpp"
#include "stats.hpp"

using namespace std;


/* execute a 1-cycle loop 'iters' times */
#if UARCH_BENCH_PORTABLE
bench2_f portable_add_chain;
#define CAL_FN portable_add_chain
#else
extern "C" bench2_f add_calibration_x86;
#define CAL_FN add_calibration_x86
#endif

using namespace Stats;
using namespace std::chrono;

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
    static_assert((ITERS & 3) == 0, "iters must be divisible by 4 because we unroll some loops by 4");
    const char* mhz;
    if ((mhz = getenv("UARCH_BENCH_CLOCK_MHZ"))) {
        double ghz = std::stoi(mhz) / 1000.0;
        return ghz;
    }

    std::array<nanoseconds::rep, TRIES> results;

    for (size_t w = 0; w < WARMUP + 1; w++) {
        for (size_t r = 0; r < TRIES; r++) {
            auto t0 = CLOCK::nanos();
            CAL_FN(ITERS, nullptr);
            auto t1 = CLOCK::nanos();
            CAL_FN(ITERS * 2, nullptr);
            auto t2 = CLOCK::nanos();
            results[r] = (t2 - t1) - (t1 - t0);
        }
    }

    DescriptiveStats stats = get_stats(results.begin(), results.end());

    double ghz = ((double)ITERS / stats.getMedian());
    return ghz;
}

template <typename CLOCK>
double ClockTimerT<CLOCK>::getGHz() {
    static double ghz = CalcCpuFreq<10000,CLOCK,1000>();
    return ghz;
}

template <typename CLOCK>
void ClockTimerT<CLOCK>::init(Context &c) {
        c.out() << "Median CPU speed: " << std::fixed << std::setw(4) << std::setprecision(3)
        << getGHz() << " GHz" << std::endl;
}

// explicit instantiation for the default clock
template double DefaultClockTimer::getGHz();
template void DefaultClockTimer::init(Context& c);

// stuff for calculating clock overhead

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

template <typename CLOCK>
void printOneClock(std::ostream& out, const char* name) {
    out << setw(48) << name << setw(28) << CalcClockRes<100,CLOCK>().getString4(5,1);
    out << setw(30) << CalcClockCost<100,CLOCK>().getString4(5,1) << endl;
}

struct DumbClock {
    static int64_t nanos() { return 0; }
};

void printClockOverheads(std::ostream& out) {
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
#ifdef CLOCK_BOOTTIME
    PRINT_CLOCK(GettimeAdapter<CLOCK_BOOTTIME>);
#endif

    PRINT_CLOCK(DumbClock);

    out << endl;
}





