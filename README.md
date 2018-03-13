[![Build Status](https://travis-ci.org/travisdowns/uarch-bench.svg?branch=master)](https://travis-ci.org/travisdowns/uarch-bench)

# uarch-bench

A fine-grained micro-benchmark intended to investigate micro-architectural details of a target CPU, or to precisely benchmark small functions in a repeatable manner.

## Disclaimer

**This project is very much a work-in-progress, and is currently in a very early state with limited documentation and testing. Pull requests and issues welcome.**

## Purpose

The uarch-bench project is a collection of micro-benchmarks that try to stress certain microarchitectural features of modern CPUs and a framework for writing such benchmarks. Using [libpfc](https://github.com/obilaniu/libpfc) you can accurately track the value of Intel performance counters across the benchmarked region - often with precision of a single cycle.

At the moment it supports only x86, using mosty assembly and a few C++ benchmarks. In the future, I'd like to have more C or C++ benchmarks, allowing coverage (in prinicple) of more platforms (non-x86 assembly level benchmarks are also welcome). Of course, for any non-asm benchmark, it is possible that the compiler makes a transformation that invalidates the intent of the benchmark. You could detect this as a large difference between the C/C++ and asm scores.

Of course, these have all the pitfalls of any microbenchmark and are not really intended to be a simple measure of the overall 
performance of any CPU architecture. Rather they are mostly useful to:

1. Suss out changes between architectures. Often there are changes to particular microarchitectural feature that can be exposed 
via benchmarks of specific features. For example, you might be able to understand something about the behavior of the store buffer based on tests that excercise store-to-load forwarding. 
2. Understand low-level performance of various approaches to guide implemenation of highly-tuned algorithms. For the vast
majority of typical development tasks, the very low level information provided by these benches is essentially useless in
providing any guidance about performance. For some very specific tasks, such as highly-tuned C or C++ methods or hand-written
assembly, it might be useful to characterize the performance of, for example, the relative costs of aligned and unaligned accesses, or whatever.
3. Satisfy curiosity for those who care about this stuff and to collect the results from various architectures.
4. Provide a simple, standard way to quickly do one-off tests of some small assembly or C/C++ level idioms. Often the 
test itself is a few lines of code, but the cost is in all the infrastructure: implementing the timing code, converting measurements to cycles, removing outliers, running the tests for various parameters, reporting the results, whatever. This 
project aims to implement that infrastructure and make it easy to add your own tests (not complete!).


## Platform support

Currently only supports x86 Linux, but [Windows](https://github.com/travisdowns/uarch-bench/issues/1) should arrive at some point,
and one could even imagine a world with [OSX support](https://github.com/travisdowns/uarch-bench/issues/2).

## Prerequisites

On Intel platforms install `msr-tools` which is needed to read and write msrs to disable TurboBoost. On a Debian-like distribution that's usually accomplished with `sudo apt-get install msr-tools`.

## Building

Just run `make` in the project directory. If you want to modify any of the make settings, you can do it directly in `config.mk` or in a newly create local file `local.mk` (the latter having the advantage that this file is ignored by git so you won't have any merge conflicts on later pulls and won't automatically commit your local build settings).

## Running

Ideally, you run `./uarch-bench.sh` as root, since this allows the permissions needed to disable frequency scaling, as well as making it possible use `USE_LIBPFC=1` mode. If you don't have root or don't want to run a random project as root, you can also run it has non-root as `uarch-bench` (i.e., without the wrapper shell script), which will still work with some limitations. There is current [an open issue](https://github.com/travisdowns/uarch-bench/issues/31) for making non-root use a bit smoother.

### With Root

Just run `./uarch-bench.sh` after building. The script will generally invoke `sudo` to prompt you for root credentials in order to disable frequency scaling (either using the `no_turbo` flag if `intel_pstate` governor is used, or `rdmsr` and `wrmsr` otherwise).

### Without Root

You can also run the binary as `./uarch-bench` directly, which doesn't require sudo, but frequency scaling won't be automatically disabled in this case (you can still separately disable it prior to running `uarch-bench`).

### Command Line Arguments

Run `uarch-bench --help` to see a list and brief description of command line arguments.

### Frequency Scaling

One key to more reliable measurements (especially with the timing-based counters) is to ensure that there is no frequency scaling going on.

Generally this involves disabling turbo mode (to avoid scaling above nominal) and setting the power saving mode to performance (to avoid
scaling below nominal). The uarch-bench.sh script tries to do this, while restoring your previous setting after it completes.

