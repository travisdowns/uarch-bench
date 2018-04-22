/*
 * cpp-benches.cpp
 *
 * Benchmarks written in C++.
 *
 */

#include "cpp-benches.hpp"

#include <limits>
#include <cinttypes>
#include <vector>
#include <random>

using std::size_t;
using std::uint64_t;

typedef uint64_t (div_func)(uint64_t);

static inline uint64_t somefunction_inline(uint64_t a) {
    return 10000 / a;
}

__attribute__ ((noinline))
uint64_t somefunction_noinline(uint64_t a) {
    return somefunction_inline(a);
}

template <div_func F>
long div64_lat_templ(uint64_t iters, void *arg) {
    uint64_t z = (uintptr_t)arg;
    for(uint64_t k = 0; k < iters; k++)
      z = F(z);
    return (long)z;
}

template <div_func F>
long div64_tput_templ(uint64_t iters, void *arg) {
    uint64_t z = 0;
    for(uint64_t k = 0; k < iters; k++)
      z += F(k + 1);
    return (long)z;
}

long div64_lat_inline(uint64_t iters, void *arg) {
    return div64_lat_templ<somefunction_inline>(iters, arg);
}

long div64_tput_inline(uint64_t iters, void *arg) {
    return div64_tput_templ<somefunction_inline>(iters, arg);
}

long div64_lat_noinline(uint64_t iters, void *arg) {
    return div64_lat_templ<somefunction_noinline>(iters, arg);
}

long div64_tput_noinline(uint64_t iters, void *arg) {
    return div64_tput_templ<somefunction_noinline>(iters, arg);
}



struct list_node {
    int value;
    list_node* next;
};

struct list_head {
    int size;
    list_node *first;
};

list_head makeList(int size) {
    list_head head = { size, nullptr };
    if (size != 0) {
        list_node* all_nodes = new list_node[size]();
        head.first = new list_node{ 1, nullptr };
        list_node* cur = head.first;
        while (--size > 0) {
            list_node* next = all_nodes++;
            cur->next = next;
            cur = next;
        }
    }
    return head;
}

constexpr int NODE_COUNT = 5;

std::vector<list_head> makeLists() {
    std::mt19937_64 engine;
    std::uniform_int_distribution<int> dist(0, NODE_COUNT * 2);
    std::vector<list_head> lists;
    for (int i = 0; i < LIST_COUNT; i++) {
        lists.push_back(makeList(NODE_COUNT));
//        lists.push_back(makeList(dist(engine)));
//        printf("SIZE %d\n", lists.back().size);
    }
    return lists;
}

std::vector<list_head> listOfLists = makeLists();

typedef long (list_sum)(list_head head);

template <list_sum SUM_IMPL>
long linkedlist_sum(uint64_t iters) {
    int sum = 0;
    while (iters-- > 0) {
        for (size_t list_index = 0; list_index < LIST_COUNT; list_index++) {
            sum += SUM_IMPL(listOfLists[list_index]);
        }
    }
    return sum;
}

long sum_counter(list_head list) {
    int sum = 0;
    list_node* cur = list.first;
    for (int i = 0; i < list.size; cur = cur->next, i++) {
        sum += cur->value;
    }
    return sum;
}

long sum_sentinel(list_head list) {
    int sum = 0;
    for (list_node* cur = list.first; cur; cur = cur->next) {
        sum += cur->value;
    }
    return sum;
}

long linkedlist_counter(uint64_t iters, void *arg) {
    return linkedlist_sum<sum_counter>(iters);
}

long linkedlist_sentinel(uint64_t iters, void *arg) {
    return linkedlist_sum<sum_sentinel>(iters);
}









