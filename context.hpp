/*
 * context.h
 */

#ifndef CONTEXT_HPP_
#define CONTEXT_HPP_

#include <iosfwd>

#include "args.hxx"
#include "timer-info.hpp"
#include "util.hpp"

/* Context object couldn't be created but error information was already emitted */
class SilentFailure {};
/* Context object couldn't be created, but we treat it as a success (e.g., when --help is asked for) */
class SilentSuccess {};
/*
 * Holds the configuration and other context for a particular benchmark run, such as the output
 * location.
 */
class Context {
public:

    constexpr static unsigned int DEFAULT_PRECISION = 2;

    Context(int argc, char **argv, std::ostream *out);

    /* return the stream for error output */
    std::ostream& err() { return *err_; }

    /* return the stream for verbose informational output */
    std::ostream& log() { return *log_; }

    /* return the stream for benchmark result output */
    std::ostream& out() { return *out_; }

    /* return the argc origianlly passed to the process */
    int argc() { return argc_; }

    /* return the argv originally passed to the process (don't mutate this, ok?) */
    char** argv() { return argv_; }

    /* terminate the application with the given fatal error, treated as a printf-style format string */
    template<typename ... Args>
    void fatal(const std::string& str, Args ... args) {
        throw std::runtime_error(string_format(str, args ...));
    }

    /* return the default precision (number of decimal places) to use for metrics */
    unsigned int getPrecision() {
        return arg_default_precision.Get();
    }

    std::string getTimerName() {
        return arg_timer ? arg_timer.Get() : "clock";
    }

    /* return the TimerInfo for the timer associated with the current context, if any */
    TimerInfo& getTimerInfo() {
        return *timer_info_;
    }

    /* true if the user has selected verbose operation */
    bool verbose() { return verbose_; }

    /* execute the benchmark or other behavior */
    void run();

    /* get the TimerArgs for the current context */
    TimerArgs getTimerArgs();

private:
    std::ostream *err_, *log_, *out_;
    TimerInfo *timer_info_;
    int argc_;
    char **argv_;
    bool verbose_;


    args::ArgumentParser parser{"uarch-bench: A CPU micro-architecture benchmark"};
    args::HelpFlag help{parser, "help", "Display this help menu", {'h', "help"}};
    args::Flag arg_clockoverhead{parser, "clock-overhead", "Dislay clock overhead, then quit", {"clock-overhead"}};
    args::Flag arg_listbenches{parser, "list-benches", "Dislay the available benchmarks", {"list"}};
    args::Flag arg_listtimers{parser, "list-timers", "Dislay the available timers", {"list-timers"}};
    args::Flag arg_verbose{parser, "verbose", "Verbose output", {"verbose"}};
    args::ValueFlag<std::string> arg_timer{parser, "TIMER-NAME", "Use the specified timer", {"timer"}};
    args::ValueFlag<unsigned int> arg_default_precision{parser, "PRECISION", "Use the specified number of decimal places"
            " to report values from most benchmarks", {"precision"}, (unsigned int)DEFAULT_PRECISION};
    args::ValueFlag<std::string> arg_test_name{parser, "PATTERN", "Run only tests with name matching the given pattern", {"test-name"}};
    args::ValueFlag<std::string> arg_test_tag{parser, "PATTERN", "Run only the tests with a tag matching the given pattern", {"test-tag"}};
    args::Flag arg_listevents{parser, "list-events", "Display the extra available events associated with the timer", {"list-events"}};
    args::ValueFlag<std::string> arg_extraevents{parser, "extra-events", "A comma separated list of extra timer-specific events to track", {"extra-events"}};
    args::ValueFlag<int> arg_pincpu{parser, "pinned-cpu", "All tests will be pinned this CPU to (defaults to first available CPU)", {'c', "pinned-cpu"}, 0};


    // internal flags: these aren't displayed to the user via help, but are used by some wrapper script to interact with the
    // benchmark program, e.g., to dump info

    // dump the name of the selected timer, so that the wrapper script can consume it
    args::Flag arg_internal_dump_timer{parser, "internal-dump-timer", "", {"internal-dump-timer"}, args::Options::Hidden};

};



#endif /* CONTEXT_HPP_ */
