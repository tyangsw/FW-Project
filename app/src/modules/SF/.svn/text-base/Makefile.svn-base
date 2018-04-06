#
# SF2 module Makefile.
#
# Copyright (C) 2014-2015 North Atlantic Industries, Inc.
#

# Set MODULE_SRC
ifeq ($(strip $(MODULE_ID)),SF2)
    # SF2 is built from SC3 sources
    MODULE_SRC = SC/SC3
else
    MODULE_SRC = $(MODULE_TYPE)/$(MODULE_ID)
endif

# Include module specific Makefile
include $(MODULE_DIR)/$(MODULE_SRC)/Makefile

