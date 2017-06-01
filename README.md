**This project is a work-in-progress, and is in a very early state**

uarch-bench is a series of micro-benchmarks that try to stress certain microarchitectural features of modern CPUs.

At the moment it covers only x86, using assembly-level benchmarks, the suite also contain (in the future) C or C++ versions of 
many of the benchmarks, allowing them (in prinicple) to be run on other platforms. Of course, for any non-asm benchmark,
it is possible that the compiler makes a transformation that invalidates the intent of the benchmark. You could detect this this
is mostly noticable as a large difference between the C and asm scores.

## Building

Just run `make` in the project directory.

## Running

Just run `./uarch-bench` after building. The program currently accepts no arguments.
