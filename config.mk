-include local.mk

# Whether to compile support for using --timer=libpfc, which is a kernel module
# and associated userland code that allows use of rdpmc instruction to sample
# the Intel performance counters directly, leading to very precise measurements.
# Even when this is enabled, the default std::chrono based timing is still available.
USE_LIBPFC ?= 1

# Whether to compile support for using backwards-cpp, which gives stack traces
# on crashes. By default, only binaries and addresses are given in the backtrace, but
# for better stack traces you can enable either of the BACKWARD_HAS options below.
# See https://github.com/bombela/backward-cpp for more details. 
USE_BACKWARD_CPP ?= 1

# Only has an effect if USE_BACKWARD_CPP == 1, set this to 1 to use gnu binutils
# backtracing. Probably you can install it with (or similar for non-apt platforms):
# apt-get install binutils-dev
BACKWARD_HAS_BFD ?= 0

# Only has an effect if USE_BACKWARD_CPP == 1, set this to 1 to use elfutils
# backtracing. Probably you can install it with (or similar for non-apt platforms):
# apt-get install libdw-dev
BACKWARD_HAS_DW ?= 0

# set DEBUG to 1 to enable various debugging checks
DEBUG ?= 0

# The assembler to use. Defaults to nasm, but can also be set to yasm which has better
# debug info handling.
ASM ?= nasm

ifeq ($(DEBUG),1)
O_LEVEL ?= -O0
NASM_DEBUG ?= 1
else
O_LEVEL ?= -O2
NASM_DEBUG ?= 0
endif

ifdef CC_OVERRIDE
CC := $(CC_OVERRIDE)
$(info Overiding CC to $(CC))
endif 

ifdef CXX_OVERRIDE
CXX := $(CXX_OVERRIDE)
$(info Overiding CXX to $(CXX))
endif 
