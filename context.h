/*
 * context.h
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <iostream>

#include "args.hxx"
#include "timer-info.hpp"

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

    Context(int argc, char* const * argv, std::ostream *out) : err_(out), log_(out), out_(out) {
        try {
            parser.ParseCLI(argc, argv);
        } catch (args::Help&) {
            log() << parser;
            throw SilentSuccess();
        } catch (args::ParseError& e) {
            err() << "ERROR: " << e.what() << std::endl << parser;
            throw SilentFailure();
        } catch (args::UsageError & e) {
            err() << "ERROR: " << e.what() << std::endl;
            throw SilentFailure();
        }
    }

    /* return the stream for error output */
    std::ostream& err() { return *err_; }

    /* return the stream for informational output */
    std::ostream& log() { return *log_; }

    /* return the stream for benchmark result output */
    std::ostream& out() { return *out_; }

    /* return the default precision (number of decimal places) to use for metrics */
    unsigned int getPrecision() {
        return arg_default_precision.Get();
    }

    /* return the TimerInfo for the timer associated with the current context, if any */
    TimerInfo& getTimerInfo() {
        return *timer_info_;
    }

    int run();
    void listTimers();
    void listBenches();

private:
    std::ostream *err_, *log_, *out_;
    TimerInfo *timer_info_;

    args::ArgumentParser parser{"uarch-bench: A CPU micro-architecture benchmark"};
    args::HelpFlag help{parser, "help", "Display this help menu", {'h', "help"}};
    args::Flag arg_clockoverhead{parser, "clock-overhead", "Dislay clock overhead, then quit", {"clock-overhead"}};
    args::Flag arg_listbenches{parser, "list-benches", "Dislay the available benchmarks", {"list"}};
    args::Flag arg_listtimers{parser, "list-timers", "Dislay the available timers", {"list-timers"}};
    args::ValueFlag<std::string> arg_timer{parser, "timer", "Use the specified timer", {"timer"}};
    args::ValueFlag<unsigned int> arg_default_precision{parser, "precision", "Use the specified number of decimal places"
            " to report values from most benchmarks", {"precision"}, (unsigned int)DEFAULT_PRECISION};
};



#endif /* CONTEXT_H_ */
