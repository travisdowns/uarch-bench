/*
 * cpp-benches.hpp
 */

#ifndef CPP_BENCHES_HPP_
#define CPP_BENCHES_HPP_

#include "bench-declarations.h"

bench2_f div64_lat_inline;
bench2_f div64_lat_noinline;
bench2_f div64_tput_inline;
bench2_f div64_tput_noinline;
bench2_f linkedlist_sentinel;
bench2_f linkedlist_counter;

constexpr int LIST_COUNT = 4000;


void* getLinkedList();

#endif /* CPP_BENCHES_HPP_ */
