/*
 * version.hpp
 */

#ifndef VERSION_HPP_
#define VERSION_HPP_

// normally this definition is passed on the compiler command line, but if not (e.g., someone is building this
// from an IDE or whatever) just use unknown
#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif

#endif /* VERSION_HPP_ */
