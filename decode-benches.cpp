/*
 * Decode tests.
 * 
 * Test that cover aspects of the front-end instruction and uop deliver
 * such as decoding, DSB, LSD, etc.
 */

#include "benchmark.hpp"
#include "util.hpp"
#include "boost/preprocessor/repetition/repeat_from_to.hpp"

extern "C" {
/* misc benches */

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
bench2_f decode_monoid2;
bench2_f decode_monoid3;

// bench2_f quadratic;
BOOST_PP_REPEAT_FROM_TO(35, 47, DECL_BENCH2, quadratic)
BOOST_PP_REPEAT_FROM_TO(48, 50, DECL_BENCH2, quadratic)

}


template <typename TIMER>
void register_decode(GroupList& list) {
#if !UARCH_BENCH_PORTABLE

    auto group = std::make_shared<BenchmarkGroup>("decode", "Decode tests");
    using default_maker = StaticMaker<TIMER>;

    const size_t decode_ops = 50400/2;

    auto benches = std::vector<Benchmark> {

    // legacy (MITE) decode tests
    default_maker::template make_bench<decode33334>(group.get(),   "decode33334", "Decode 3-3-3-3-4 byte nops", decode_ops, null_provider, 1000),
    default_maker::template make_bench<decode33333>(group.get(),   "decode33333", "Decode 3-3-3-3-3 byte nops", decode_ops, null_provider, 1000),
    default_maker::template make_bench<decode16x1>(group.get(),    "decode16x1",  "Decode 16x1 byte nops",      decode_ops, null_provider, 1000),
    default_maker::template make_bench<decode8x2>(group.get(),     "decode8x2",   "Decode 8x2 byte nops",       decode_ops, null_provider, 1000),
    default_maker::template make_bench<decode4x4>(group.get(),     "decode4x4",   "Decode 4x4 byte nops",       decode_ops, null_provider, 1000),
    default_maker::template make_bench<decode664>(group.get(),     "decode664",   "Decode 6-6-4 byte nops",     decode_ops, null_provider, 1000),
    default_maker::template make_bench<decode88>(group.get(),      "decode88",    "Decode 8-8 byte nops",       decode_ops, null_provider, 1000),
    default_maker::template make_bench<decode8833334>(group.get(), "decode8833334", "Decode 8-8-3-3-3-3-4 byte nops",     decode_ops, null_provider, 1000),
    default_maker::template make_bench<decode884444>(group.get(),  "decode884444",  "Decode 8-8-4-4-4-4 byte nops",     decode_ops, null_provider, 1000),
    default_maker::template make_bench<decode_monoid>(group.get(),  "decode-monoid",  "Decode 33334x10, 556x10 blocks", 3200, null_provider, 1000),
    default_maker::template make_bench<decode_monoid2>(group.get(),  "decode-monoid2",  "monoid2", 9000, null_provider, 1000),
    default_maker::template make_bench<decode_monoid3>(group.get(),  "decode-monoid3",  "monoid3", 9000, null_provider, 1000),


        // shows that a loop split by a 64-byte boundary takes at least 2 cycles, probably because the DSB can deliever
        // from only one set per cycle
#define MAKE_QUAD(z, n, _) \
        default_maker::template make_bench<quadratic ## n>(group.get(), "quad" #n, "nested loops offset: " #n, 1000, \
                [=]{ return aligned_ptr(4, 1024 * sizeof(uint32_t)); }, 1000),

        BOOST_PP_REPEAT_FROM_TO(35, 37, MAKE_QUAD, ignore)
        BOOST_PP_REPEAT_FROM_TO(48, 50, MAKE_QUAD, ignore)

    };

    group->add(benches);
    list.push_back(group);


#endif // #if !UARCH_BENCH_PORTABLE

}

#define REG_DEFAULT(CLOCK) template void register_decode<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



