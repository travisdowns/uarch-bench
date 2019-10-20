Certain configurable aspects of the build are controlled by make variables.

You can get a non-exhaustive list of these variables and their defaults by looking at [config.mk](config.mk).

Although reasonable defaults are chosen, you may want to override them.

You may override a variable on a per-invocation basis, by providing a new value on the `make` command line. For example, to build the project in debug mode by overriding the `DEBUG` variable, one could invoke make as follows:

      make DEBUG=1 clean && make DEBUG=1
      
Note that there are two invocations of make here, one for the `clean` target and to rebuild the project (default target), both with `DEBUG=1`. This reflects the fact that, generally speaking, when you change any build variable, you need to *explicitly clean and rebuild* the project. That is, make knows when dependencies are out of date, but it doesn't know when you've changed build variables.

You can also also make some build variables more permanent by adding them to the file `local.mk` in the root project directory (this file doesn't exist by default, you have to create it). This file is included in the main `Makefile` if it exists and here you can set variables that will take effect on every build. Here's what I have in mine, for example:


```
ASM ?= nasm
DEBUG ?= 0
USE_BACKWARD_CPP ?= 1
BACKWARD_HAS_BFD ?= 1
CPU_ARCH ?= haswell

# use ccache for C++ compiles
CXX := ccache g++
CC := ccache gcc

# use gold linker
LDFLAGS = -fuse-ld=gold
```

Changes to `local.mk` trigger a rebuild, so you usually don't have to do a `clean` when you change a value there.



  

