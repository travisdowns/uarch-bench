/*
 * benchmark.cpp
 */
#include <cassert>

#include "benches.hpp"

using namespace std;

constexpr int  NAME_WIDTH =  30;
constexpr int  COLUMN_PAD =  3;

template <typename T>
static void printAlignedMetrics(Context &c, const std::vector<T>& metrics) {
    const TimerInfo &ti = c.getTimerInfo();
    assert(ti.getMetricNames().size() == metrics.size());
    for (size_t i = 0; i < metrics.size(); i++) {
        // the width is either the expected max width of the value, or the with of the name, plus COLUMN_PAD
        unsigned int width = std::max(ti.getMetricNames().size(), 4UL + c.getPrecision()) + COLUMN_PAD;
        c.out() << std::setw(width) << metrics[i];
    }
}

static void printResultLine(Context& c, const std::string& benchName, const TimingResult& result) {
    std::ostream& os = c.out();
    os << setprecision(c.getPrecision()) << fixed << setw(NAME_WIDTH) << benchName;
    printAlignedMetrics(c, result.getResults());
    os << endl;
}

void printResultHeader(Context& c, const TimerInfo& ti) {
    // "Benchmark", "Cycles", "Nanos"
    c.out() << setw(NAME_WIDTH) << "Benchmark";
    printAlignedMetrics(c, ti.getMetricNames());
    c.out() << endl;
}

Benchmark::Benchmark(const BenchmarkGroup* parent, const std::string& id, const std::string& description,
        size_t ops_per_loop, full_bench_t full_bench, uint32_t loop_count) :
        parent{parent},
        id{id},
        description{description},
        ops_per_loop_{ops_per_loop},
        full_bench_{full_bench},
        loop_count_{loop_count}
        {}

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
    printResultLine(c, getDescription(), timing);
}

std::string Benchmark::getPath() {
    return parent->getId() + "/" + getId();
}




