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

#if USE_LIBPFC
#define IF_PFC(x) x
#else
#define IF_PFC(x)
#endif

/* use some reasonable default clock to return a point in time measured in nanos,
 * which has no relation to wall-clock time (is suitable for measuring intervals)
 */
static inline int64_t nanos() {
    auto t = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(t).time_since_epoch().count();
}

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

#endif /* UTIL_HPP_ */
