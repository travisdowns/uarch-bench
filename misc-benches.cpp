/*
 * default_benches.cpp
 *
 * Various "default" benchmarks.
 */

#include "benches.hpp"
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

bench2_f nehalem_sub1;
bench2_f nehalem_sub2;
bench2_f nehalem_sub3;
bench2_f nehalem_sub4;
bench2_f nehalem_sub5;
bench2_f nehalem_sub6;
bench2_f nehalem_sub7;
bench2_f nehalem_sub8;
bench2_f nehalem_sub9;
bench2_f nehalem_sub10;
bench2_f nehalem_sub11;
bench2_f nehalem_sub12;
bench2_f nehalem_sub13;
bench2_f nehalem_sub14;
bench2_f nehalem_sub15;
bench2_f nehalem_sub16;
bench2_f nehalem_sub17;
bench2_f nehalem_sub18;
bench2_f nehalem_sub19;
bench2_f nehalem_sub20;
bench2_f nehalem_sub21;
bench2_f nehalem_sub22;
bench2_f nehalem_sub23;
bench2_f nehalem_sub24;
bench2_f nehalem_sub25;
bench2_f nehalem_sub26;
bench2_f nehalem_sub27;
bench2_f nehalem_sub28;
bench2_f nehalem_sub29;
bench2_f nehalem_sub30;
bench2_f nehalem_sub31;
bench2_f nehalem_sub32;
bench2_f nehalem_sub33;
bench2_f nehalem_sub34;
bench2_f nehalem_sub35;
bench2_f nehalem_sub36;
bench2_f nehalem_sub37;
bench2_f nehalem_sub38;
bench2_f nehalem_sub39;
bench2_f nehalem_sub40;
bench2_f nehalem_sub41;
bench2_f nehalem_sub42;
bench2_f nehalem_sub43;
bench2_f nehalem_sub44;
bench2_f nehalem_sub45;
bench2_f nehalem_sub46;
bench2_f nehalem_sub47;
bench2_f nehalem_sub48;
bench2_f nehalem_sub49;
bench2_f nehalem_sub50;
bench2_f nehalem_sub51;
bench2_f nehalem_sub52;
bench2_f nehalem_sub53;
bench2_f nehalem_sub54;
bench2_f nehalem_sub55;
bench2_f nehalem_sub56;
bench2_f nehalem_sub57;
bench2_f nehalem_sub58;
bench2_f nehalem_sub59;
bench2_f nehalem_sub60;
bench2_f nehalem_sub61;
bench2_f nehalem_sub62;
bench2_f nehalem_sub63;
bench2_f nehalem_sub64;
bench2_f nehalem_sub65;
bench2_f nehalem_sub66;
bench2_f nehalem_sub67;
bench2_f nehalem_sub68;
bench2_f nehalem_sub69;
bench2_f nehalem_sub70;
bench2_f nehalem_sub71;
bench2_f nehalem_sub72;
bench2_f nehalem_sub73;
bench2_f nehalem_sub74;
bench2_f nehalem_sub75;
bench2_f nehalem_sub76;
bench2_f nehalem_sub77;
bench2_f nehalem_sub78;
bench2_f nehalem_sub79;
bench2_f nehalem_sub80;
bench2_f nehalem_sub81;
bench2_f nehalem_sub82;
bench2_f nehalem_sub83;
bench2_f nehalem_sub84;
bench2_f nehalem_sub85;
bench2_f nehalem_sub86;
bench2_f nehalem_sub87;
bench2_f nehalem_sub88;
bench2_f nehalem_sub89;
bench2_f nehalem_sub90;
bench2_f nehalem_sub91;
bench2_f nehalem_sub92;
bench2_f nehalem_sub93;
bench2_f nehalem_sub94;
bench2_f nehalem_sub95;
bench2_f nehalem_sub96;
bench2_f nehalem_sub97;
bench2_f nehalem_sub98;
bench2_f nehalem_sub99;
bench2_f nehalem_sub100;

}

template <typename TIMER>
void register_misc(GroupList& list) {
    std::shared_ptr<BenchmarkGroup> misc_group = std::make_shared<BenchmarkGroup>("misc", "Miscellaneous tests");

    using default_maker = StaticMaker<TIMER>;

    const uint32_t iters = 10*1000;
    auto benches = std::vector<Benchmark> {
        default_maker::template make_bench<misc_add_loop32>(misc_group.get(), "add-32", "32-bit add-loop", 1,
                null_provider, iters),
        default_maker::template make_bench<misc_add_loop64>(misc_group.get(), "add-64", "64-bit add-loop", 1,
                null_provider, iters),
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
        default_maker::template make_bench<tight_loop1>(misc_group.get(), "tight-loop1", "Tight dec loop", 1,
                null_provider, iters * 10),
        default_maker::template make_bench<tight_loop2>(misc_group.get(), "tight-loop2", "Tight dec loop taken jmp", 1,
                null_provider, iters * 10),
        default_maker::template make_bench<tight_loop3>(misc_group.get(), "tight-loop3", "Tight dec loop untaken jmp", 1,
                null_provider, iters * 10),

        // https://news.ycombinator.com/item?id=15935283
        default_maker::template make_bench<loop_weirdness_fast>(misc_group.get(), "loop-weirdness-fast", "Loop weirdness fast", 1,
                []{ return aligned_ptr(1024, 1024); }, 10000),
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
        std::shared_ptr<BenchmarkGroup> group = std::make_shared<BenchmarkGroup>("studies/nehalem", "BMI false-dependency tests");
        list.push_back(group);
        auto maker = DeltaMaker<TIMER>(group.get());

        maker.template make<nehalem_sub1>("sub1", "Subtract unrolled 1 times", 1);
        maker.template make<nehalem_sub2>("sub2", "Subtract unrolled 2 times", 1);
        maker.template make<nehalem_sub3>("sub3", "Subtract unrolled 3 times", 1);
        maker.template make<nehalem_sub4>("sub4", "Subtract unrolled 4 times", 1);
        maker.template make<nehalem_sub5>("sub5", "Subtract unrolled 5 times", 1);
        maker.template make<nehalem_sub6>("sub6", "Subtract unrolled 6 times", 1);
        maker.template make<nehalem_sub7>("sub7", "Subtract unrolled 7 times", 1);
        maker.template make<nehalem_sub8>("sub8", "Subtract unrolled 8 times", 1);
        maker.template make<nehalem_sub9>("sub9", "Subtract unrolled 9 times", 1);
        maker.template make<nehalem_sub10>("sub10", "Subtract unrolled 10 times", 1);
        maker.template make<nehalem_sub11>("sub11", "Subtract unrolled 11 times", 1);
        maker.template make<nehalem_sub12>("sub12", "Subtract unrolled 12 times", 1);
        maker.template make<nehalem_sub13>("sub13", "Subtract unrolled 13 times", 1);
        maker.template make<nehalem_sub14>("sub14", "Subtract unrolled 14 times", 1);
        maker.template make<nehalem_sub15>("sub15", "Subtract unrolled 15 times", 1);
        maker.template make<nehalem_sub16>("sub16", "Subtract unrolled 16 times", 1);
        maker.template make<nehalem_sub17>("sub17", "Subtract unrolled 17 times", 1);
        maker.template make<nehalem_sub18>("sub18", "Subtract unrolled 18 times", 1);
        maker.template make<nehalem_sub19>("sub19", "Subtract unrolled 19 times", 1);
        maker.template make<nehalem_sub20>("sub20", "Subtract unrolled 20 times", 1);
        maker.template make<nehalem_sub21>("sub21", "Subtract unrolled 21 times", 1);
        maker.template make<nehalem_sub22>("sub22", "Subtract unrolled 22 times", 1);
        maker.template make<nehalem_sub23>("sub23", "Subtract unrolled 23 times", 1);
        maker.template make<nehalem_sub24>("sub24", "Subtract unrolled 24 times", 1);
        maker.template make<nehalem_sub25>("sub25", "Subtract unrolled 25 times", 1);
        maker.template make<nehalem_sub26>("sub26", "Subtract unrolled 26 times", 1);
        maker.template make<nehalem_sub27>("sub27", "Subtract unrolled 27 times", 1);
        maker.template make<nehalem_sub28>("sub28", "Subtract unrolled 28 times", 1);
        maker.template make<nehalem_sub29>("sub29", "Subtract unrolled 29 times", 1);
        maker.template make<nehalem_sub30>("sub30", "Subtract unrolled 30 times", 1);
        maker.template make<nehalem_sub31>("sub31", "Subtract unrolled 31 times", 1);
        maker.template make<nehalem_sub32>("sub32", "Subtract unrolled 32 times", 1);
        maker.template make<nehalem_sub33>("sub33", "Subtract unrolled 33 times", 1);
        maker.template make<nehalem_sub34>("sub34", "Subtract unrolled 34 times", 1);
        maker.template make<nehalem_sub35>("sub35", "Subtract unrolled 35 times", 1);
        maker.template make<nehalem_sub36>("sub36", "Subtract unrolled 36 times", 1);
        maker.template make<nehalem_sub37>("sub37", "Subtract unrolled 37 times", 1);
        maker.template make<nehalem_sub38>("sub38", "Subtract unrolled 38 times", 1);
        maker.template make<nehalem_sub39>("sub39", "Subtract unrolled 39 times", 1);
        maker.template make<nehalem_sub40>("sub40", "Subtract unrolled 40 times", 1);
        maker.template make<nehalem_sub41>("sub41", "Subtract unrolled 41 times", 1);
        maker.template make<nehalem_sub42>("sub42", "Subtract unrolled 42 times", 1);
        maker.template make<nehalem_sub43>("sub43", "Subtract unrolled 43 times", 1);
        maker.template make<nehalem_sub44>("sub44", "Subtract unrolled 44 times", 1);
        maker.template make<nehalem_sub45>("sub45", "Subtract unrolled 45 times", 1);
        maker.template make<nehalem_sub46>("sub46", "Subtract unrolled 46 times", 1);
        maker.template make<nehalem_sub47>("sub47", "Subtract unrolled 47 times", 1);
        maker.template make<nehalem_sub48>("sub48", "Subtract unrolled 48 times", 1);
        maker.template make<nehalem_sub49>("sub49", "Subtract unrolled 49 times", 1);
        maker.template make<nehalem_sub50>("sub50", "Subtract unrolled 50 times", 1);
        maker.template make<nehalem_sub51>("sub51", "Subtract unrolled 51 times", 1);
        maker.template make<nehalem_sub52>("sub52", "Subtract unrolled 52 times", 1);
        maker.template make<nehalem_sub53>("sub53", "Subtract unrolled 53 times", 1);
        maker.template make<nehalem_sub54>("sub54", "Subtract unrolled 54 times", 1);
        maker.template make<nehalem_sub55>("sub55", "Subtract unrolled 55 times", 1);
        maker.template make<nehalem_sub56>("sub56", "Subtract unrolled 56 times", 1);
        maker.template make<nehalem_sub57>("sub57", "Subtract unrolled 57 times", 1);
        maker.template make<nehalem_sub58>("sub58", "Subtract unrolled 58 times", 1);
        maker.template make<nehalem_sub59>("sub59", "Subtract unrolled 59 times", 1);
        maker.template make<nehalem_sub60>("sub60", "Subtract unrolled 60 times", 1);
        maker.template make<nehalem_sub61>("sub61", "Subtract unrolled 61 times", 1);
        maker.template make<nehalem_sub62>("sub62", "Subtract unrolled 62 times", 1);
        maker.template make<nehalem_sub63>("sub63", "Subtract unrolled 63 times", 1);
        maker.template make<nehalem_sub64>("sub64", "Subtract unrolled 64 times", 1);
        maker.template make<nehalem_sub65>("sub65", "Subtract unrolled 65 times", 1);
        maker.template make<nehalem_sub66>("sub66", "Subtract unrolled 66 times", 1);
        maker.template make<nehalem_sub67>("sub67", "Subtract unrolled 67 times", 1);
        maker.template make<nehalem_sub68>("sub68", "Subtract unrolled 68 times", 1);
        maker.template make<nehalem_sub69>("sub69", "Subtract unrolled 69 times", 1);
        maker.template make<nehalem_sub70>("sub70", "Subtract unrolled 70 times", 1);
        maker.template make<nehalem_sub71>("sub71", "Subtract unrolled 71 times", 1);
        maker.template make<nehalem_sub72>("sub72", "Subtract unrolled 72 times", 1);
        maker.template make<nehalem_sub73>("sub73", "Subtract unrolled 73 times", 1);
        maker.template make<nehalem_sub74>("sub74", "Subtract unrolled 74 times", 1);
        maker.template make<nehalem_sub75>("sub75", "Subtract unrolled 75 times", 1);
        maker.template make<nehalem_sub76>("sub76", "Subtract unrolled 76 times", 1);
        maker.template make<nehalem_sub77>("sub77", "Subtract unrolled 77 times", 1);
        maker.template make<nehalem_sub78>("sub78", "Subtract unrolled 78 times", 1);
        maker.template make<nehalem_sub79>("sub79", "Subtract unrolled 79 times", 1);
        maker.template make<nehalem_sub80>("sub80", "Subtract unrolled 80 times", 1);
        maker.template make<nehalem_sub81>("sub81", "Subtract unrolled 81 times", 1);
        maker.template make<nehalem_sub82>("sub82", "Subtract unrolled 82 times", 1);
        maker.template make<nehalem_sub83>("sub83", "Subtract unrolled 83 times", 1);
        maker.template make<nehalem_sub84>("sub84", "Subtract unrolled 84 times", 1);
        maker.template make<nehalem_sub85>("sub85", "Subtract unrolled 85 times", 1);
        maker.template make<nehalem_sub86>("sub86", "Subtract unrolled 86 times", 1);
        maker.template make<nehalem_sub87>("sub87", "Subtract unrolled 87 times", 1);
        maker.template make<nehalem_sub88>("sub88", "Subtract unrolled 88 times", 1);
        maker.template make<nehalem_sub89>("sub89", "Subtract unrolled 89 times", 1);
        maker.template make<nehalem_sub90>("sub90", "Subtract unrolled 90 times", 1);
        maker.template make<nehalem_sub91>("sub91", "Subtract unrolled 91 times", 1);
        maker.template make<nehalem_sub92>("sub92", "Subtract unrolled 92 times", 1);
        maker.template make<nehalem_sub93>("sub93", "Subtract unrolled 93 times", 1);
        maker.template make<nehalem_sub94>("sub94", "Subtract unrolled 94 times", 1);
        maker.template make<nehalem_sub95>("sub95", "Subtract unrolled 95 times", 1);
        maker.template make<nehalem_sub96>("sub96", "Subtract unrolled 96 times", 1);
        maker.template make<nehalem_sub97>("sub97", "Subtract unrolled 97 times", 1);
        maker.template make<nehalem_sub98>("sub98", "Subtract unrolled 98 times", 1);
        maker.template make<nehalem_sub99>("sub99", "Subtract unrolled 99 times", 1);
        maker.template make<nehalem_sub100>("sub100", "Subtract unrolled 100 times", 1);



    }
}

#define REG_DEFAULT(CLOCK) template void register_misc<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



