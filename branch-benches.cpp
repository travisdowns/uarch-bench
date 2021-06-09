/*
 * branch-benches.cpp
 *
 * Branching benchmarks.
 */

#include "benchmark.hpp"

#define JF_D(f, a, b) f(jmp_forward_##a##_##b, a, b)
#define IN_D(f, a) f(indirect_forward_##a, a)
#define JD_D(f, a, b) f(jmp_forward_dual_##a##_##b##_0, a, b, 0) f(jmp_forward_dual_##a##_##b##_1, a, b, 1)
#define JL_D(f, a, b, c, d) f(a, b, c, d)

#define FORWARD_X(delegate, f) \
    delegate(f,   8,   8) \
    delegate(f,  16,  16) \
    delegate(f,  32,  32) \
    delegate(f,  64,  64) \
    delegate(f,  96,  96) \
    delegate(f, 128, 128) \

#define FORWARD_X2(delegate, f) \
    delegate(f,  8) \
    delegate(f, 16) \
    delegate(f, 32) \
    delegate(f, 64) \

#define DUAL_X(delegate, f) \
    delegate(f, 32, 16) \
    delegate(f, 32,  6) \
    delegate(f, 64, 16) \
    delegate(f, 64, 48) \

#define LOOP_X(delegate, f) \
    delegate(f, 32, 16,   2,   0) \
    delegate(f, 32, 16,   2,   1) \
    delegate(f, 32, 16,   2,   2) \
    \
    delegate(f, 32, 16,   3,   0) \
    delegate(f, 32, 16,   3,   1) \
    delegate(f, 32, 16,   3,   2) \
    delegate(f, 32, 16,   3,   3)

#define JMP_FORWARD_X(f) FORWARD_X(JF_D, f)
#define IND_FORWARD_X(f) FORWARD_X2(IN_D, f)
#define JMP_DUAL_X(f)    DUAL_X(JD_D, f)
#define JMP_LOOP_X(f)    LOOP_X(JL_D, f)

#define DECLARE_FORWARD(name, ...) bench2_f name;

extern "C" {
JMP_FORWARD_X(DECLARE_FORWARD)
IND_FORWARD_X(DECLARE_FORWARD)
JMP_DUAL_X(DECLARE_FORWARD)

bench2_f jmp_loop_generic_32_16;

bench2_f define_indirect_variable;

bench2_f la_load;
bench2_f la_lea;
}

template <typename TIMER>
void register_branch(GroupList& list) {
#if !UARCH_BENCH_PORTABLE
    std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("branch/x86/indirect",
            "Indirect branch benchmarks");
    list.push_back(group);

    auto maker = DeltaMaker<TIMER>(group.get());

    #define MAKE_JUMP_FORWARD(name, a, b) maker.template make<name> \
            (#name, "20 uncond jumps by " #a " then " #b " bytes", 1);

    JMP_FORWARD_X(MAKE_JUMP_FORWARD)

    #define MAKE_INDIRECT_FORWARD(name, a) maker.template make<name> \
            (#name, "20 indirect jumps by " #a " bytes", 1);

    IND_FORWARD_X(MAKE_INDIRECT_FORWARD)

    // shows that the first target in a 32-byte block gets special (fast) treatment
    // in the BTB
    #define MAKE_JUMP_DUAL(name, a, b, sprio) maker.template make<name> \
            (#name, "30 dual jmps, blk: " #a ", gap: " #b ", sp: " #sprio, 1);

    JMP_DUAL_X(MAKE_JUMP_DUAL)

    struct jmp_loop_args {
        uint32_t total, first;
    };

    #define MAKE_JUMP_LOOP(a, b, total, first) maker.template make<jmp_loop_generic_##a##_##b> \
            ("jmp-loop-" #a "-" #b "-" #total "-" #first, "loop blk: " #a " gap: " #b " first: " #first " tot: " #total, 1,  \
            arg_object(jmp_loop_args{total, first}));

    JMP_LOOP_X(MAKE_JUMP_LOOP)

    uint32_t sizes[] = {10, 100};

    for (auto total : sizes) {
        for (uint32_t first = 0; first <= total; first++) {
            auto id = std::string("jmp-loop-32-16-{}-{}") + std::to_string(total) + "-" + std::to_string(first);
            auto desc = std::string("loop blk: 32 gap: 16 first: ") + std::to_string(first) + " tot: " + std::to_string(total);
            maker.template make<jmp_loop_generic_32_16>
                    (id, desc, 1, arg_object(jmp_loop_args{total, first}));
        }
    }

    maker.template make<define_indirect_variable> \
        ("define_indirect_variable", "indirect variable", 1);

    maker.template make<la_load>("load-alloc", "Load rename/alloc latency", 1);
    maker.template make<la_lea>("lea-alloc", "LEA rename/alloc latency", 1);

#endif // #if !UARCH_BENCH_PORTABLE
}

#define REGISTER(CLOCK) template void register_branch<CLOCK>(GroupList& list);

ALL_TIMERS_X(REGISTER);



