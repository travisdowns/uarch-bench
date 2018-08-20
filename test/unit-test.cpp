/*
 * unit-test.cpp
 */


#include "../util.hpp"
#include "../table.hpp"
#include "../matchers.hpp"

#include "catch.hpp"


TEST_CASE( "string_format", "[util]" ) {
    REQUIRE( string_format("foo") == "foo" );
    REQUIRE( string_format("foo %d", 42) == "foo 42" );
    REQUIRE( string_format("%s %s", "foo", "bar") == "foo bar" );
}

TEST_CASE( "table", "[util]" ) {
    using namespace table;

    Table t;

    t = {};
    REQUIRE( t.str() == "" );

    t = {};
    t.newRow().add("a").add("b");
    REQUIRE( t.str() == "a b\n" );

    t = {};
    t.newRow().add("a").add("b");
    t.newRow().add("aa").add("bb");
    REQUIRE( t.str() ==
            "a  b \n"
            "aa bb\n"
    );

    t = {};
    t.newRow().add("a");
    t.newRow().add("a").add("b");
    t.newRow().add("a").add("b").add("c");
    REQUIRE( t.str() ==
            "a\n"
            "a b\n"
            "a b c\n"
    );


    t = {};
    t.newRow().add("a").add("b");
    t.newRow().add("xxx").add("xxx");
    REQUIRE( t.str() ==
            "a   b  \n"
            "xxx xxx\n"
    );
    t.colInfo(0).justify = ColInfo::RIGHT;
    REQUIRE( t.str() ==
            "  a b  \n"
            "xxx xxx\n"
    );
    t.colInfo(1).justify = ColInfo::RIGHT;
    REQUIRE( t.str() ==
            "  a   b\n"
            "xxx xxx\n"
    );
}


TEST_CASE( "split", "[util]") {
    using sv = std::vector<std::string>;

    CHECK(split("a,b,c", ",")  == sv{"a", "b", "c"});
    CHECK(split("a,b,c", ',')  == sv{"a", "b", "c"});
    CHECK(split("a,b,c,", ',') == sv{"a", "b", "c", ""});
}

TEST_CASE( "tag-matcher", "[matchers]" ) {
    {
        TagMatcher matcher("foo*");
        CHECK( matcher({"foo"})    == true );
        CHECK( matcher({"fooxxx"}) == true );
        CHECK( matcher({"fo"})     == false );
    }

    {
        TagMatcher matcher("foo,bar");

        CHECK( matcher({"foo"})               == true );
        CHECK( matcher({"bar"})               == true );
        CHECK( matcher({"xxx", "foo"})        == true );
        CHECK( matcher({"xxx", "bar"})        == true );
        CHECK( matcher({"xxx", "bar", "zzz"}) == true );

        CHECK( matcher({})                    == false );
        CHECK( matcher({"xxx", "barz"})       == false );
    }

}



