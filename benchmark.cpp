/*
 * benchmark.cpp
 */
#include "benchmark.hpp"
#include "isa-support.hpp"
#include "opt-control.hpp"

#include <cassert>


using namespace std;

BenchArgs::BenchArgs(
            const BenchmarkGroup* parent,
            const std::string& id,
            const std::string& description,
            taglist_t tags,
            featurelist_t features,
            uint32_t ops_per_loop
            ) :
    parent{parent},
    id{id},
    description{description},
    tags{tags},
    features{features},
    ops_per_loop{ops_per_loop}
    {}

arg_provider_t constant(void *value) {
    return [=]{ return value; };
}

const arg_provider_t null_provider = constant(nullptr);

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

void BenchmarkBase::runAndPrint(Context& c) {
    if (!supports(args.features)) {
        // can't run this test on this hardware
        printBenchName(c, this);
        printOneMetric(c, std::string("Skipped because hardware doesn't support required features: ") +
                container_to_string(args.features));
        c.out() << endl;
    } else {
        runAndPrintInner(c);
    }
}

std::string BenchmarkBase::getPath() const {
    return getGroup().getId() + "/" + getId();
}

// a benchmark that immediately returns, useful as the "base" method
// to cancel out some forms of overhead
long dummy_bench(uint64_t iters, void* arg) {
    // we jump through these hoops to get a function with a single ret
    // instruction
    long l;
    opt_control::overwrite(l);
    return l;
}



