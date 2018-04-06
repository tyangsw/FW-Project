/*
 * RYx module specific definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __MODULE_H__
#define __MODULE_H__

/*
 * Module memory layout
 */
#define MODULE_MEM_MAP_REV  1
#define SYS_REGS_BASE       MODULE_REGS_BASE
#define ID_REGS_BASE        (MODULE_PRIV_REGS_BASE + 0x9000000)

/*
 * Are parameter and/or calibration files required for this module?
 */
#define PARAM_FILE_REQUIRED     0
#define CAL_FILE_REQUIRED       0

/*
 * Is programmable CPLD 1 implemented on this module?
 * Should it be programmed late? (after module specific initialization)
 */
#define CPLD1_IMPLEMENTED       0
#define CPLD1_PROGRAM_LATE      0

/*
 * Is programmable CPLD 2 implemented on this module?
 * Should it be programmed late? (after module specific initialization)
 */
#define CPLD2_IMPLEMENTED       0
#define CPLD2_PROGRAM_LATE      0

typedef enum
{
    CHANNEL_1 = 0,
    CHANNEL_2,
    CHANNEL_3,
    CHANNEL_4,
    NUM_CHANNELS
} CHANNEL;

/*
 * System registers
 */
#if MODULE_RY1
#define RELAY_TYPE      0   // Non-latching (RY1)
#else
#define RELAY_TYPE      1   // Latching (RY2)
#endif
typedef volatile struct {
    u32     select;             /* 0x00: Relay Select */
    u32     force_bit_fail;     /* 0x04: Force BIT fail */
    u32     type;               /* 0x08: Relay type */
    u32     _reserved[3];       /* 0x0C - 0x14: Reserved */
    u32     read_pos;           /* 0x18: Read Relay position */
} SYS_REGS;

/*
 * ID registers
 */
typedef volatile struct {
    u32     id;      /* 0x00: ID (1=RY1, 2=RY2, etc.) */
} ID_REGS;

#endif /* __MODULE_H__ */

