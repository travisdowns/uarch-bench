include config.mk

# rebuild when makefile changes
-include dummy.rebuild

.PHONY: all clean libpfc libpfc-clean

CXX ?= g++
CC ?= gcc
ASM ?= nasm
ASM_FLAGS ?= -DNASM_ENABLE_DEBUG=$(NASM_DEBUG) -w+all

# make submakes use the specified compiler also
export CXX
export CC


ifeq ($(PORTABLE),1)
$(info uarch-bench is being compiled in PORTABLE=1 mode, most x86-specific tests disabled)
endif

# The target to build when building libpfc (if we build it at all). By default, we are going
# to build the userspace shared lib and the kernel module, but TravisCI, for example, would
# (usually) like to skip building the kernel module. Note that we don't build the demo, see #issue 71
LIBPFC_TARGET ?= libpfc.so pfc.ko

PFM_DIR ?= libpfm4
PFM_LIBDIR ?= $(PFM_DIR)/lib

JEVENTS_DIR ?= pmu-tools/jevents
JEVENTS_LIB ?= $(JEVENTS_DIR)/libjevents.a

PSNIP_DIR ?= portable-snippets
# all the psnip source files we want to compile into uarch-bench
PSNIP_SRC := cpu.c

BOOST_DIR := boost_1_70_0

GIT_VERSION := $(shell git describe --dirty --always)

ifneq ($(CPU_ARCH),)
ARCH_FLAGS := -march=$(CPU_ARCH)
endif
O_LEVEL ?= -O2

COMMON_FLAGS := -MMD -Wall $(ARCH_FLAGS) -g $(O_LEVEL) -DGIT_VERSION=\"$(GIT_VERSION)\" \
    -DUSE_LIBPFC=$(USE_LIBPFC) -DUSE_BACKWARD_CPP=$(USE_BACKWARD_CPP) -DBACKWARD_HAS_BFD=$(BACKWARD_HAS_BFD) \
    -DBACKWARD_HAS_DW=$(BACKWARD_HAS_DW) -DUSE_PERF_TIMER=$(USE_PERF_TIMER) -I$(PSNIP_DIR) -I$(BOOST_DIR) \
    -DUARCH_BENCH_PORTABLE=$(if $(PORTABLE),1,0)
CPPFLAGS := $(COMMON_FLAGS)
CFLAGS := $(COMMON_FLAGS)

# files that should only be compiled if USE_LIBPFC is enabled
PFC_SRC := libpfc-timer.cpp libpfm4-support.cpp
SRC_FILES := $(wildcard *.cpp) $(wildcard *.c) nasm-utils/nasm-utils-helper.c $(PSNIP_SRC)
SRC_FILES := $(filter-out $(PFC_SRC), $(SRC_FILES))
ASM_FILES := $(if $(PORTABLE),,$(wildcard *.asm))

# on most compilers we should use no-pie since the nasm stuff isn't position independent
# but since old compilers don't support it, you can override it with PIE= on the command line
PIE ?= -no-pie
LDFLAGS += $(PIE)

EXTRA_DEPS :=

ifeq ($(USE_LIBPFC),1)
LDFLAGS += -Llibpfc '-Wl,-rpath=$$ORIGIN/libpfc/' -L$(PFM_LIBDIR) '-Wl,-rpath=$$ORIGIN/$(PFM_LIBDIR)/'
LDLIBS += -lpfc -lpfm
EXTRA_DEPS += libpfc/libpfc.so libpfc/pfc.ko $(PFM_LIBDIR)/libpfm.so
CLEAN_TARGETS += libpfc-clean libpfm4-clean
SRC_FILES += $(PFC_SRC)
endif

ifeq ($(USE_PERF_TIMER),1)
EXTRA_DEPS += $(JEVENTS_LIB) download_events.touch
LDLIBS += $(JEVENTS_LIB)
SEQTEST := util/seqtest
endif

ifeq ($(BACKWARD_HAS_BFD),1)
LDFLAGS += -lbfd -ldl
endif

ifeq ($(BACKWARD_HAS_DW),1)
LDFLAGS += -ldw
endif

OBJECTS := $(SRC_FILES:.cpp=.o) $(ASM_FILES:.asm=.o)
OBJECTS := $(OBJECTS:.c=.o)
DEPFILES = $(OBJECTS:.o=.d)
# $(info OBJECTS=$(OBJECTS))

$(info USE_LIBPFC=${USE_LIBPFC} USE_PERF_TIMER=${USE_PERF_TIMER})

VPATH = test:$(PSNIP_DIR)/cpu

###########
# Targets #
###########

all: uarch-bench unit-test $(SEQTEST)

-include $(DEPFILES) unit-test.d

clean:	libpfc-clean
	rm -f *.d *.o uarch-bench

dist-clean: clean $(CLEAN_TARGETS)

# $(filter-out $(OBJECTS), main.o)

# link all object files except main.o into unit-test
UNIT_OBJECTS := $(filter-out main.o, $(OBJECTS))
unit-test: unit-test.o unit-test-main.o $(UNIT_OBJECTS)
	$(CXX) $^         $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) -std=c++11 -o $@

uarch-bench: $(OBJECTS) $(EXTRA_DEPS)
	$(CXX) $(OBJECTS) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) -std=c++11 -o $@
# the next two lines are only to print out the size of the binary for diagnostic purposes, feel free to omit them
	@wc -c uarch-bench | awk '{print "binary size: " $$1/1000 "KB"}'
	@size uarch-bench --format=SysV | egrep '\.text|\.eh_frame|\.rodata|\.debug_info|\.debug_loc|^section'

util/seqtest: util/seqtest.o

%.o : %.c
	$(CC) $(CFLAGS) -c -std=c11 -o $@ $<

%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -std=c++11 -o $@ $<

%.o: %.asm nasm-utils/nasm-utils-inc.asm
	$(ASM) $(ASM_FLAGS) ${NASM_DEFINES} -f elf64 $<

# fake dependency, but prevents make from trying to build libpfc twice in parallel if both the ko and so are missing
libpfc/pfc.ko: libpfc/libpfc.so

libpfc/libpfc.so libpfc/pfc.ko:
	@echo "Buiding libpfc targets: $(LIBPFC_TARGET)"
	$(MAKE) -C libpfc $(LIBPFC_TARGET)

libpfc-clean:
	$(MAKE) -C libpfc clean

ifeq ($(USE_PERF_TIMER),1)
# jevents handling - we just assume jevents depends on all .c or .h files in the root directory
# which doesn't catch stuff in the examples dir (for example), but that's probably fine
$(JEVENTS_LIB): $(JEVENTS_DIR)/*.[ch]
	$(MAKE) -C $(JEVENTS_DIR)

# this file just tracks whether you've downloaded the events file for this host
download_events.touch:
	pmu-tools/event_download.py
	echo "This presence of this file is used to indicate to make that the PMU event json files have been downloaded" >> $@

endif

insmod: libpfc
	sudo sh -c "echo 2 > /sys/bus/event_source/devices/cpu/rdpmc"
	! lsmod | grep -q pfc || sudo rmmod pfc
	sudo insmod libpfc/pfc.ko

$(PFM_LIBDIR)/libpfm.so:
	$(MAKE) -C $(PFM_DIR) lib

libpfm4-clean:
	$(MAKE) -C $(PFM_DIR) clean

LOCAL_MK = $(wildcard local.mk)

# https://stackoverflow.com/a/3892826/149138
dummy.rebuild: Makefile config.mk $(LOCAL_MK)
	touch $@
	$(MAKE) -s clean
