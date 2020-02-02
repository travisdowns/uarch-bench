/*
 * util.cpp
 */

#include "util.hpp"

#include "opt-control.hpp"

#include <regex>
#include <cassert>
#include <numeric>
#include <random>
#include <cstring>
#include <exception>

#include <sys/mman.h>

#if !UARCH_BENCH_PORTABLE
#include <immintrin.h>
#endif

/* decide whether to use hugepages based on whether MADV_HUGEPAGE has it
 * as some very old kernels don't. */
#ifndef UARCH_BENCH_USE_HUGEPAGES
#ifdef MADV_HUGEPAGE
#define UARCH_BENCH_USE_HUGEPAGES 1
#else
#warning no MADV_HUGEPAGE on this system, huge pages not available
#define UARCH_BENCH_USE_HUGEPAGES 0
#endif
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#if defined(GCC_VERSION) && (GCC_VERSION < 40900)
#warning "Your gcc is too old to support regex, wildcard matches will be disabled - use gcc 4.9 or higher if you need them"
#define NO_CXX11_REGEX
#endif

#ifdef NO_CXX11_REGEX

// support only a basic form of wildcard matches with works with literal patterns (no wildcards)
// or a single trailing wildcard
bool wildcard_match(const std::string& target, const std::string& pattern) {
    auto first_star = pattern.find('*');
    if (first_star == std::string::npos) {
        return target == pattern;
    } else if (first_star == pattern.length() - 1) {
        std::string prefix = pattern.substr(0, pattern.length() - 1);
        return target.find(prefix) == 0;
    } else {
        throw std::runtime_error("You tried to use a non-trailing wildcard match, but this feature that requires C++ regex support");
    }
}

#else

std::string escape_for_regex(const std::string& input) {
    // see https://stackoverflow.com/a/40195721/149138
    static std::regex chars_to_escape{ R"([-[\]{}()*+?.\^$|])" };
    return std::regex_replace( input, chars_to_escape, R"(\$&)" );
}

bool wildcard_match(const std::string& target, const std::string& pattern) {
    // we implement wildcards just by replacing all *s with .*
    std::string escaped = escape_for_regex(std::move(pattern));
    std::string regex   = std::regex_replace(escaped, std::regex(R"(\\\*)"), R"(.*)");
    return std::regex_match(target, std::regex(regex));
}

#endif

const size_t TWO_MB = 2 * 1024 * 1024;
const int STORAGE_SIZE = 100 * 1024 * 1024;  // 100 MB
void *storage_ptr = 0;

volatile int zero = 0;
bool storage_init = false;

void *new_huge_ptr(size_t size) {
    void *ptr;
    int result = posix_memalign(&ptr, TWO_MB, size + TWO_MB);
    assert(result == 0);
#if UARCH_BENCH_USE_HUGEPAGES
    madvise(ptr, size + TWO_MB, MADV_HUGEPAGE);
    ptr = ((char *)ptr + TWO_MB);
#else
    static bool once;
    if (!once) {
        fprintf(stderr, "WARNING: huge pages not available\n");
        once = true;
    }
#endif
    // It is critical that we memset the memory region to touch each page, otherwise all or some pages
    // can be mapped to the zero page, leading to unexpected results for read-only tests (i.e., "too good to be true"
    // results for benchmarks that read large amounts of memory, because internally these are all mapped
    // to the same page.
    // We do a memset of 1 followed by zero since some compilers are recognizing the pattern of malloc/memset-to-zero
    // and transforming it to calloc, which again can result in the zero-page behavior described above. We aren't actually
    // using malloc() currently (rather we use posix_memalign), but this might change or compilers might get even smarter.
    std::memset(ptr, 1, size);
    std::memset(ptr, 0, size);
    return ptr;
}

void *align(size_t base_alignment, size_t required_size, void* p, size_t space) {
    /* std::align isn't available in GCC and clang until fairly
     * recently. This just gives us a bit more portability for older
     * compilers. Code from https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57350#c11 */
    void *r;
    {
        std::uintptr_t pn = reinterpret_cast< std::uintptr_t >( p );
        std::uintptr_t aligned = ( pn + base_alignment - 1 ) & - base_alignment;
        std::size_t padding = aligned - pn;
        if ( space < required_size + padding )
            r = nullptr;
        space -= padding;
        r = reinterpret_cast< void * >( aligned );
    }
    assert(r);
    assert((((uintptr_t)r) & (base_alignment - 1)) == 0);
    return r;
}

void *aligned_ptr(size_t base_alignment, size_t required_size, bool set_zero) {
    assert(required_size <= STORAGE_SIZE);
    assert(is_pow2(base_alignment));
    assert(base_alignment <= TWO_MB);
    if (!storage_ptr) {
        storage_ptr = new_huge_ptr(STORAGE_SIZE);
    }
    auto p = align(base_alignment, required_size, storage_ptr, STORAGE_SIZE);
    if (set_zero) {
        memset(p, 0, required_size);
    }
    return p;
}


/**
 * Returns a pointer that is first aligned to the given base alignment (per
 * aligned_ptr()) and then is offset by the amount given by misalignment.
 */
void *misaligned_ptr(size_t base_alignment, size_t required_size, ssize_t misalignment) {
    char *p = static_cast<char *>(aligned_ptr(base_alignment, required_size));
    return p + misalignment;
}

static_assert(UB_CACHE_LINE_SIZE == sizeof(CacheLine), "sizeof(CacheLine) not equal to actual cache line size, huh?");

// I don't think this going to fail on any platform in common use today, but who knows?
static_assert(sizeof(CacheLine) == UB_CACHE_LINE_SIZE, "really weird layout on this platform");

size_t count(CacheLine* first) {
    CacheLine* p = first;
    size_t count = 0;
    do {
        p = p->nexts[0];
        count++;
    } while (p != first);
    return count;
}

/**
 * Return a region of memory of size bytes, where each cache line sized chunk points to another random chunk
 * within the region. The pointers cover all chunks in a cycle of maximum size.
 *
 * The region_struct is returned by reference and points to a static variable that is overwritten every time
 * this function is called.
 */
region& shuffled_region(const size_t size, const size_t offset) {
#if UARCH_BENCH_PORTABLE
    assert(false); // need xplat impl of clflush below
    abort();
#else
    assert(size + offset <= MAX_SHUFFLED_REGION_SIZE);
    assert(size % UB_CACHE_LINE_SIZE == 0);
    size_t size_lines = size / UB_CACHE_LINE_SIZE;
    assert(size_lines > 0);

    // only get the storage once and keep re-using it, to minimize variance (e.g., some benchmarks getting huge pages
    // and others not, etc)
    static char* storage_ = static_cast<char*>(new_huge_ptr(MAX_SHUFFLED_REGION_SIZE));

    // NOTE: for non-zero offset this is technically UB since storage won't be aligned appropriately, currently not
    // a problem on x86 with any compiler I'm aware of but perhaps we should use a final memmove to apply the offset
    CacheLine* storage = reinterpret_cast<CacheLine*>(storage_ + offset);

    std::memset(storage, -1, size);

    std::vector<size_t> indexes(size_lines);
    std::iota(indexes.begin(), indexes.end(), 0);
    std::shuffle(indexes.begin(), indexes.end(), std::mt19937_64{123});

    CacheLine* p = storage + indexes[0];
    for (size_t i = 1; i < size_lines; i++) {
        CacheLine* next = storage + indexes[i];
        p->setNexts(next);
        p = next;
    }
    p->setNexts(storage + indexes[0]);

    assert(count(storage) == size_lines);

    for (char *p = (char *)storage, *e = p + size; p < e; p += UB_CACHE_LINE_SIZE) {
        _mm_clflush(p);
    }

    _mm_mfence();

    return *(new region{ size, storage }); // leak
#endif
}

long touch_lines(void *region, size_t size, size_t stride) {
    if (size == 0) {
        return 0;
    }
    char sum = 0;
    volatile char* cregion = reinterpret_cast<volatile char*>(region);
    for (volatile char *r = cregion; r < cregion + size; r += stride) {
        sum += *r;
    }
    sum += cregion[size - 1];
    return sum;
}

template <typename A>
void flush_helper(size_t working_set, A alloc) {
    const size_t EXTRA_SIZE = 4096; // we will allocate this much extra on each end to avoid prefetching effects
    static region buffer = {0, nullptr};

    if (buffer.size < working_set) {
        size_t total_size = working_set + 2 * EXTRA_SIZE;
        auto p = alloc(total_size);
        memset(p, 1, total_size);
        buffer.start = (char *)p + EXTRA_SIZE;
        buffer.size  = working_set;
    }

    char sum = 0;
    volatile char* vptr = reinterpret_cast<volatile char*>(buffer.start);
    for (size_t size = 1; ; size *= 2) {
        // use powers of two except on the last iteration
        if (size > working_set) {
            size = working_set;
        }

        for (size_t ifast = 0, islow = 0; islow < size; islow += UB_CACHE_LINE_SIZE / 2, ifast += UB_CACHE_LINE_SIZE) {
            if (HEDLEY_UNLIKELY(ifast >= working_set)) {
                ifast = 0; // wrap
            }
            sum ^= vptr[islow];
            sum += vptr[ifast];
//            vptr[islow]++
        }

        if (size == working_set) {
            break;
        }
    }

    opt_control::sink(sum);
}

void flush_caches(size_t working_set) {
    static_assert(UB_CACHE_LINE_SIZE > 1, "UB_CACHE_LINE_SIZE must be >1 because we do /2");
    flush_helper(working_set, new_huge_ptr);
    flush_helper(working_set, [](size_t s){ return new char[s]; });
}



std::string errno_to_str(int e) {
    char buf[128];
#ifndef _GNU_SOURCE
// We expect _GNU_SOURCE to be defined here since apparently g++ forces it, and we use it to know which
// version of strerror_r we are going to get. If it isn't defined somewhere, we may have to support the POSIX
// version of it too (returns an int)
#error "_GNU_SOURCE not defined"
#endif
    return strerror_r(e, buf, sizeof(buf));
}

int always_zero() {
    return zero;
}

