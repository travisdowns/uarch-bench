/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benchmark.hpp"

extern "C" {
bench2_f dep_add_rax_rax;
bench2_f indep_add;
bench2_f dep_imul128_rax;
bench2_f dep_imul64_rax;
bench2_f indep_imul128_rax;
bench2_f indep_imul64_rax;
bench2_f store_same_loc;
bench2_f store64_disjoint;
bench2_f dep_pushpop;
bench2_f indep_pushpop;
bench2_f div_64_64;
bench2_f idiv_64_64;

bench2_f nop1_128;
bench2_f nop2_128;
bench2_f xor_eax_128;

bench2_f alu_load_6_0;
bench2_f alu_load_6_1;
bench2_f alu_load_6_2;
bench2_f alu_load_6_3;
bench2_f alu_load_6_4;
bench2_f alu_load_6_5;
bench2_f alu_load_6_6;

bench2_f sameloc_pointer_chase;
bench2_f sameloc_pointer_chase_complex;
bench2_f rdtsc_bench;
bench2_f rdtscp_bench;
}

template <typename TIMER>
void register_default(GroupList& list) {
#if !UARCH_BENCH_PORTABLE
    std::shared_ptr<BenchmarkGroup> default_group = std::make_shared<BenchmarkGroup>("basic", "Basic Benchmarks");
    list.push_back(default_group);

    auto maker = DeltaMaker<TIMER>(default_group.get()).setTags({"default"});

    maker.template make<dep_add_rax_rax>  ("dep-add", "Dependent add chain",       128);
    maker.template make<indep_add>        ("indep-add", "Independent add chain",  50 * 8);
    maker.template make<dep_imul128_rax>  ("dep-mul128", "Dependent imul 64->128",    128);
    maker.template make<dep_imul64_rax>   ("dep-mul64",  "Dependent imul 64->64",     128);
    maker.template make<indep_imul128_rax>("indep-mul128", "Independent imul 64->128",  128);
    maker.template make<indep_imul64_rax >("indep-mul64",  "Independent imul 64->64",  128);
    maker.template make<store_same_loc>   ("same-stores", "Same location stores",      128);
    maker.template make<store64_disjoint> ("disjoint-stores", "Disjoint location stores",  128);
    maker.template make<dep_pushpop>      ("dep-push-pop", "Dependent push/pop chain",  128);
    maker.template make<indep_pushpop>    ("indep-push-pop", "Independent push/pop chain",  128);
    maker.template make<div_64_64>        ("64-bit div", "64-bit dependent div 1/1 = 1",  128);
    maker.template make<idiv_64_64>       ("64-bit idiv","64-bit dependent idiv 1/1 = 1",  128);

    // tests to test the maximum CPU width for a variety of easy instruction mixes
    maker.template make<nop1_128> ("1-byte nops",  "128 consecutive 1-byte nops",  128);
    maker.template make<nop2_128> ("2-byte nops",  "128 consecutive 2-byte nops",  128);
    maker.template make<xor_eax_128>  ("xor zero", "128 consecutive xor eax, eax", 128);

    maker.template make<alu_load_6_0>  ("alu-load-6-0", "6 adds, 0 loads",  6 * 64);
    maker.template make<alu_load_6_1>  ("alu-load-6-1", "6 adds, 1 loads",  7 * 64);
    maker.template make<alu_load_6_2>  ("alu-load-6-2", "6 adds, 2 loads",  8 * 64);
    maker.template make<alu_load_6_3>  ("alu-load-6-3", "6 adds, 3 loads",  9 * 64);
    maker.template make<alu_load_6_4>  ("alu-load-6-4", "6 adds, 4 loads", 10 * 64);
    maker.template make<alu_load_6_5>  ("alu-load-6-5", "6 adds, 5 loads", 11 * 64);
    maker.template make<alu_load_6_6>  ("alu-load-6-6", "6 adds, 6 loads", 12 * 64);

    maker.template make<sameloc_pointer_chase>         ("pointer-chase-simple", "Simple addressing pointer chase",  128);
    maker.template make<sameloc_pointer_chase_complex> ("pointer-chase-complex","Complex addressing pointer chase",  128);
    maker.template make<rdtsc_bench> ("rdtsc","back-to-back rdtsc throughput",  128);
    maker.template make<rdtscp_bench> ("rdtscp","back-to-back rdtscp throughput",  128);



    // note: more pointer-chasing tests in mem-benches.cpp
#endif // #if !UARCH_BENCH_PORTABLE
}

#define REG_DEFAULT(CLOCK) template void register_default<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



