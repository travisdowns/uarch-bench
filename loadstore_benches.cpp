/*
 * loadstore_benches.cpp
 */

#include <iostream>
#include <cassert>
#include <sstream>
#include <iomanip>

#include "hedley.h"
#include "util.hpp"
#include "asm_methods.h"
#include "benches.hpp"
#include "context.hpp"
#include "timers.hpp"

using namespace std;

/*
 * A specialization of BenchmarkGroup that outputs its results in a 4 x 16 grid for all 64 possible
 * offsets within a 64B cache line.
 */
class LoadStoreGroup : public BenchmarkGroup {
    static constexpr unsigned DEFAULT_ROWS =  4;
    static constexpr unsigned DEFAULT_COLS = 16;

    unsigned rows_, cols_, total_cells_, op_size_;
public:
    LoadStoreGroup(const string& name, unsigned op_size, unsigned rows, unsigned cols)
: BenchmarkGroup(name), rows_(rows), cols_(cols), total_cells_(rows * cols), op_size_(op_size) {
        assert(rows < 10000 && cols < 10000);
    }

    HEDLEY_NEVER_INLINE std::string make_name(ssize_t misalign);

    HEDLEY_NEVER_INLINE static shared_ptr<LoadStoreGroup> make_group(const string& name, ssize_t op_size);

    template<typename TIMER, bench2_f METHOD>
    static shared_ptr<LoadStoreGroup> make(const string& name, unsigned op_size) {
        shared_ptr<LoadStoreGroup> group = make_group(name, op_size);
        using maker = BenchmarkMaker<TIMER>;
        for (ssize_t misalign = 0; misalign < 64; misalign++) {
            group->add(maker::template make_bench<METHOD>(group->make_name(misalign), 128,
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

shared_ptr<LoadStoreGroup> LoadStoreGroup::make_group(const string& name, ssize_t op_size) {
    return make_shared<LoadStoreGroup>(name, op_size, DEFAULT_ROWS, DEFAULT_COLS);
}

std::string LoadStoreGroup::make_name(ssize_t misalign) {
    std::stringstream ss;
    ss << "Misaligned " << (op_size_ * 8) << "-bit " << getName() << " [" << setw(2) << misalign << "]";
    return ss.str();
}

template <typename TIMER>
void register_loadstore(BenchmarkList& list) {
    // load throughput benches
    list.push_back(LoadStoreGroup::make<TIMER,  load16_any>("load/16-bit",  2));
    list.push_back(LoadStoreGroup::make<TIMER,  load32_any>("load/32-bit",  4));
    list.push_back(LoadStoreGroup::make<TIMER,  load64_any>("load/64-bit",  8));
    list.push_back(LoadStoreGroup::make<TIMER, load128_any>("load/128-bit", 16));
    list.push_back(LoadStoreGroup::make<TIMER, load256_any>("load/256-bit", 32));

    // store throughput
    list.push_back(LoadStoreGroup::make<TIMER,  store16_any>( "store/16-bit",  2));
    list.push_back(LoadStoreGroup::make<TIMER,  store32_any>( "store/32-bit",  4));
    list.push_back(LoadStoreGroup::make<TIMER,  store64_any>( "store/64-bit",  8));
    list.push_back(LoadStoreGroup::make<TIMER, store128_any>("store/128-bit", 16));
    list.push_back(LoadStoreGroup::make<TIMER, store256_any>("store/256-bit", 32));
}

#define REG_LOADSTORE(CLOCK) template void register_loadstore< CLOCK >(BenchmarkList& list);

ALL_TIMERS_X(REG_LOADSTORE)



