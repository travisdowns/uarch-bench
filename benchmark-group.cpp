/*
 * benchmark-group.cpp
 */
#include <cassert>

#include "benchmark.hpp"
#include "table.hpp"

using namespace std;

using namespace table;


void BenchmarkGroup::runIf(Context &c, const predicate_t& predicate) {
    bool header = false;
    for (auto& b : benches_) {
        if (predicate(b)) {
            if (!header) {
                c.out() << std::endl << "** Running group " << getId() << " : " << getDescription() << " **" << std::endl;
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
    t.newRow().add("ID").add("Description").add("Tags").add("ISA Features");
    for (auto& bench : getBenches()) {
        t.newRow()
                .add(bench->getPath())
                .add(bench->getDescription())
                .add(container_to_string(bench->getTags()))
                .add(container_to_string(bench->getFeatures()));
    }
    out << t.str();
}







