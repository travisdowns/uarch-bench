/*
 * util.cpp
 */

#include <regex>

std::string escape_for_regex(const std::string& input) {
    // see https://stackoverflow.com/a/40195721/149138
    static std::regex chars_to_escape{ R"([-[\]{}()*+?.\^$|])" };
    return std::regex_replace( input, chars_to_escape, R"(\$&)" );
}

bool wildcard_match(const std::string& target, const std::string& pattern) {
    // we implement wildcards just by replacing all *s with .*
    std::string escaped = escape_for_regex(std::move(pattern));
    std::string regex   = std::regex_replace(escaped, std::regex(R"(\\*)"), R"(.*)");
    return std::regex_match(target, std::regex(regex));
}
