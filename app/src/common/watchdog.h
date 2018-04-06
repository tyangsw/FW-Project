/*
 * Watchdog support definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

/* All private timers and watchdog timers are always clocked at 1/2 of the CPU frequency (CPU_3x2x) */
#define WDT_LOAD_VALUE  (XPAR_CPU_CORTEXA9_CORE_CLOCK_FREQ_HZ)

/*
 * Function prototypes
 */
XStatus start_watchdog(void);
void stop_watchdog(void);
void watchdog_reset(void);

#endif /* __WATCHDOG_H__ */

