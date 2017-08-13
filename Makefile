include config.mk

.PHONY: all clean libpfc libpfc-clean

GIT_VERSION := $(shell git describe --dirty --always)

HEADERS := $(wildcard *.h) $(wildcard *.hpp) $(wildcard *.hxx)

CPPFLAGS := -Wall -g -O0 -march=native -DGIT_VERSION=\"$(GIT_VERSION)\" -DUSE_LIBPFC=$(USE_LIBPFC)

SRC_FILES:=$(filter-out libpfc-timer.cpp, $(wildcard *.cpp))

ifeq ($(USE_LIBPFC),1)
LDFLAGS += -Llibpfc '-Wl,-rpath=$$ORIGIN/libpfc/'
LDLIBS += -lpfc
LIBPFC_DEP += libpfc
CLEAN_TARGETS += libpfc-clean
SRC_FILES += libpfc-timer.cpp
endif

OBJECTS := $(SRC_FILES:.cpp=.o) x86_methods.o

$(info USE_LIBPFC=${USE_LIBPFC})



all: uarch-bench

clean: $(CLEAN_TARGETS)
	rm -f *.o uarch-bench
	
uarch-bench: $(OBJECTS) $(LIBPFC_DEP)
	g++ $(OBJECTS) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) -o uarch-bench
	
%.o : %.cpp $(HEADERS) 
	g++ $(CPPFLAGS) -c -std=c++11 -o $@ $<

x86_methods.o: x86_methods.asm
	nasm -w+all -f elf64 -l x86_methods.list x86_methods.asm
	
libpfc:
	cd libpfc && make
	
libpfc-clean:		
	cd libpfc && make clean
	
insmod: libpfc
	sudo sh -c "echo 2 > /sys/bus/event_source/devices/cpu/rdpmc"
	! lsmod | grep -q pfc || sudo rmmod pfc
	sudo insmod libpfc/pfc.ko
	

