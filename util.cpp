/*
 * util.cpp
 */

#include "util.hpp"

#include <regex>
#include <cassert>

#include <sys/mman.h>


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

const size_t TWO_MB = 2 * 1024 * 1024;
const int STORAGE_SIZE = TWO_MB;  // * 4 because we overalign the pointer in order to guarantee minimal alignemnt
//unsigned char unaligned_storage[STORAGE_SIZE];
void *storage_ptr = 0;

volatile int zero = 0;
bool storage_init = false;

/**
 * Return a new pointer to a memory region of at least size, aligned to a 2MB boundary and with
 * an effort to ensure the pointer is backed by transparent huge pages.
 */
void *new_huge_ptr(size_t size) {
    void *ptr;
    int result = posix_memalign(&ptr, TWO_MB, size + TWO_MB);
    assert(result == 0);
    madvise(ptr, size + TWO_MB, MADV_HUGEPAGE);
    ptr = ((char *)ptr + TWO_MB);
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

/*
 * Returns a pointer that is minimally aligned to base_alignment. That is, it is
 * aligned to base_alignment, but *not* aligned to 2 * base_alignment. Each call returns
 * the same pointer, so you probably shouldn't write to this memory region.
 */
void *aligned_ptr(size_t base_alignment, size_t required_size) {
    assert(required_size <= STORAGE_SIZE);
    assert(is_pow2(base_alignment));
    assert(base_alignment < TWO_MB);
    if (!storage_ptr) {
        storage_ptr = new_huge_ptr(STORAGE_SIZE);
    }
    return align(base_alignment, required_size, storage_ptr, STORAGE_SIZE);
}


/**
 * Returns a pointer that is first *minimally* aligned to the given base alignment (per
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
region& shuffled_region(const size_t size) {
    assert(size <= MAX_SHUFFLED_REGION_SIZE);
    assert(size % UB_CACHE_LINE_SIZE == 0);
    size_t size_lines = size / UB_CACHE_LINE_SIZE;
    assert(size_lines > 0);

    // only get the storage once and keep re-using it, to minimize variance (e.g., some benchmarks getting huge pages
    // and others not, etc)
    static CacheLine* storage = (CacheLine*)new_huge_ptr(MAX_SHUFFLED_REGION_SIZE);

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

    return *(new region{ size, storage }); // leak
}
