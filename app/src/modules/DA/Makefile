#
# DAx module Makefile.
#
# Copyright (C) 2014-2015 North Atlantic Industries, Inc.
#

# Set MODULE_SRC
ifeq ($(strip $(MODULE_ID)),DA4)
    # DA4 is built from DA3 sources
    MODULE_SRC = $(MODULE_TYPE)/DA3
else
    MODULE_SRC = $(MODULE_TYPE)/$(MODULE_ID)
endif

# Include module specific Makefile
include $(MODULE_DIR)/$(MODULE_SRC)/Makefile

