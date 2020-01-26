/*
 * unit-test.cpp
 */


#include "../util.hpp"
#include "../table.hpp"
#include "../matchers.hpp"
#include "../simple-timer.hpp"
#include "../perf-timer.hpp"

#include "catch.hpp"

#include <thread>


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


TEST_CASE( "split_on_string", "[util]") {
    using sv = std::vector<std::string>;

    CHECK(split_on_string("a,b,c", ",")  == sv{"a", "b", "c"});
    CHECK(split_on_string("a,b,c", ",")  == sv{"a", "b", "c"});
    CHECK(split_on_string("a,b,c,", ",") == sv{"a", "b", "c", ""});

    CHECK(split_on_string("xxayyybzzzz", "ab") == sv{"xxayyybzzzz"});
    CHECK(split_on_string("xxabyyyabzzzz", "ab") == sv{"xx", "yyy", "zzzz"});
}

TEST_CASE( "split_on_any", "[util]") {
    using sv = std::vector<std::string>;

    CHECK(split_on_any("a,b,c", ",")  == sv{"a", "b", "c"});
    CHECK(split_on_any("a,b,c", ",")  == sv{"a", "b", "c"});
    CHECK(split_on_any("a,b,c,", ",") == sv{"a", "b", "c", ""});

    CHECK(split_on_any("xxayyybzzzz", "ab") == sv{"xx", "yyy", "zzzz"});
}

TEST_CASE( "tag-matcher", "[matchers]" ) {
    {
        TagMatcher matcher("foo*");
        CHECK( matcher({"foo"})    == true );
        CHECK( matcher({"fooxxx"}) == true );
        CHECK( matcher({"fo"})     == false );
    }

    // pos only matches
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

    // pos + neg matches
    {
        TagMatcher matcher("foo,~bar,fob,~baz");

        CHECK( matcher({"foo"})               == true );
        CHECK( matcher({"bar"})               == false );
        CHECK( matcher({"foo", "bar"})        == false );
        CHECK( matcher({"foo", "fob"})        == true );
        CHECK( matcher({"foo", "baz", "fob"}) == false );

        CHECK( matcher({})                    == false );
        CHECK( matcher({"xxx", "barz"})       == false );
    }

    // neg only matches
    // the rules here are different: if there are zero positive tags in the pattern, we
    // accept any tag list that doesn't match any negative tag (normally we need to match
    // at least one positive tag as well)
    {
        TagMatcher matcher("~bar,~baz");

        CHECK( matcher({"foo"})               == true );
        CHECK( matcher({"bar"})               == false );
        CHECK( matcher({"foo", "bar"})        == false );
        CHECK( matcher({"foo", "fob"})        == true );
        CHECK( matcher({"foo", "baz", "fob"}) == false );

        CHECK( matcher({})                    == true );
        CHECK( matcher({"xxx", "barz"})       == true );
    }
}

TEST_CASE( "simple_timer", "[util]" ) {

    {
        SimpleTimer timer(false);
        CHECK(timer.elapsedNanos() == 0);
    }

    {
        SimpleTimer timer;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        CHECK(timer.elapsedNanos() >= 1000*1000);
    }

    {
        SimpleTimer timer(false);
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        timer.stop();
        int64_t elapsed = timer.elapsedNanos();
        CHECK(elapsed >= 1000*1000);
        // check that time doesn't increment when stopped
        CHECK(elapsed == timer.elapsedNanos());
    }

    {
        // double stop
        SimpleTimer timer;
        timer.stop();
        CHECK_THROWS_AS(timer.stop(), std::logic_error);
    }

    {
        // double start
        SimpleTimer timer;
        CHECK_THROWS_AS(timer.start(), std::logic_error);
    }

    {
        // double stop
        SimpleTimer timer;
        CHECK(timer.isStarted() == true);
        timer.stop();
        CHECK(timer.isStarted() == false);
    }

    {
        // elapsed with other durations
        SimpleTimer timer;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        timer.stop();
        CHECK(timer.elapsedNanos() >= 1000 * 1000);
        CHECK(timer.elapsedNanos()           == timer.elapsed<std::chrono::nanoseconds>());
        CHECK(timer.elapsedNanos() / 1000    == timer.elapsed<std::chrono::microseconds>());
        CHECK(timer.elapsedNanos() / 1000000 == timer.elapsed<std::chrono::milliseconds>());
    }

}

#if USE_PERF_TIMER

TEST_CASE( "parse_perf_events", "[perf]" ) {
    using sv = std::vector<NamedEvent>;
    CHECK(parsePerfEvents("foo,bar") == sv{{"foo"}, {"bar"}});
    CHECK(parsePerfEvents("foo/bar,baz/") == sv{{"foo/bar,baz/"}});
    CHECK(parsePerfEvents("foo/bar,baz/,beef") == sv{{"foo/bar,baz/"}, {"beef"}});
    CHECK(parsePerfEvents("foo/bar,baz/,beef,blah") == sv{{"foo/bar,baz/"}, {"beef"}, {"blah"}});

    CHECK(parsePerfEvents("foo#name") == sv{{"foo", "name"}});
    CHECK(parsePerfEvents("foo#name,bar#name2") == sv{{"foo", "name"}, {"bar", "name2"}});
    CHECK(parsePerfEvents("foo/bar,baz/#name,beef,blah") == sv{{"foo/bar,baz/", "name"}, {"beef"}, {"blah"}});
}

#endif
