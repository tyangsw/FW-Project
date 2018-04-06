#
# ADx module Makefile.
#
# Copyright (C) 2014-2015 North Atlantic Industries, Inc.
#

# Version numbers
FW_VER_MAJOR := 4
FW_VER_MINOR := 2

# Rev 4.0 - 06/09/2015 - Updated to Top Board Rev F.  Also updated BIT code, ImodeNormalizer and current limit
# Rev 4.1 - 10/19/2015 - Fixing range normalization for current mode on the BIT channel
# Rev 4.2 - 04/04/2016 - Background Cal (D2) test had a leftover outter loop which was too small and could kick out before the proper number of samples.
#						 There BIT was also in the wrong range for Current mode.
#						 Also adding logic to ignore BIT if the A/Ds are lower than a certain threshold since we are having trouble detecting if the inputs are connected or not (they are reading some voltage).

# Additional module specific source files
MODULE_C += \
    $(MODULE_DIR)/module_BkgndCal.c

# Additional module specific header files
MODULE_H += \
    #$(MODULE_DIR)/module_lib.h

