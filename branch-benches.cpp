/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benchmark.hpp"

#define JF_D(f, a, b) f(jmp_forward_##a##_##b, a, b)
#define IN_D(f, a) f(indirect_forward_##a, a)

#define FORWARD_X(delegate, f) \
    delegate(f,  8,  8) \
    delegate(f, 16, 16) \
    delegate(f, 32, 32) \
    delegate(f, 64, 64) \

#define FORWARD_X2(delegate, f) \
    delegate(f,  8) \
    delegate(f, 16) \
    delegate(f, 32) \
    delegate(f, 64) \

#define JMP_FORWARD_X(f) FORWARD_X(JF_D, f)
#define IND_FORWARD_X(f) FORWARD_X2(IN_D, f)

#define DECLARE_FORWARD(name, ...) bench2_f name;

extern "C" {
JMP_FORWARD_X(DECLARE_FORWARD)
IND_FORWARD_X(DECLARE_FORWARD)
bench2_f define_indirect_variable;
}

template <typename TIMER>
void register_branch(GroupList& list) {
#if !UARCH_BENCH_PORTABLE
    std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("branch/x86/indirect",
            "Indirect branch benchmarks");
    list.push_back(group);

    auto default_maker = DeltaMaker<TIMER>(group.get());

    #define MAKE_JUMP_FORWARD(name, a, b) default_maker.template make<name> \
            (#name, "20 uncond jumps by " #a " then " #b " bytes", 1);

    JMP_FORWARD_X(MAKE_JUMP_FORWARD)

    #define MAKE_INDIRECT_FORWARD(name, a) default_maker.template make<name> \
            (#name, "20 indirect jumps by " #a " bytes", 1);

    IND_FORWARD_X(MAKE_INDIRECT_FORWARD)

    default_maker.template make<define_indirect_variable> \
        ("define_indirect_variable", "indirect variable", 1);

#endif // #if !UARCH_BENCH_PORTABLE
}

#define REGISTER(CLOCK) template void register_branch<CLOCK>(GroupList& list);

ALL_TIMERS_X(REGISTER);



