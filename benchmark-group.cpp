/*
 * benchmark-group.cpp
 */
#include <cassert>

#include "benches.hpp"

using namespace std;

constexpr int BENCH_ID_WIDTH = 16;


void BenchmarkGroup::runIf(Context &context, const TimerInfo &ti, const predicate_t& predicate) {
    bool header = false;
    for (auto& b : benches_) {
        if (predicate(b)) {
            if (!header) {
                context.out() << std::endl << "** Running benchmark group " << getDescription() << " **" << std::endl;
                printResultHeader(context, ti);
                header = true;
            }
            b.runAndPrint(context);
        }
    }
}

void BenchmarkGroup::printBenches(std::ostream& out) const {
    for (auto& bench : getBenches()) {
        printBench(out, bench);
    }
}

void BenchmarkGroup::printBench(std::ostream& out, const Benchmark& bench) {
    out << left << setw(BENCH_ID_WIDTH) << bench.getPath() << ": " << bench.getDescription() << endl;
}






