**This project is a work-in-progress, and is currently in a very early state. Use is welcome, but don't expect miracles. **

The uarch-bench project is a collection of micro-benchmarks that try to stress certain microarchitectural features of modern CPUs.

At the moment it covers only x86, using assembly-level benchmarks, the suite also contain (in the future) C or C++ versions of 
many of the benchmarks, allowing them (in prinicple) to be run on other platforms. Of course, for any non-asm benchmark,
it is possible that the compiler makes a transformation that invalidates the intent of the benchmark. You could detect this this
is mostly noticable as a large difference between the C and asm scores.

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

Just run `make` in the project directory.

## Running

Just run `sudo ./uarch-bench.sh` after building. The `sudo` is required since `rdmsr` and `wrmsr` are used to query
and modify the turbo boost state on Intel platforms (turbo is disabled for the benchmark and restored to its original state after).

You can also run the binary as `./uarch-bench` directly, which doesn't require sudo, but doesn't disable frequency scaling.

The program currently accepts no arguments.

### Frequency Scaling

One key to more reliable measurements (especially with the timing-based counters) is to ensure that there is no frequency scaling going on.

Generally this involves disabling turbo mode (to avoid scaling above nominal) and setting the power saving mode to performance (to avoid
scaling below nominal). The uarch-bench.sh script tries to do this, while restoring your previous setting after it completes.

