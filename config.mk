-include local.mk

USE_LIBPFC = 1

# set DEBUG to 1 to enable various debugging checks
DEBUG := 0

ifeq ($(DEBUG),1)
O_LEVEL ?= -O0
NASM_DEBUG ?= 1
else
O_LEVEL ?= -O2
NASM_DEBUG ?= 0
endif
