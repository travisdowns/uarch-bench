/*
 * unit-test.cpp
 */

#include "catch.hpp"

#include "../util.hpp"

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

TEST_CASE( "string_format", "[util]" ) {
    REQUIRE( string_format("foo") == "foo" );
    REQUIRE( string_format("foo %d", 42) == "foo 42" );
    REQUIRE( string_format("%s %s", "foo", "bar") == "foo bar" );
}


