/*
 * DFx module specific definitions.
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
    NUM_CHANNELS
} CHANNEL;

/*
 * System registers
 */
typedef volatile struct {
    u32     read_io;                /* 0x000: Read I/O */
    u32     _reserved1;             /* 0x004: Reserved */
    u32     slew_rate;              /* 0x008: slew rate */
    u32     termination;            /* 0x00C: termination */
    u32     _reserved2;             /* 0x010: Reserved */
    u32     bit_clear;              /* 0x014: BIT Count Error Clr */
    u32     bit_inversion;          /* 0x018: Invert BIT Signal */
    u32     bit_err_irq_interval;   /* 0x01C: BIT error interrupt interval */
    u32     _reserved3;             /* 0x020: Reserved */
    u32     write_out;              /* 0x024: Write Outputs (DATAOUT register) */
    u32     _reserved4[4];          /* 0x028 - 0x034: Reserved */
    u32     trx_dir;                /* 0x038: Transceiver direction */
    u32     _reserved5[49];         /* 0x03C - 0x0FC: Reserved */
    u32     oc_reset;               /* 0x100: Overcurrent Reset */
#if MODULE_DF3
    u32     flush_q;                /* 0x104: Flush Queues */
    u32     _reserved6[65];         /* 0x108 - 0x208: Reserved */
    u32     tod_set;                /* 0x20C: Set TOD Course Counter */
    u32     tod;                    /* 0x210: Read TOD Course Counter */
    u32     subs;                   /* 0x214: Read SubSecond Counter */
    u32     subs_set;               /* 0x218: Set SubSecond Counter */
    u32     event[3];               /* 0x21C: Event Bits */
#endif
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
#if ! MODULE_DF3
typedef volatile struct {
    u32     id;      /* 0x00: ID (1=DF1, 2=DF2, etc.) */
} ID_REGS;
#endif

#endif /* __MODULE_H__ */

