/*
 * util.hpp
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <chrono>

/* use some reasonable default clock to return a point in time measured in nanos,
 * which has no relation to wall-clock time (is suitable for measuring intervals)
 */
int64_t nanos() {
    auto t = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(t).time_since_epoch().count();
}



#endif /* UTIL_HPP_ */
