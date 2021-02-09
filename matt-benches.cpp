/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benchmark.hpp"
#include "util.hpp"

#if !UARCH_BENCH_PORTABLE

#define BUF_SIZE (64*1024)
static char buf[BUF_SIZE];

void __clear_mov_imm(void *addr)
{
        long __d0;
        asm volatile("movq $0,(%[dst])\n"
		: [dst] "=&D" (__d0)
		: "[dst]"(addr));
}

void __clear_mov_reg(void *addr)
{
        long __d0;
        asm volatile("movq %[zero],(%[dst])\n"
		: [dst] "=&D" (__d0)
		: "[dst]"(addr), [zero] "r" (0UL));
}

HEDLEY_NEVER_INLINE
long BM_mov_reg(uint64_t iters, void *arg) {
	do {
		__clear_mov_reg(buf);
    } while (--iters);
    return 0;
}

HEDLEY_NEVER_INLINE
long BM_mov_imm(uint64_t iters, void *arg) {
	do {
		__clear_mov_imm(buf);
    } while (--iters);
    return 0;
}

HEDLEY_NEVER_INLINE
long BM_mov_reg_inline(uint64_t iters, void *arg) {
	do {
        long __d0;
        asm volatile("movq %[zero],(%[dst])\n"
        : [dst] "=&D" (__d0)
        : "[dst]"(buf), [zero] "r" (0UL));
    } while (--iters);
    return 0;
}

HEDLEY_NEVER_INLINE
long BM_mov_imm_inline(uint64_t iters, void *arg) {
	do {
	        long __d0;
	        asm volatile("movq $0,(%[dst])\n"
			: [dst] "=&D" (__d0)
			: "[dst]"(buf));
	} while (--iters);
    return 0;
}

template <typename TIMER>
void register_matt(GroupList& list) {
    std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("misc/matt", "From Twitter");

    auto maker = DeltaMaker<TIMER>(group.get(), 100 * 1000);

    maker.template make<BM_mov_reg>("BM_mov_reg",  "BM_mov_reg", 1);
    maker.template make<BM_mov_imm>("BM_mov_imm",  "BM_mov_imm", 1);
    maker.template make<BM_mov_reg_inline>("BM_mov_reg_inline",  "BM_mov_reg_inline", 1);
    maker.template make<BM_mov_imm_inline>("BM_mov_imm_inline",  "BM_mov_imm_inline", 1);

    list.push_back(group);
}

#else // #if !UARCH_BENCH_PORTABLE

template <typename TIMER>
void register_matt(GroupList& list) {}

#endif

#define REG_MATT(CLOCK) template void register_matt<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_MATT)




