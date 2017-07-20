/*
 * context.h
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <iostream>

/*
 * Holds the configuration and other context for a particular benchmark run, such as the output
 * location.
 */
class Context {
public:

    Context(std::ostream *out) : err_(out), log_(out), out_(out) {}

    /* return the stream for error output */
    std::ostream& err() { return *err_; }

    /* return the stream for informational output */
    std::ostream& log() { return *log_; }

    /* return the stream for benchmark result output */
    std::ostream& out() { return *out_; }

private:
    std::ostream *err_, *log_, *out_;
};



#endif /* CONTEXT_H_ */
