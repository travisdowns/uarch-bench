/*
 * libpfc-timer.cpp
 */

#include <sched.h>
#include <x86intrin.h> // for rdtsc

#include "libpfc/include/libpfc.h"
#include "libpfc-timer.hpp"
#include "util.hpp"
#include "asm_methods.h"  // for add_calibration

using namespace std;

#define CALIBRATION_NANOS (1000L * 1000L * 1000L)
#define CALIBRATION_ADDS (CALIBRATION_NANOS * 3) // estimate 3GHz

bool LibpfcTimer::is_init = false;

static void inner_init(Context &context) {
    auto err = pfcInit();
    if (err) {
        const char* msg = pfcErrorString(err);
        throw std::runtime_error(std::string("pfcInit() failed (error ") + std::to_string(err) + ": " + msg + ")");
    }

    err = pfcPinThread(0);
    if (err) {
        // let's treat this as non-fatal, it could occur if, for example
        context.log() << "WARNING: Pinning to CPU 0 failed, continuing without pinning" << endl;
    } else {
        context.log() << "Pinned to CPU 0" << endl;
    }

    // just use the fixed counters for now
    PFC_CFG  cfg[7] = {2,2,2};

    err = pfcWrCfgs(0, sizeof(cfg)/sizeof(cfg[0]), cfg);
    if (err) {
        const char* msg = pfcErrorString(err);
        throw std::runtime_error(std::string("pfcWrCfgs() failed (error ") + std::to_string(err) + ": " + msg + ")");
    }

    context.log() << "lipfc init OK" << std::endl;

//    /* calculate CPU frequency using reference cycles */
//    for (int i = 0; i < 100; i++) {
//        PFC_CNT cnt[7] = {};
//
//        int64_t start = nanos(), end = start + CALIBRATION_NANOS, now;
//        PFCSTART(cnt);
//        int64_t tsc =__rdtsc();
//        add_calibration(CALIBRATION_ADDS);
//        PFCEND(cnt);
//        int64_t tsc_delta = __rdtsc() - tsc;
//        now = nanos();
//
//        int64_t delta = now - start;
//        printf("CPU %d, measured CLK_REF_TSC MHz: %6.2f\n", sched_getcpu(), 1000.0 * cnt[PFC_FIXEDCNT_CPU_CLK_REF_TSC] / delta);
//        printf("CPU %d, measured rdtsc MHz      : %6.2f\n", sched_getcpu(), 1000.0 * tsc_delta / delta);
//        printf("CPU %d, measured add   MHz      : %6.2f\n", sched_getcpu(), 1000.0 * CALIBRATION_ADDS / delta);
//    }
}

void LibpfcTimer::init(Context &context) {
    if (!is_init) {
        inner_init(context);
        is_init = true;
    }
}






