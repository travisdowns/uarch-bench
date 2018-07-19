/*
 * benchmark-group.cpp
 */
#include <cassert>

#include "benches.hpp"
#include "table.hpp"

using namespace std;

using namespace table;


void BenchmarkGroup::runIf(Context &c, const TimerInfo &ti, const predicate_t& predicate) {
    bool header = false;
    for (auto& b : benches_) {
        if (predicate(b)) {
            if (!header) {
                c.out() << std::endl << "** Running benchmark group " << getDescription() << " **" << std::endl;
                printGroupHeader(c);
                header = true;
            }
            b->runAndPrint(c);
        }
    }
}

void BenchmarkGroup::printBenches(std::ostream& out) const {
    Table t;
    t.colInfo(0).justify = ColInfo::LEFT;
    t.newRow().add("ID").add("Description");
    for (auto& bench : getBenches()) {
        t.newRow().add(bench->getPath()).add(bench->getDescription());
    }
    out << t.str();
}







