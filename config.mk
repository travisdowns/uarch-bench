-include local.mk

# OS detection
OS ?= $(shell uname)
ifneq ($(OS),Linux)
$(error non-Linux not supported yet)
endif

OS_ARCH := $(shell uname -m)
ifneq ($(OS_ARCH),x86_64)
PORTABLE ?= 1
endif

# Whether to compile support for using --timer=libpfc, which is a kernel module
# and associated userland code that allows use of rdpmc instruction to sample
# the Intel performance counters directly, leading to very precise measurements.
# Even when this is enabled, the default std::chrono based timing is still available.
USE_LIBPFC ?= $(if $(PORTABLE),0,1)

# Whether to compile support for using --timer=perf which is a timer that uses the perf
# subsystem through Andi Kleen's jevent events library. It uses uses the Intel 01.org
# downloaded event csvs to support use of perf_event_open and friends
USE_PERF_TIMER ?= $(if $(PORTABLE),0,1)

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

ifdef ASM
$(info using specified nasm: $(nasm))
else
# The assembler to use, we used to support yasm also but as of issue #63 we only support nasm
ifeq (, $(shell which nasm))
$(info using local nasm)
ASM ?= ./nasm-binaries/linux/nasm-2.13.03-0.fc24
else
$(info using system nasm)
ASM ?= nasm
endif
endif

ifeq ($(DEBUG),1)
O_LEVEL ?= -O0
NASM_DEBUG ?= 1
else
O_LEVEL ?= -O2
NASM_DEBUG ?= 0
endif
