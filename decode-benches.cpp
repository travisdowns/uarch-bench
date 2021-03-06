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
bench2_f decode333322;
bench2_f decode3333211;
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

bench2_f decode_complex_211;
bench2_f decode_complex_2111;
bench2_f decode_complex_21111;
bench2_f decode_complex_31;
bench2_f decode_complex_311;
bench2_f decode_complex_3111;

// bench2_f quadratic;
BOOST_PP_REPEAT_FROM_TO(35, 47, DECL_BENCH2, quadratic)
BOOST_PP_REPEAT_FROM_TO(48, 50, DECL_BENCH2, quadratic)

}


template <typename TIMER>
void register_decode(GroupList& list) {
#if !UARCH_BENCH_PORTABLE

    auto group = std::make_shared<BenchmarkGroup>("decode", "Decode tests");

    const size_t decode_ops = 50400/2;

    auto maker = DeltaMaker<TIMER>(group.get(), 1000);

    // legacy (MITE) decode tests
    maker.template make<decode33334>   ("decode33334",  "Decode 3-3-3-3-4 byte nops", decode_ops);
    maker.template make<decode333322>  ("decode333322", "Decode 3-3-3-3-2-2 byte nops", decode_ops);
    maker.template make<decode3333211> ("decode3333211","Decode 3-3-3-3-2-1-1 byte nops", decode_ops);
    maker.template make<decode33333>   ("decode33333",  "Decode 3-3-3-3-3 byte nops", decode_ops);
    maker.template make<decode16x1>    ("decode16x1",   "Decode 16x1 byte nops",      decode_ops);
    maker.template make<decode8x2>     ("decode8x2",    "Decode 8x2 byte nops",       decode_ops);
    maker.template make<decode4x4>     ("decode4x4",    "Decode 4x4 byte nops",       decode_ops);
    maker.template make<decode664>     ("decode664",    "Decode 6-6-4 byte nops",     decode_ops);
    maker.template make<decode88>      ("decode88",     "Decode 8-8 byte nops",       decode_ops);
    maker.template make<decode8833334> ("decode8833334", "Decode 8-8-3-3-3-3-4 byte nops",     decode_ops);
    maker.template make<decode884444>  ("decode884444",  "Decode 8-8-4-4-4-4 byte nops",     decode_ops);
    maker.template make<decode_monoid> ("decode-monoid",  "Decode 33334x10, 556x10 blocks", 3200);
    maker.template make<decode_monoid2>("decode-monoid2",  "monoid2", 9000);
    maker.template make<decode_monoid3>("decode-monoid3",  "monoid3", 9000);


    // these tests use a complex instruction (2 uops) to test the patterns that can be decoded
    // in 1 cycle. If the decoder cannot decode the entire sequence (e.g., 2111 is 4 instructions with
    // 2, 1, 1, 1 uops each) it will take 2 cycles, otherwise less (with the exact amount usually depending
    // on the rename limit of 4).
#define DEFINE_DECODE_COMPLEX(suffix) maker.setLoopCount(10000).template make<decode_complex_##suffix> \
        ("decode_complex_" #suffix, "decode_complex_" #suffix, 200)

    DEFINE_DECODE_COMPLEX(211);
    DEFINE_DECODE_COMPLEX(2111);
    DEFINE_DECODE_COMPLEX(21111);

    DEFINE_DECODE_COMPLEX(31);
    DEFINE_DECODE_COMPLEX(311);
    DEFINE_DECODE_COMPLEX(3111);

    // shows that a loop split by a 64-byte boundary takes at least 2 cycles, probably because the DSB can deliever
    // from only one set per cycle
#define MAKE_QUAD(z, n, _) \
    maker.template make<quadratic ## n>("quad" #n, "nested loops offset: " #n, 1000, \
            [=]{ return aligned_ptr(4, 1024 * sizeof(uint32_t)); });

    BOOST_PP_REPEAT_FROM_TO(35, 37, MAKE_QUAD, ignore)
    BOOST_PP_REPEAT_FROM_TO(48, 50, MAKE_QUAD, ignore)

    list.push_back(group);


#endif // #if !UARCH_BENCH_PORTABLE

}

#define REG_DEFAULT(CLOCK) template void register_decode<CLOCK>(GroupList& list);

ALL_TIMERS_X(REG_DEFAULT)



