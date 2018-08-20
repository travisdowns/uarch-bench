/*
 * matchers.hpp
 */

#ifndef MATCHERS_HPP_
#define MATCHERS_HPP_

#include "util.hpp"
#include <string>
#include <vector>

class TagMatcher {
    std::vector<std::string> yes_patterns, no_patterns;
public:
    TagMatcher(std::string pattern_list) {
        // filter the list like "foo,~bar" into positive patterns (foo) and negative (bar)
        for (auto& p : split(pattern_list, ',')) {
            if (!p.empty() && p[0] == '~') {
                no_patterns.push_back(p.substr(1, p.size()));
            } else {
                yes_patterns.push_back(p);
            }
        }
    }

    bool operator()(const std::vector<std::string>& tags) const {
        bool pos_match = matches(tags, yes_patterns);
        bool neg_match = matches(tags, no_patterns);

        // note the yes_patterns.empty() condition: this means that in the special case there
        // are zero positive patterns we match _any_ tag that doesn't match the negative tags (it's as if you
        // had added a * to the tag list)
        return (pos_match || yes_patterns.empty()) && !neg_match;
    };

    bool static matches(const std::vector<std::string>& tags, const std::vector<std::string> &patterns) {
        for (auto& tag : tags) {
            for (auto& pattern : patterns) {
                if (wildcard_match(tag, pattern)) {
                    return true;
                }
            }
        }
        return false;
    }
};


#endif /* MATCHERS_HPP_ */
