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

#define UB_CACHE_LINE_SIZE    64

#if USE_LIBPFC
#define IF_PFC(x) x
#else
#define IF_PFC(x)
#endif

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

void *new_huge_ptr(size_t size);
void *aligned_ptr(size_t base_alignment, size_t required_size);
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



/*
 * Split a string delimited by sep.
 *
 * See https://stackoverflow.com/a/7408245/149138
 */
static inline std::vector<std::string> split(const std::string &text, const std::string &sep) {
  std::vector<std::string> tokens;
  std::size_t start = 0, end = 0;
  while ((end = text.find(sep, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = end + sep.length();
  }
  tokens.push_back(text.substr(start));
  return tokens;
}


static inline std::vector<std::string> split(const std::string &text, char sep) {
    return split(text, std::string{sep});
}

/** Take a string and escape it so that it will be treated as a literal string in a regex */
std::string escape_for_regex(const std::string& input);

/**
 * Returns true if the entire string target matches pattern, where pattern can contain * wildcards
 * that match any number of characters.
 */
bool wildcard_match(const std::string& target, const std::string& pattern);
#endif /* UTIL_HPP_ */
