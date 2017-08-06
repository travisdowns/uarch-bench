include config.mk

.PHONY: all clean libpfc libpfc-clean

GIT_VERSION := $(shell git describe --dirty --always)

HEADERS := $(wildcard *.h) $(wildcard *.hpp) $(wildcard *.hxx)

CPPFLAGS := -Wall -g -O0 -march=native -DGIT_VERSION=\"$(GIT_VERSION)\" -DUSE_LIBPFC=$(USE_LIBPFC)

OBJECTS := main.o x86_methods.o

ifeq ($(USE_LIBPFC),1)
LDFLAGS += -Llibpfc '-Wl,-rpath=$$ORIGIN/libpfc/'
LDLIBS += -lpfc
LIBPFC_DEP += libpfc
CLEAN_TARGETS += libpfc-clean
OBJECTS += libpfc-timer.o
endif

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

