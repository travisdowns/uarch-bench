#ifndef UARCH_BENCH_OPT_CONTROL_H_
#define UARCH_BENCH_OPT_CONTROL_H_

#include <type_traits>

#include "hedley.h"

namespace opt_control {


/*
 * "sinking" a value instructs the compiler to calculate it, i.e.,
 * makes the compiler believe that the value is necessary and hence
 * must be calculated.
 *
 * Note that this function does not imply that the value is *modified*
 * which means that a compiler may assume the value is unchanged across
 * it. This means that code on either side of the sink that deals with
 * the sunk value can still be optimized, as long as the correct value
 * is somehow "produced" for each call to sink. If you don't want that,
 * you might be looking for modify() instead.
 *
 * The actual sink implementation is empty and
 * so usually leaves no trace in the generated code except that the
 * value will be calculated.
 *
 * Currently, only primitive ("arithmetic") values are accepted, for
 * classes and pointers we need to think more about the meaning of
 * sinking a value.
 */
template <typename T>
HEDLEY_ALWAYS_INLINE
static void sink(T x) {
    static_assert(std::is_arithmetic<T>::value, "types should be primitive");
    __asm__ volatile ("" :: "r"(x) :);
}

/*
 * "modifying" a value instructs the compiler to produce the value, and
 * also that the value is changed after the call, i.e., that the value cannot
 * be cached, precaculated or folded around this call.
 *
 * The value is NOT actually modified: no instructions are emitted by the asm,
 * it only prevents the compiler from assuming the value is unmodified.
 *
 * This asm block is declared volatile, so the effect above always occurs, even
 * if the value is determined to be dead after the call. In particular, the input
 * value will be calculated even if the output is never used. Contrast with
 * modify_nv, which has the same semantics but is non-volatile.
 */
template <typename T>
HEDLEY_ALWAYS_INLINE
static void modify(T& x) {
    static_assert(std::is_arithmetic<T>::value, "types should be primitive");
    __asm__ volatile ("" :"+r"(x)::);
}

/*
 * "overwriting" a value instructs the compiler that the passed value has been
 * overwritten and so the value is unknown after this call (and so is not subject
 * to various optimizations).
 *
 * The value is NOT actually modified: no instructions are emitted by the asm,
 * it only prevents the compiler from assuming the value is unmodified.
 *
 * This function does not use the value as an input, so the compiler may not
 * calculate the up-to-date value of the variable at the point of the call, since
 * that value is dead (about to be overwritten).
 */
template <typename T>
HEDLEY_ALWAYS_INLINE
static void overwrite(T& x) {
    static_assert(std::is_arithmetic<T>::value, "types should be primitive");
    __asm__ volatile ("" :"=r"(x)::);
}

/*
 * Similar to sink except that it sinks the content pointed to 
 * by the pointer, so the compiler will materialize in memory
 * anything pointed to by the pointer.
 */
static inline void sink_ptr(void *p) {
    __asm__ volatile ("" :: "r"(p) : "memory");
}

}

#endif
