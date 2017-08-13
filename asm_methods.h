/*
 * Declarations for methods defined in asm.
 *
 * asm_methods.h
 */

#ifndef ASM_METHODS_H_
#define ASM_METHODS_H_

#include "bench-declarations.hpp"

extern "C" {

/* execute a 1-cycle loop 'iters' times */
bench_f add_calibration;
bench_f dep_add_rax_rax;
bench_f indep_add;
bench_f dep_imul128_rax;
bench_f dep_imul64_rax;
bench_f indep_imul128_rax;
bench_f store_same_loc;
bench_f store64_disjoint;

bench2_f  store16_any;
bench2_f  store32_any;
bench2_f  store64_any;
bench2_f store128_any; // AVX2 128-bit store
bench2_f store256_any; // AVX2 256-bit store

bench2_f  load16_any;
bench2_f  load32_any;
bench2_f  load64_any;
bench2_f load128_any; // AVX (REX-encoded) 128-bit store
bench2_f load256_any; // AVX (REX-encoded) 256-bit store


//void add_calibration(uint64_t iters);
//void dep_add_rax_rax(uint64_t iters);

}

#endif /* ASM_METHODS_H_ */
