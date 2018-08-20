/*
 * matchers.hpp
 */

#ifndef MATCHERS_HPP_
#define MATCHERS_HPP_

#include "util.hpp"
#include <string>
#include <vector>

class TagMatcher {
    std::vector<std::string> patterns;
public:
    TagMatcher(std::string pattern) : patterns{split(pattern, ',')} {}

    bool operator()(const std::vector<std::string>& tags) const {
        for (auto& tag : tags) {
            for (auto& pattern : patterns) {
                if (wildcard_match(tag, pattern)) {
                    return true;
                }
            }
        }
        return false;
    };
};


#endif /* MATCHERS_HPP_ */
