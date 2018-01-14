include config.mk

# rebuild when makefile changes
-include dummy.rebuild

.PHONY: all clean libpfc libpfc-clean

ASM ?= nasm
ASM_FLAGS ?= -DNASM_ENABLE_DEBUG=$(NASM_DEBUG) -w+all -l x86_methods.list 

PFM_VER ?= 4.8.0
PFM_DIR ?= libpfm-$(PFM_VER)
PFM_LIBDIR ?= $(PFM_DIR)/lib

GIT_VERSION := $(shell git describe --dirty --always)

HEADERS := $(wildcard *.h) $(wildcard *.hpp) $(wildcard *.hxx)

O_LEVEL ?= -O2
CPPFLAGS := -Wall -g $(O_LEVEL) -march=native -DGIT_VERSION=\"$(GIT_VERSION)\" -DUSE_LIBPFC=$(USE_LIBPFC)

# files that should only be compiled if USE_LIBPFC is enabled
PFC_SRC := libpfc-timer.cpp libpfm4-support.cpp
SRC_FILES := $(wildcard *.cpp) nasm-utils/nasm-utils-helper.c
SRC_FILES := $(filter-out $(PFC_SRC), $(SRC_FILES))

ifeq ($(USE_LIBPFC),1)
LDFLAGS += -Llibpfc '-Wl,-rpath=$$ORIGIN/libpfc/' -L$(PFM_LIBDIR) '-Wl,-rpath=$$ORIGIN/$(PFM_LIBDIR)/'
LDLIBS += -lpfc -lpfm
LIBPFC_DEP += libpfc/libpfc.so $(PFM_LIBDIR)/libpfm.so
CLEAN_TARGETS += libpfc-clean libpfm4-clean
SRC_FILES += $(PFC_SRC)
endif

OBJECTS := $(SRC_FILES:.cpp=.o) x86_methods.o
OBJECTS := $(OBJECTS:.c=.o)
# $(info OBJECTS=$(OBJECTS))

$(info USE_LIBPFC=${USE_LIBPFC})

###########
# Targets #
###########

all: uarch-bench

clean:
	rm -f *.o uarch-bench
	
dist-clean: clean $(CLEAN_TARGETS)

uarch-bench: $(OBJECTS) $(LIBPFC_DEP)
	g++ $(OBJECTS) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) -o uarch-bench
# the next two lines are only to print out the size of the binary for diagnostic purposes, feel free to omit them 
	@wc -c uarch-bench | awk '{print "binary size: " $$1/1000 "KB"}'
	@size uarch-bench --format=SysV | egrep '\.text|\.eh_frame|\.rodata|^section'

%.o : %.cpp $(HEADERS) $(LIBPFC_DEP)
	g++ $(CPPFLAGS) -c -std=c++11 -o $@ $<

x86_methods.o: x86_methods.asm nasm-utils/nasm-utils-inc.asm
	$(ASM) $(ASM_FLAGS) -f elf64 x86_methods.asm

libpfc/libpfc.so:
	@echo "Buiding libpfc..."
	cd libpfc && make

libpfc-clean:
	cd libpfc && make clean

insmod: libpfc
	sudo sh -c "echo 2 > /sys/bus/event_source/devices/cpu/rdpmc"
	! lsmod | grep -q pfc || sudo rmmod pfc
	sudo insmod libpfc/pfc.ko

$(PFM_LIBDIR)/libpfm.so:
	tar xzf libpfm-4.8.0.tar.gz
	$(MAKE) -C $(PFM_DIR) lib

libpfm4-clean:
	rm -rf $(PFM_DIR)

	
LOCAL_MK = $(wildcard local.mk)
	
# https://stackoverflow.com/a/3892826/149138
dummy.rebuild: Makefile config.mk $(LOCAL_MK)
	touch $@
	$(MAKE) -s clean
	
foo: barzzz

