/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benchmark.hpp"
#include "util.hpp"

extern "C" {
/* misc benches */
bench2_f misc_add_loop32;
bench2_f misc_add_loop64;
bench2_f misc_port7;
bench2_f misc_fusion_add;
bench2_f misc_flag_merge_1;
bench2_f misc_flag_merge_2;
bench2_f misc_flag_merge_3;
bench2_f misc_flag_merge_4;
bench2_f david_schor1;
bench2_f double_macro_fusion;
bench2_f dsb_alignment_cross64;
bench2_f dsb_alignment_nocross64;
bench2_f bmi_tzcnt;
bench2_f bmi_lzcnt;
bench2_f bmi_popcnt;

bench2_f dendibakh_fused;
bench2_f dendibakh_fused_simple;
bench2_f dendibakh_fused_add;
bench2_f dendibakh_fused_add_simple;
bench2_f dendibakh_unfused;
bench2_f fusion_better_fused;
bench2_f fusion_better_unfused;
bench2_f misc_macro_fusion_addjo;

bench2_f adc_0_lat;
bench2_f adc_1_lat;
bench2_f adc_rcx_lat;
bench2_f adc_0_tput;
bench2_f adc_1_tput;
bench2_f adc_rcx_tput;

bench2_f retpoline_dense_call_lfence;
bench2_f retpoline_dense_call_pause;
bench2_f retpoline_sparse_call_base;
bench2_f retpoline_sparse_indep_call_lfence;
bench2_f retpoline_sparse_indep_call_pause;
bench2_f retpoline_sparse_dep_call_lfence;
bench2_f retpoline_sparse_dep_call_pause;
bench2_f indirect_dense_call_pred;
bench2_f indirect_dense_call_unpred;
bench2_f loop_weirdness_fast;

bench2_f tight_loop1;
bench2_f tight_loop2;
bench2_f tight_loop3;

bench2_f dep_add_noloop_128;

bench2_f vz_samereg;
bench2_f vz_diffreg;
bench2_f vz_diffreg16;
bench2_f vz_diffreg16xor;
bench2_f vz256_samereg;
bench2_f vz256_diffreg;
bench2_f vz128_samereg;
bench2_f vz128_diffreg;
bench2_f vzsse_samereg;
bench2_f vzsse_diffreg;

bench2_f adc_chain32;
bench2_f adc_chain64;

bench2_f decode33334;
bench2_f decode33333;
bench2_f decode16x1;
bench2_f decode8x2;
bench2_f decode4x4;
bench2_f decode664;
bench2_f decode88;
bench2_f decode871;
bench2_f decode8833334;
bench2_f decode884444;
bench2_f decode_monoid;

bench2_f weird_store_mov;
bench2_f weird_store_xor;
}


template <typename TIMER>
void register_misc(GroupList& list) {
    std::shared_ptr<BenchmarkGroup> misc_group = std::make_shared<BenchmarkGroup>("misc", "Miscellaneous tests");

    using default_maker = StaticMaker<TIMER>;

    const uint32_t iters = 10*1000;
    const size_t decode_ops = 50400/2;

    auto maker = DeltaMaker<TIMER>(misc_group.get(), iters);
    auto makerbmi1 = maker.setFeatures({BMI1}).setLoopCount(3 * 1000 * 1000);

    makerbmi1.template make<misc_add_loop32>("add-32", "32-bit add-loop", 1);
    makerbmi1.template make<misc_add_loop64>("add-64", "64-bit add-loop", 1);

    auto benches = std::vector<Benchmark> {
        default_maker::template make_bench<misc_port7>(misc_group.get(), "port7", "Can port7 be used by loads", 1,
                null_provider, iters),
        default_maker::template make_bench<misc_fusion_add>(misc_group.get(), "fusion-add", "Test micro-fused add", 128,
                null_provider, iters),
        default_maker::template make_bench<misc_macro_fusion_addjo>(misc_group.get(), "add-jo-fusion", "Add-JO fusion", 128,
                null_provider, iters),
        default_maker::template make_bench<adc_0_lat >(misc_group.get(), "adc-0-lat", "adc reg, 0 latency", 128,
                null_provider, iters),
        default_maker::template make_bench<adc_1_lat >(misc_group.get(), "adc-1-lat", "adc reg, 1 latency", 128,
                null_provider, iters),
        default_maker::template make_bench<adc_rcx_lat >(misc_group.get(), "adc-reg-lat", "adc reg,zero-reg latency", 128,
                null_provider, iters),
        default_maker::template make_bench<adc_0_tput>(misc_group.get(), "adc-0-tput", "adc reg, 0 throughput", 128,
                null_provider, iters),
        default_maker::template make_bench<adc_1_tput>(misc_group.get(), "adc-1-tput", "adc reg, 1 throughput", 128,
                null_provider, iters),
        default_maker::template make_bench<adc_rcx_tput>(misc_group.get(), "adc-rcx-tput", "adc reg,zero-reg throughput", 128,
                null_provider, iters),
        default_maker::template make_bench<misc_flag_merge_1>(misc_group.get(), "flag-merge-1", "Flag merge 1", 128,
                null_provider, iters),
        default_maker::template make_bench<misc_flag_merge_2>(misc_group.get(), "flag-merge-2", "Flag merge 2", 128,
                null_provider, iters),
        default_maker::template make_bench<misc_flag_merge_3>(misc_group.get(), "flag-merge-3", "Flag merge 3", 128,
                null_provider, iters),
        default_maker::template make_bench<misc_flag_merge_4>(misc_group.get(), "flag-merge-4", "Flag merge 4", 128,
                null_provider, iters),
        default_maker::template make_bench<david_schor1>(misc_group.get(), "schor1", "Suggested by David Schor", 1,
                    null_provider, iters),
        default_maker::template make_bench<double_macro_fusion>(misc_group.get(), "double-macro-fuse", "Double not-taken macro fusion", 128,
                null_provider, iters),
        default_maker::template make_bench<tight_loop1>(misc_group.get(), "tight-loop1", "Tight dec loop", 1,
                null_provider, iters * 10),
        default_maker::template make_bench<tight_loop2>(misc_group.get(), "tight-loop2", "Tight dec loop taken jmp", 1,
                null_provider, iters * 10),
        default_maker::template make_bench<tight_loop3>(misc_group.get(), "tight-loop3", "Tight dec loop untaken jmp", 1,
                null_provider, iters * 10),

        // https://news.ycombinator.com/item?id=15935283
        default_maker::template make_bench<loop_weirdness_fast>(misc_group.get(), "loop-weirdness-fast", "Loop weirdness fast", 1,
                []{ return aligned_ptr(1024, 1024); }, 10000),

        // private email
        default_maker::template make_bench<adc_chain32>(misc_group.get(), "adc-chain32", "adc add chain 32-bit", 1000,
                []{ return nullptr; }, 10000),
        default_maker::template make_bench<adc_chain64>(misc_group.get(), "adc-chain64", "adc add chain 64-bit", 1000,
                []{ return nullptr; }, 10000),

        // legacy (MITE) decode tests
        default_maker::template make_bench<decode33334>(misc_group.get(),   "decode33334", "Decode 3-3-3-3-4 byte nops", decode_ops, null_provider, 1000),
        default_maker::template make_bench<decode33333>(misc_group.get(),   "decode33333", "Decode 3-3-3-3-3 byte nops", decode_ops, null_provider, 1000),
        default_maker::template make_bench<decode16x1>(misc_group.get(),    "decode16x1",  "Decode 16x1 byte nops",      decode_ops, null_provider, 1000),
        default_maker::template make_bench<decode8x2>(misc_group.get(),     "decode8x2",   "Decode 8x2 byte nops",       decode_ops, null_provider, 1000),
        default_maker::template make_bench<decode4x4>(misc_group.get(),     "decode4x4",   "Decode 4x4 byte nops",       decode_ops, null_provider, 1000),
        default_maker::template make_bench<decode664>(misc_group.get(),     "decode664",   "Decode 6-6-4 byte nops",     decode_ops, null_provider, 1000),
        default_maker::template make_bench<decode88>(misc_group.get(),      "decode88",    "Decode 8-8 byte nops",       decode_ops, null_provider, 1000),
        default_maker::template make_bench<decode8833334>(misc_group.get(), "decode8833334", "Decode 8-8-3-3-3-3-4 byte nops",     decode_ops, null_provider, 1000),
        default_maker::template make_bench<decode884444>(misc_group.get(),  "decode884444",  "Decode 8-8-4-4-4-4 byte nops",     decode_ops, null_provider, 1000),
        default_maker::template make_bench<decode_monoid>(misc_group.get(),  "decode-monoid",  "Decode 33334x10, 556x10 blocks", 3200, null_provider, 1000),

        // case where when using the LSD, a loop with 2 stores apparently takes an extra cycle
        // Reported by Alexander Monakov in https://github.com/travisdowns/bimodal-performance/issues/4
        default_maker::template make_bench<weird_store_mov>(misc_group.get(), "weird-store-mov", "Store LSD weirdness, mov 0", 1000,
                []{ return nullptr; }, 10000),
        default_maker::template make_bench<weird_store_xor>(misc_group.get(), "weird-store-xor", "Store LSD weirdness, xor zero", 1000,
                []{ return nullptr; }, 10000),
    };

    misc_group->add(benches);
    list.push_back(misc_group);

    // Tests from https://dendibakh.github.io/blog/2018/02/04/Micro-ops-fusion
    std::shared_ptr<BenchmarkGroup> dendibakh = std::make_shared<BenchmarkGroup>("dendibakh", "Fusion tests from dendibakh blog");

    dendibakh->add(std::vector<Benchmark> {

        // https://dendibakh.github.io/blog/2018/01/18/Code_alignment_issues
        default_maker::template make_bench<dsb_alignment_cross64>(dendibakh.get(), "dsb-align64-cross", "Crosses 64-byte i-boundary", 1,
                []{ return aligned_ptr(1024, 1024); }, 1024),
        default_maker::template make_bench<dsb_alignment_nocross64>(dendibakh.get(), "dsb-align64-nocross", "No cross 64-byte i-boundary", 1,
                []{ return aligned_ptr(1024, 1024); }, 1024),

        default_maker::template make_bench<dendibakh_fused>  (dendibakh.get(),   "fused-original",  "Fused (original)",  1, null_provider, 1024),
        default_maker::template make_bench<dendibakh_fused_simple>  (dendibakh.get(),   "fused-simple",  "Fused (simple addr)", 1, null_provider, 1024),
        default_maker::template make_bench<dendibakh_fused_add>  (dendibakh.get(),"fused-add",  "Fused (add [reg + reg * 4], 1)",  1, null_provider, 1024),
        default_maker::template make_bench<dendibakh_fused_add_simple>  (dendibakh.get(),"fused-add-simple",  "Fused (add [reg], 1)",  1, null_provider, 1024),
        default_maker::template make_bench<dendibakh_unfused>(dendibakh.get(), "unfused-original","Unfused (original)",  1, null_provider, 1024),

        default_maker::template make_bench<fusion_better_fused>(dendibakh.get(), "fusion-better-fused", "Fused summation",  1, []{ return aligned_ptr(64, 8000); }, 1024),
        default_maker::template make_bench<fusion_better_unfused>(dendibakh.get(), "fusion-better-unfused", "Unfused summation",  1, []{ return aligned_ptr(64, 8000); }, 1024)

    });
    list.push_back(dendibakh);

    {
        std::shared_ptr<BenchmarkGroup> bmi_group = std::make_shared<BenchmarkGroup>("bmi", "BMI false-dependency tests");
        list.push_back(bmi_group);
        auto bmi_maker = DeltaMaker<TIMER>(bmi_group.get()).setTags({"default"});

        bmi_maker.template make<bmi_tzcnt>("dep-tzcnt", "dest-dependent tzcnt", 128);
        bmi_maker.template make<bmi_lzcnt>("dep-lzcnt", "dest-dependent lzcnt", 128);
        bmi_maker.template make<bmi_popcnt>("dep-popcnt", "dest-dependent popcnt", 128);
    }

    std::shared_ptr<BenchmarkGroup> retpoline_group = std::make_shared<BenchmarkGroup>("misc/retpoline", "retpoline tests");
    retpoline_group->add(std::vector<Benchmark> {
        default_maker::template make_bench<retpoline_dense_call_pause> (retpoline_group.get(), "retp-call-pause", "Dense retpoline call  pause", 32),
        default_maker::template make_bench<retpoline_dense_call_lfence>(retpoline_group.get(), "retp-call-lfence", "Dense retpoline call lfence", 32),
        default_maker::template make_bench<indirect_dense_call_pred>(retpoline_group.get(),    "ibra-call-pred", "Dense indirect pred calls", 32),
        default_maker::template make_bench<indirect_dense_call_unpred>(retpoline_group.get(),  "ibra-call-unpred", "Dense indirect unpred calls", 32),
        default_maker::template make_bench<retpoline_sparse_indep_call_pause,retpoline_sparse_call_base> (retpoline_group.get(), "retp-sparse-indep-call-pause", "Sparse retpo indep call  pause", 8),
        default_maker::template make_bench<retpoline_sparse_indep_call_lfence,retpoline_sparse_call_base>(retpoline_group.get(), "retp-sparse-indep-call-lfence", "Sparse retpo indep call lfence", 8),
        default_maker::template make_bench<retpoline_sparse_dep_call_pause,retpoline_sparse_call_base> (retpoline_group.get(),   "retp-sparse-dep-call-pause", "Sparse retpo dep call  pause", 8),
        default_maker::template make_bench<retpoline_sparse_dep_call_lfence,retpoline_sparse_call_base>(retpoline_group.get(),   "retp-sparse-dep-call-lfence", "Sparse retpo dep call lfence", 8)
    });
    list.push_back(retpoline_group);

    {
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("studies/vzeroall", "VZEROALL weirdness");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get()).setTags({"default"});
        auto maker256 = maker.setFeatures({AVX2});
        auto maker512 = maker.setFeatures({AVX512F});

        maker512.template make<vz_samereg>("vz512-samereg", "vpaddq zmm0, zmm0, zmm0", 100);
        maker512.template make<vz_diffreg>("vz512-diffreg", "vpaddq zmm0, zmm1, zmm0", 100);
        maker512.template make<vz_diffreg16>("vz512-diff16", "vpaddq zmm0, zmm16, zmm0", 100);
        maker512.template make<vz_diffreg16xor>("vz512-diff16xor", "vpxor zmm16; vpaddq zmm0, zmm16, zmm0", 100);
        maker256.template make<vz256_samereg>("vz256-samereg", "vpaddq ymm0, ymm0, ymm0", 100);
        maker256.template make<vz256_diffreg>("vz256-diffreg", "vpaddq ymm0, ymm1, ymm0", 100);
        maker256.template make<vz128_samereg>("vz128-samereg", "vpaddq xmm0, xmm0, xmm0", 100);
        maker256.template make<vz128_diffreg>("vz128-diffreg", "vpaddq xmm0, xmm1, xmm0", 100);
        maker256.template make<vzsse_samereg>("vzsse-samereg", "paddq xmm0, xmm0", 100);
        maker256.template make<vzsse_diffreg>("vzsse-diffreg", "paddq xmm0, xmm1", 100);
    }

}

#define REG_DEFAULT(CLOCK) template void register_misc<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



