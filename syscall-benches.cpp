/*
 * Test syscall/sysenter etc overhead.
 */

#include <sched.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "benchmark.hpp"
#include "util.hpp"

extern "C" {
bench2_f syscall_asm;
bench2_f syscall_asm_lfence_before;
bench2_f syscall_asm_lfence_after;
bench2_f lfence_only;
bench2_f parallel_misses;
bench2_f lfenced_misses;
bench2_f mfenced_misses;
bench2_f sfenced_misses;
bench2_f syscall_misses;
bench2_f syscall_misses_lfence;
}

template <int X>
void *constant() {
    return reinterpret_cast<void *>(X);
}

long getuid_glibc(uint64_t iters, void *arg) {
    while (iters-- > 0) {
        getuid();
    }
    return 0;
}

long getuid_syscall(uint64_t iters, void *arg) {
    while (iters-- > 0) {
        syscall(SYS_getuid);
    }
    return 0;
}

long getpid_syscall(uint64_t iters, void *arg) {
    while (iters-- > 0) {
        syscall(SYS_getpid);
    }
    return 0;
}

long close999(uint64_t iters, void *arg) {
    while (iters-- > 0) {
        close(999);
    }
    return 0;
}

long notexist_syscall(uint64_t iters, void *arg) {
    while (iters-- > 0) {
        syscall(123456);
    }
    return 0;
}

// courtesy of https://stackoverflow.com/questions/35944030
// lets this compiled with a dummy getcpu if it doesn't exist
template<typename T>
int getcpu(T, const void*) { return 0; }

long getcpu_syscall(uint64_t iters, void *arg) {
    unsigned cpu, total = 0;
    while (iters-- > 0) {
        // may be implemented in vDSO
        total += getcpu(&cpu, nullptr);
    }
    return total;
}

long call_sched_getcpu(uint64_t iters, void *arg) {
    unsigned total = 0;
    while (iters-- > 0) {
        total += sched_getcpu();
    }
    return total;
}

#define BUFSIZE (1u << 25)

template <typename TIMER>
void register_syscall(GroupList& list) {
    {
        std::shared_ptr<BenchmarkGroup> syscall_group = std::make_shared<BenchmarkGroup>("syscall", "Syscall benches");
        list.push_back(syscall_group);

        auto maker = DeltaMaker<TIMER>(syscall_group.get()).setTags({"os"});

        maker.template make<getuid_glibc>             ("getuid-glibc",     "getuid() glibc call",           1);
        maker.template make<getuid_syscall>           ("getuid-syscall",   "getuid using syscall()",        1);
        maker.template make<getpid_syscall>           ("getpid-syscall",   "getpid using syscall()",        1);
        maker.template make<close999>                 ("close-999",        "close() on a non-existent FD", 1);
        maker.template make<getcpu_syscall>           ("getcpu-syscall",   "getcpu syscall (maybe VDSO)",   1);
        maker.template make<notexist_syscall>         ("notexist-syscall", "non-existent syscall",          1);
        maker.template make<call_sched_getcpu>        ("sched_getcpu",     "sched_getcpu",                  1);

#if !UARCH_BENCH_PORTABLE
        maker.template make<syscall_asm>              ("getuid-asm",       "getuid direct syscall",         1, constant<SYS_getuid>);
        maker.template make<syscall_asm>              ("notexist-asm",     "non-existent direct syscall",   1, constant<123456>);
        maker.template make<syscall_asm_lfence_before>("lfence-before",    "syscall+lfence before",         1, constant<123456>);
        maker.template make<syscall_asm_lfence_after> ( "lfence-after",    "syscall+lfence after",          1, constant<123456>);
        maker.template make<lfence_only>              ("lfence-only",      "back-to-back lfence",           8, constant<0>);

        auto aligned_buf = [](){ return aligned_ptr(4096, BUFSIZE); };
        maker = maker.setLoopCount(524288).setTags({"slow"});
        maker.template make<parallel_misses>      ("parallel-misses",       "parallel-misses",           1, aligned_buf);
        maker.template make<mfenced_misses>       ("mfenced-misses",        "mfenced misses",            1, aligned_buf);
        maker.template make<sfenced_misses>       ("sfenced-misses",        "sfenced misses",            1, aligned_buf);
        maker.template make<lfenced_misses>       ("lfenced-misses",        "lfenced misses",            1, aligned_buf);
        maker.template make<syscall_misses>       ("syscall-misses",        "syscall misses",            1, aligned_buf);
        maker.template make<syscall_misses_lfence>("syscall-misses-lfence", "misses + lfence + syscall", 1, aligned_buf);
#endif // #if !UARCH_BENCH_PORTABLE

    }


}

#define REGISTER(CLOCK) template void register_syscall<CLOCK>(GroupList& list);

ALL_TIMERS_X(REGISTER)



