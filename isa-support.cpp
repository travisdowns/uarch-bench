/*
 * isa-support.cpp
 */

#include "isa-support.hpp"
#include "cpu/cpu.h"

#include <assert.h>

struct Entry {
    x86Feature feature;
    PSnipCPUFeature psnip_feature;
    const char *name;
    bool supported() const {
        return psnip_cpu_feature_check(psnip_feature);
    }
};

#define MAKE_ENTRY(x) Entry{x, PSNIP_CPU_FEATURE_X86_ ## x, #x},

const Entry FEATURES_ARRAY[] = {
    FEATURES_X(MAKE_ENTRY)
};

const size_t FEATURES_COUNT = sizeof(FEATURES_ARRAY)/sizeof(FEATURES_ARRAY[0]);

const Entry& lookup(x86Feature feature) {
    assert(feature >= 0);
    assert(feature < FEATURES_COUNT);
    return FEATURES_ARRAY[feature];
}

const char* to_name(x86Feature feature) {
    return lookup(feature).name;
}

std::string support_string() {
    std::string result;
    for (const Entry& e : FEATURES_ARRAY) {
        if (e.supported()) {
            result += result.empty() ? e.name : std::string(" ") + e.name;
        }
    }
    return result;
}

