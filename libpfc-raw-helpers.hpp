/*
 * libpfc-raw-helpers.hpp
 *
 * Stuff dealing with "raw" libpfc benchmarks, which are those that embed the rdpmc call directly
 * in the benchmark itself, rather than using the generic mechanism exposed by LibPfcTimer.
 */

#ifndef LIBPFC_RAW_HELPERS_HPP_
#define LIBPFC_RAW_HELPERS_HPP_

#include "libpfc-timer.hpp"

#if USE_LIBPFC

#include <functional>

extern "C" {
/**
 * libpfc raw functions implement this function and result the results in result (passed in rdx)
 * [rdx +  0] <-- INST_RETIRED.ANY
 * [rdx +  8] <-- CPU_CLK_UNHALTED.THREAD (aka clock cycles)
 * [rdx + 16] <-- CPU_CLK_UNHALTED.REF_TSC
 * [rdx + 24] <-- programmable counter 1
 * ...
 */
typedef void (libpfc_raw1)(size_t loop_count, void *arg, LibpfcNow* results);

libpfc_raw1 raw_rdpmc0_overhead;
libpfc_raw1 raw_rdpmc4_overhead;
}

template <int samples, libpfc_raw1 METHOD, bench2_f WARM_EVERY = inlined_empty>
std::array<LibpfcNow, samples> libpfc_raw_adapt(size_t loop_count, void *arg) {
    std::array<LibpfcNow, samples> result = {};
    for (int i = 0; i < samples; i++) {
        WARM_EVERY(loop_count, arg);
        METHOD(loop_count, arg, &result[i]);
    }
    return result;
}


/**
 * Create an overhead calculation function which is suitable for use by OneShotMaker.withOverhead()
 */
template <libpfc_raw1 OMETHOD>
std::function<LibpfcNow(Context&)> libpfc_raw_overhead_adapt(const std::string& name, size_t loop_count = 0, void* arg = nullptr) {
    // calculate the overhead using raw_rdpmc_overhead, binding loops and arg to 0 and nullptr
    auto o1 = std::bind(libpfc_raw_adapt<ONESHOT_OVERHEAD_TOTAL, OMETHOD>, 0, nullptr);
    return [o1, name](Context& c){ return rawToOverhead<LibpfcTimer>(c, o1, name); };
}

#endif // USE_LIBPFC

#endif /* LIBPFC_RAW_HELPERS_HPP_ */
