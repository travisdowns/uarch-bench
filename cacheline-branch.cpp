/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benchmark.hpp"
#include "util.hpp"
#include "boost/preprocessor/repetition/repeat_from_to.hpp"

#if !UARCH_BENCH_PORTABLE

// The store/add/dec/jnz encodes to 15 bytes, add 50 NOPs to split the
// branch target and the branch across a 64-byte cacheline.
#define NOPS \
		"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
		"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
		"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
		"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
		"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"

#define BUF_SIZE (64*1024)
static char buf[BUF_SIZE];

void cross_cacheline_branch_fast(void *addr, unsigned long size)
{
        long __d0;
        asm volatile(
		".align 64\n"
                "0:     movq $0,(%[dst])\n"
                "       addq   $8,%[dst]\n"
                "       decl %%ecx ; jnz   0b\n"
                : [size8] "=&c"(size), [dst] "=&D" (__d0)
                : [size1] "r"(size & 7), "[size8]" (size / 8), "[dst]"(addr),
                  [zero] "r" (0UL), [eight] "r" (8UL));
}

void cross_cacheline_branch_slow(void *addr, unsigned long size)
{
        long __d0;
        asm volatile(
		".align 64\n"
		NOPS
                "0:     movq $0,(%[dst])\n"
                "       addq   $8,%[dst]\n"
                "       decl %%ecx ; jnz   0b\n"
                : [size8] "=&c"(size), [dst] "=&D" (__d0)
                : [size1] "r"(size & 7), "[size8]" (size / 8), "[dst]"(addr),
                  [zero] "r" (0UL), [eight] "r" (8UL));
}

HEDLEY_NEVER_INLINE __attribute__((noclone))
static long cacheline_branch_fast(uint64_t iters, void *arg) {
	do {
		cross_cacheline_branch_fast(buf, BUF_SIZE);
    } while (--iters);
    return 0;
}

HEDLEY_NEVER_INLINE __attribute__((noclone))
static long cacheline_branch_slow(uint64_t iters, void *arg) {
	do {
		cross_cacheline_branch_slow(buf, BUF_SIZE);
    } while (--iters);
    return 0;
}

template <typename TIMER>
void register_cacheline_branch(GroupList& list) {
    std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("misc/cacheline_branch", "Cacheline crossing branch");

    auto maker = DeltaMaker<TIMER>(group.get(), 100 * 1000).setTags({"slow"});

    maker.template make<cacheline_branch_slow>("cacheline_branch_slow",  "cacheline_branch_slow", 1);
    maker.template make<cacheline_branch_fast>("cacheline_branch_fast",  "cacheline_branch_fast", 1);
    list.push_back(group);
}

#else // #if !UARCH_BENCH_PORTABLE

template <typename TIMER>
void register_cacheline_branch(GroupList& list) {}

#endif

#define REG_CL_BRANCH(CLOCK) template void register_cacheline_branch<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_CL_BRANCH)
