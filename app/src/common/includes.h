/*
 * Master include file.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

/* Standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <inttypes.h>

/* Xilinx headers */
#include <xil_types.h>
#include <xil_io.h>
#include <xil_exception.h>
#include <xil_misc_psreset_api.h>
#include <xil_mmu.h>
#include <xil_cache.h>
#include <xparameters.h>
#include <xscugic.h>
#include <xstatus.h>
#include <xuartps.h>
#include <sleep.h>
#include <xtime_l.h>
#include <xscuwdt.h>
#include <xiicps.h>
#include <xadcps.h>
#include <xqspips.h>
#include <xl2cc.h>
#include <xl2cc_counter.h>

/* Local headers */
#include <common.h>
#include <i2c.h>
#include <interrupts.h>
#include <qspi.h>
#include <version.h>
#include <volt_temp.h>
#include <watchdog.h>
#include <module.h>

#if SERIAL_DEBUG
#include <menu.h>
#endif

