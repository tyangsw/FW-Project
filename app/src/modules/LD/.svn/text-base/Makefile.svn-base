#
# LDx/SDx module Makefile.
#
# Copyright (C) 2014-2015 North Atlantic Industries, Inc.
#

# Version numbers
FW_VER_MAJOR := 4
FW_VER_MINOR := 11

# Additional module specific source files
MODULE_C += \
    #$(MODULE_DIR)/module_lib.c

# Additional module specific header files
MODULE_H += \
    #$(MODULE_DIR)/module_lib.h



#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#						REVISIONS
#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	FW_VER_MAJOR		2	(2015-05-26)	
#		FW_VER_MINOR	1	(2015-05-26)
#			1. 	Changed code to stop false signal & reference loss errors when coming out of D0 or D3 modes
#		FW_VER_MINOR	2	(2015-05-27)
#			1. 	Changed velocity scale factor to reflect change (averaging 256 readings) in FPGA
#	FW_VER_MAJOR		3	(2015-06-10)	
#		FW_VER_MINOR	1
#			1. 	Changed addresses for Calibration registers. We now have 4 registers. (CalEnable, CalAddress, CalData & CalRdWrt)
#			2.	We now have to wrt a "0x41303037" to enable the cal registers.
#			3.	Added code in the menu commands to get the background bit ad values. (was troubleshooting 180 degree out of phase issue)
#			4.	Added bandwidth values, although very crude and needs to be revisted. Allowed auto bandwidth.
#			5.	Added D2 Verify value when performing background BIT (0x0055)
#			6.	Changed addresses for D0,D2,D3 & BitAngle to conform to ModuleCommonMemory
#
#		FW_VER_MINOR	2
#			1. 	Added register access for background dc cal values at 0x3f30 - 0x3f4C
#
#		FW_VER_MINOR	3
#			1. 	Added arrays for Filter Coef & Filter Gain
#
#		FW_VER_MINOR	4
#			1. 	Playing around BW numbers, not complete yet.
#
#		FW_VER_MINOR	5
#			1. 	Fixed possible divide by zero in DC calculation by not performing divide if totalCycleCount equaled zero. Fixed serdes dropout problem.
#
#		FW_VER_MINOR	6
#			1. 	Changed FREQ scale to accomodate faster clock (62.5Mhz ->125Mhz)
#
#	FW_VER_MAJOR		4	(2015-10-30)	
#		FW_VER_MINOR	3
#			1. 	Changed signal sumOfsquares low limit value in Calculate_RMSValues(void) function from 1.8e+11 to 2.00e+10. It was cutting out around below 5V signal
#				between 5 & 7 Khz as noticed by Hayden D. Need to check this out over frequency and voltages.
#
#		FW_VER_MINOR	4
#			1. 	Changed code to be able to detect when coming out of Manual Bandwidth mode to Auto Bandwidth in order to update the correct Bandwidth.
#				Basically, I set the PrevFrequency variable to 0hz in ManualBandwidth mode.
#
#		FW_VER_MINOR	5	(2016-06-28)
#			1. 	Changed code to be able to detect when a BIT angle has changed to make sure we wait before checking. This was to stop false BIT errors
#				when in D0 mode
#
#		FW_VER_MINOR	6	(2016-07-12)
#			1. 	Changed code to be able to detect when a BIT angle has changed to make sure we wait before checking. This was to stop false BIT errors
#				when in D0 mode. (Different approach than in minor rev 5)
#
#		FW_VER_MINOR	7	(2016-07-20)
#			1. 	Changed Integrator,Proportional,Filter files to use 2Hz values with 0Hz index.
#
#		FW_VER_MINOR	8	(2016-07-27)
#			1. 	Changed BIT Bandwidth from 100Hz to 500Hz to help flase bit errors with large angle change
#
#		FW_VER_MINOR	9	(2016-08-01)
#			1. 	Changed algorithm in how we detect BIT errors. Also mad error limit 0.1 (was 0.05)
#
#		FW_VER_MINOR	10	(2016-08-02)
#			1. 	Changed velocity scale. Was reading 68056 when expected was 68000
#
#		FW_VER_MINOR	11	(2016-08-03)
#			1. 	Changed aaccumulating BIT Angle from float to double to get rid of false errors. Also chnaged MaxErr back to 0.05 degrees
#
#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
