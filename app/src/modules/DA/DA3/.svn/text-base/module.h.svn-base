/*
 * DA3 module specific definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __MODULE_H__
#define __MODULE_H__

/*
 * Module memory layout
 */
#define MODULE_MEM_MAP_REV      1
#define SYS_REGS_BASE           MODULE_REGS_BASE
#define CH_REGS_BASE            (MODULE_REGS_BASE + 0x1000)
#define HPS_REGS_BASE           MODULE_PRIV_REGS_BASE
#define ID_REGS_BASE            (MODULE_PRIV_REGS_BASE + 0x9000000)

/*
 * Are parameter and/or calibration files required for this module?
 */
#define PARAM_FILE_REQUIRED     0
#define CAL_FILE_REQUIRED       1

/*
 * Is programmable CPLD 1 implemented on this module?
 * Should it be programmed late? (after module specific initialization)
 */
#define CPLD1_IMPLEMENTED       1
#define CPLD1_PROGRAM_LATE      1

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
typedef volatile struct {
    //@@@ define sys regs
} SYS_REGS;

/*
 * Channel registers
 */
typedef volatile struct {
    u32     ps_v_range;                     /* 0x000: Power Supply Voltage Range */
    u32     _reserved[63];                  /* 0x004 - 0x0FC: Reserved */
} CH_REGS;

/*
 * HPS side registers
 */
typedef volatile struct {
    u32     power_ok;                       /* 0x0000: 12V Power Good */
    u32     ps_dac_update;                  /* 0x0004: Update Power Supply DAC */
    u32     ps_en;                          /* 0x0008: Power Supply Enable */
    u32     _reserved[1021];                /* 0x000C - 0x0FFC: Reserved */
    u32     ps_dac_data[NUM_CHANNELS];      /* 0x1000: Power Supply DAC Data */
} HPS_REGS;

/*
 * Calibration data
 */
typedef volatile struct {
    //@@@ define cal data
} CAL_DATA;

/*
 * ID registers
 */
typedef volatile struct {
    u32     id;      /* 0x00: ID (1=DT1, 2=DT2, etc.) */
} ID_REGS;

#endif /* __MODULE_H__ */

