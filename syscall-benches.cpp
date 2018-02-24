/*
 * Test syscall/sysenter etc overhead.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "benches.hpp"
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

#define BUFSIZE (1u << 25)

template <typename TIMER>
void register_syscall(GroupList& list) {
    {
        std::shared_ptr<BenchmarkGroup> syscall_group = std::make_shared<BenchmarkGroup>("syscall", "Syscall benches");

        using maker = BenchmarkMaker<TIMER>;

        auto benches = std::vector<Benchmark> {
            maker::template make_bench<getuid_glibc>(  syscall_group.get(),  "getuid-glibc",    "getuid() glibc call", 1),
            maker::template make_bench<getuid_syscall>(syscall_group.get(),  "getuid-syscall",  "getuid using syscall()", 1),
            maker::template make_bench<getpid_syscall>(syscall_group.get(),  "getpid-syscall",  "getpid using syscall()", 1),
            maker::template make_bench<close999>(syscall_group.get(),  "close-999",  "close() on a non-existent FD", 1),
            maker::template make_bench<notexist_syscall>(syscall_group.get(),"notexist-syscall", "non-existent syscall", 1),
            maker::template make_bench<syscall_asm>(syscall_group.get(),"getuid-asm", "getuid direct syscall", 1, constant<SYS_getuid>),
            maker::template make_bench<syscall_asm>(syscall_group.get(),"notexist-asm", "non-existent direct syscall", 1, constant<123456>),
            maker::template make_bench<syscall_asm_lfence_before>(syscall_group.get(), "lfence-before", "syscall+lfence before", 1, constant<123456>),
            maker::template make_bench<syscall_asm_lfence_after>(syscall_group.get(),  "lfence-after",  "syscall+lfence after", 1, constant<123456>),
            maker::template make_bench<lfence_only>(syscall_group.get(),"lfence-only", "back-to-back lfence", 8, constant<0>),
            maker::template make_bench<parallel_misses>(syscall_group.get(),  "parallel-misses", "parallel-misses", 1, [](){ return aligned_ptr(4096, BUFSIZE); }, 524288),
            maker::template make_bench<mfenced_misses>(syscall_group.get(),  "mfenced-misses", "mfenced misses", 1, [](){ return aligned_ptr(4096, BUFSIZE); }, 524288),
            maker::template make_bench<sfenced_misses>(syscall_group.get(),  "sfenced-misses", "sfenced misses", 1, [](){ return aligned_ptr(4096, BUFSIZE); }, 524288),
            maker::template make_bench<lfenced_misses>(syscall_group.get(),  "lfenced-misses", "lfenced misses", 1, [](){ return aligned_ptr(4096, BUFSIZE); }, 524288),
            maker::template make_bench<syscall_misses>(syscall_group.get(),  "syscall-misses", "syscall misses", 1, [](){ return aligned_ptr(4096, BUFSIZE); }, 524288),
            maker::template make_bench<syscall_misses_lfence>(syscall_group.get(),  "syscall-misses-lfence", "misses + lfence + syscall", 1, [](){ return aligned_ptr(4096, BUFSIZE); }, 524288),
        };

        syscall_group->add(benches);
        list.push_back(syscall_group);
    }


}

#define REGISTER(CLOCK) template void register_syscall<CLOCK>(GroupList& list);

ALL_TIMERS_X(REGISTER)



