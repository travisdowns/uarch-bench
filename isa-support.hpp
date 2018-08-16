/*
 * isa-support.hpp
 *
 * Functionality to all specification of required ISA features and checking such features. Most of
 * the hard work is just delegated to portable-snippets/cpu
 */

#ifndef ISA_SUPPORT_HPP_
#define ISA_SUPPORT_HPP_

#include <vector>
#include <string>

#define FEATURES_X(f)   \
          f(SSE3      ) \
          f(PCLMULQDQ ) \
          f(VMX       ) \
          f(SMX       ) \
          f(EST       ) \
          f(TM2       ) \
          f(SSSE3     ) \
          f(FMA       ) \
          f(CX16      ) \
          f(SSE4_1    ) \
          f(SSE4_2    ) \
          f(MOVBE     ) \
          f(POPCNT    ) \
          f(AES       ) \
          f(AVX       ) \
          f(RDRND     ) \
          f(TSC_ADJ   ) \
          f(SGX       ) \
          f(BMI1      ) \
          f(HLE       ) \
          f(AVX2      ) \
          f(BMI2      ) \
          f(ERMS      ) \
          f(RTM       ) \
          f(MPX       ) \
          f(PQE       ) \
          f(AVX512F   ) \
          f(AVX512DQ  ) \
          f(RDSEED    ) \
          f(ADX       ) \
          f(AVX512IFMA) \
          f(PCOMMIT   ) \
          f(CLFLUSHOPT) \
          f(CLWB      ) \
          f(INTEL_PT  ) \
          f(AVX512PF  ) \
          f(AVX512ER  ) \
          f(AVX512CD  ) \
          f(SHA       ) \
          f(AVX512BW  ) \
          f(AVX512VL  )

#define COMMA(x) x,

/**
 * Features a benchmark may require from an x86 CPU.
 */
enum x86Feature {
    // the list is the same as the arguments in the FEATURES_X macro above
    FEATURES_X(COMMA)
};

/** does the current CPU support all of the given features */
bool supports(std::vector<x86Feature> features);

/** return a space-delimited string of all the features in x86Features the current CPU supports */
std::string support_string();


#endif /* ISA_SUPPORT_HPP_ */
