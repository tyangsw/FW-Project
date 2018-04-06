#
# SCx module Makefile.
#
# Copyright (C) 2014-2015 North Atlantic Industries, Inc.
#

# Set MODULE_SRC
ifeq ($(strip $(MODULE_ID)),SC7)
    # SC7 is built from SC1 sources
    MODULE_SRC = $(MODULE_TYPE)/SC1
else
    MODULE_SRC = $(MODULE_TYPE)/$(MODULE_ID)
endif

# Include module specific Makefile
include $(MODULE_DIR)/$(MODULE_SRC)/Makefile

