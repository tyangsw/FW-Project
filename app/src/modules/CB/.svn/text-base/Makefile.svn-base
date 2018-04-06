#
# CBx module Makefile.
#
# Copyright (C) 2014 North Atlantic Industries, Inc.
#

# Version numbers
FW_VER_MAJOR := 1
FW_VER_MINOR := 3

# Additional module specific source files
ifeq ($(strip $(MODULE_ID)),CB1)
MODULE_C += \
    $(MODULE_DIR)/can.c
else ifeq ($(strip $(MODULE_ID)),CB2)
MODULE_C += \
    $(MODULE_DIR)/can.c \
    $(MODULE_DIR)/ssJ1939/j1939.c \
    $(MODULE_DIR)/ssJ1939/j1939tp.c
else ifeq ($(strip $(MODULE_ID)),CB3)
MODULE_C += \
    $(MODULE_DIR)/can.c \
    $(MODULE_DIR)/ssJ1939/j1939.c \
    $(MODULE_DIR)/ssJ1939/j1939tp.c    
endif

# Additional module specific header files
MODULE_H += \
    $(MODULE_DIR)/axi.h \
    $(MODULE_DIR)/can.h \
    $(MODULE_DIR)/bits.h \
    $(MODULE_DIR)/fpga_addresses.h

