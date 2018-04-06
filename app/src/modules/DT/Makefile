#
# DTx module Makefile.
#
# Copyright (C) 2014-2015 North Atlantic Industries, Inc.
#

# Set MODULE_SRC
ifeq ($(strip $(MODULE_ID)),DT4)
    # DT4 is built from DT1 sources
    MODULE_SRC = $(MODULE_TYPE)/DT1
else ifeq ($(strip $(MODULE_ID)),DT5)
    # DT5 is built from DT2 sources
    MODULE_SRC = $(MODULE_TYPE)/DT2
else
    MODULE_SRC = $(MODULE_TYPE)/$(MODULE_ID)
endif

# Include module specific Makefile
include $(MODULE_DIR)/$(MODULE_SRC)/Makefile

