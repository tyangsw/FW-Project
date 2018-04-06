/*
 * Interrupt support definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

/*
 * Function prototypes
 */
XStatus init_interrupts(void);
void enable_interrupts(bool enable);
bool disable_interrupts(void);
XStatus irq_register(u32 int_id, Xil_InterruptHandler handler, void *context);

#endif /* __INTERRUPTS_H__ */

