/*
 * util.hpp
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <chrono>
#include <string>
#include <cstdio>
#include <memory>
#include <vector>
#include <sstream>

#define UB_CACHE_LINE_SIZE    64

#if USE_LIBPFC
#define IF_PFC(x) x
#else
#define IF_PFC(x)
#endif

// returns the argument followed by a comma, often useful to pass to x-macros
#define APPEND_COMMA(x) x,

template <typename T>
static inline bool is_pow2(T x) {
    static_assert(std::is_unsigned<T>::value, "must use unsigned integral types");
    return x && !(x & (x - 1));
}

/* use some reasonable default clock to return a point in time measured in nanos,
 * which has no relation to wall-clock time (is suitable for measuring intervals)
 */
static inline int64_t nanos() {
    auto t = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(t).time_since_epoch().count();
}

/**
 * Return a pointer to a NEWLY ALLOCATED memory region of at least size, aligned to a 2MB boundary and with
 * an effort to ensure the pointer is backed by transparent huge pages.
 */
void *new_huge_ptr(size_t size);

/**
 * Return a pointer to a region of the of the given size and alignment. The same region is repeated REUSED.
 */
void *aligned_ptr(size_t base_alignment, size_t required_size, bool set_zero = false);
void *misaligned_ptr(size_t base_alignment, size_t required_size, ssize_t misalignment);

/*
 * Given a printf-style format and args, return the formatted string as a std::string.
 *
 * See https://stackoverflow.com/a/26221725/149138.
 */
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args) {
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf( new char[ size ] );
    snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

/* stricty speaking this overload is not necessary but it avoids a gcc warning about a format operation without args */
static inline std::string string_format(const std::string& format) {
    return format;
}

/** make a string like [1,2,3] out of anything supporting std::begin/end and whose elements support ostream << */
template <typename T>
std::string container_to_string(const T& container) {
    std::stringstream ss;
    ss << "[";
    bool first = true;
    for (const auto& e : container) {
        if (!first) ss << ",";
        first = false;
        ss << e;
    }
    ss << "]";
    return ss.str();
}




/*
 * Split a string delimited by sep.
 *
 * See https://stackoverflow.com/a/7408245/149138
 */
template <typename S>
static inline std::vector<std::string> split_helper(const std::string &text, S splitfunc) {
  using ptype = decltype(splitfunc(text,0));
  std::vector<std::string> tokens;
  std::size_t start = 0;
  ptype result;
  while ((result = splitfunc(text, start)).first != std::string::npos) {
    tokens.push_back(text.substr(start, result.first - start));
    start = result.first + result.second;
  }
  tokens.push_back(text.substr(start));
  return tokens;
}


/** splits a string based on finding occurrences of the entire passed separator string */
static inline std::vector<std::string> split_on_string(const std::string &text, const std::string& sep) {
    return split_helper(text, [=](const std::string& s, size_t start) { return std::make_pair(s.find(sep, start), sep.length()); });
}

/** splits on any of the characters in sep_chars */
static inline std::vector<std::string> split_on_any(const std::string &text, const std::string& sep_chars) {
    return split_helper(text, [=](const std::string& s, size_t start) { return std::make_pair(s.find_first_of(sep_chars, start), 1); });
}

/** Take a string and escape it so that it will be treated as a literal string in a regex */
std::string escape_for_regex(const std::string& input);

/**
 * Returns true if the entire string target matches pattern, where pattern can contain * wildcards
 * that match any number of characters.
 */
bool wildcard_match(const std::string& target, const std::string& pattern);

constexpr size_t MAX_SHUFFLED_REGION_SIZE = 400 * 1024 * 1024;

/** the whole cache line object is filled with pointers to the next chunk */
struct CacheLine {
    static_assert(UB_CACHE_LINE_SIZE % sizeof(CacheLine *) == 0, "cache line size not a multiple of pointer size");
    CacheLine* nexts[UB_CACHE_LINE_SIZE / sizeof(CacheLine *)];

    void setNexts(CacheLine* next) {
        std::fill(std::begin(nexts), std::end(nexts), next);
    }
};

struct region {
    size_t size;
    void *start;  // actually a CacheLine object
};

/**
 * Return a region of memory of size bytes, where each cache line sized chunk points to another random chunk
 * within the region. The pointers cover all chunks in a cycle of maximum size.
 *
 * The region_struct is returned by reference and points to a static variable that is overwritten every time
 * this function is called.
 *
 * Non-zero offset means that the returned region will be offset relative to the start of a cache line, e.g.,
 * offset 60 could be used to ensure each load crosses a cache line.
 */
region& shuffled_region(const size_t size, const size_t offset = 0);

/**
 * Touch each cache line (or other specified stride) in region of size size.
 */
long touch_lines(void *region, size_t size, size_t stride = UB_CACHE_LINE_SIZE);


void flush_caches(size_t working_set = 16 * 1024 * 1024);

/**
 * Return the string description of the given system errno
 */
std::string errno_to_str(int e);

/**
 * This method always returns zero, but the optimizer doesn't know that.
 */
int always_zero();


#endif /* UTIL_HPP_ */
