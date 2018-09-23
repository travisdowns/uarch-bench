/*
 * main.cpp
 */

#include <iostream>
#include <iomanip>
#include <memory>
#include <chrono>
#include <cstddef>
#include <new>
#include <cassert>
#include <type_traits>
#include <sstream>
#include <functional>
#include <cstring>
#include <cstdlib>

#include <sys/mman.h>

#if USE_BACKWARD_CPP
#include "backward-cpp/backward.hpp"
#endif

#include "stats.hpp"
#include "version.hpp"
#include "args.hxx"
#include "benchmark.hpp"
#include "timer-info.hpp"
#include "context.hpp"
#include "util.hpp"
#include "timers.hpp"
#include "isa-support.hpp"
#include "matchers.hpp"

using namespace std;
using namespace std::chrono;
using namespace Stats;




#if USE_BACKWARD_CPP
backward::SignalHandling sh;
#endif

int main(int argc, char **argv) {
    cout << "Welcome to uarch-bench (" << GIT_VERSION << ")" << endl;
    cout << "Supported CPU features: " + support_string() << endl;

    try {
        Context context(argc, argv, &std::cout);
        context.run();
    } catch (SilentSuccess& e) {
    } catch (SilentFailure& e) {
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
