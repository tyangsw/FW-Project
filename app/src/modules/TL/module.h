/*
 * TLx module specific definitions.
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
#define CH_REGS_BASE            (MODULE_REGS_BASE + 0x1080)
#define PATTERN_GENERATOR_RAM   (MODULE_REGS_BASE + 0x2000)
#define ID_REGS_BASE            (MODULE_PRIV_REGS_BASE + 0x9000000)

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
    CHANNEL_5,
    CHANNEL_6,
    CHANNEL_7,
    CHANNEL_8,
    CHANNEL_9,
    CHANNEL_10,
    CHANNEL_11,
    CHANNEL_12,
    CHANNEL_13,
    CHANNEL_14,
    CHANNEL_15,
    CHANNEL_16,
    CHANNEL_17,
    CHANNEL_18,
    CHANNEL_19,
    CHANNEL_20,
    CHANNEL_21,
    CHANNEL_22,
    CHANNEL_23,
    CHANNEL_24,
    NUM_CHANNELS
} CHANNEL;

/*
 * System registers
 */
typedef volatile struct {
    u32     read_io;                /* 0x000: Read I/O */
    u32     _reserved1[4];          /* 0x004 - 0x010: Reserved */
    u32     bit_clear;              /* 0x014: BIT Count Error Clr */
    u32     bit_inversion;          /* 0x018: Invert BIT Signal */
    u32     bit_err_irq_interval;   /* 0x01C: BIT error interrupt interval */
    u32     vcc_sel;                /* 0x020: Vcc select */
    u32     write_out;              /* 0x024: Write Outputs (DATAOUT register) */
    u32     _reserved2[4];          /* 0x028 - 0x034: Reserved */
    u32     trx_dir;                /* 0x038: Transceiver direction */
    u32     _reserved3[49];         /* 0x03C - 0x0FC: Reserved */
    u32     oc_reset;               /* 0x100: Overcurrent Reset */
} SYS_REGS;

/*
 * Channel registers
 */
typedef volatile struct {
    u32     _reserved1[3];          /* 0x000 - 0x008: Reserved */
    u32     period_debounce;        /* 0x00C: Period (output modes 11,12) or Debounce(any input mode) */
    u32     _reserved2[28];         /* 0x010 - 0x07C: Reserved */
} CH_REGS;

/*
 * ID registers
 */
typedef volatile struct {
    u32     id;      /* 0x00: ID (1=TL1, 2=TL2, etc.) */
} ID_REGS;

#endif /* __MODULE_H__ */

