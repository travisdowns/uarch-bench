/*
 * benchmark.cpp
 */
#include <cassert>

#include "benches.hpp"

using namespace std;

void *null() {
    return nullptr;
}

const arg_provider_t null_provider = null;

void printBenchName(Context& c, const std::string& name) {
    c.out() << setprecision(c.getPrecision()) << fixed << setw(DESC_WIDTH) << name;
}

void printBenchName(Context& c, const Benchmark& b) {
    printBenchName(c, b->getDescription());
}


void printResultLine(Context& c, const Benchmark& b, const TimingResult& result) {
    std::ostream& os = c.out();
    printBenchName(c, b);
    printAlignedMetrics(c, result.getResults());
    os << endl;
}


void printNameHeader(Context& c) {
    c.out() << setw(DESC_WIDTH) << "Benchmark";
}

void printResultHeader(Context& c) {
    // "Benchmark", "Cycles", "Nanos"
    printNameHeader(c);
    printAlignedMetrics(c, c.getTimerInfo().getMetricNames());
    c.out() << endl;
}

BenchmarkBase::BenchmarkBase(BenchArgs args) : args{std::move(args)} {}


std::string BenchmarkBase::getPath() const {
    return getGroup().getId() + "/" + getId();
}



