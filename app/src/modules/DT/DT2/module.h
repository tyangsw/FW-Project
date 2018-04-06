/*
 * DT2 module specific definitions.
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
    u32     switch_ctrl;                    /* 0x000: Switch Control */
    u32     read_io;                        /* 0x004: Read I/O */
    u32     oc_reset;                       /* 0x008: Reset Over-Current */
} SYS_REGS;

/*
 * Channel registers
 */
typedef volatile struct {
    u32     volt;                           /* 0x000: Voltage Reading (Sampled) */
    u32     volt_avg;                       /* 0x004: Voltage Reading (Averaged) */
    u32     current;                        /* 0x008: Current Reading (Sampled) */
    u32     current_avg;                    /* 0x00C: Current Reading (Averaged) */
    u32     debounce_time;                  /* 0x010: Debounce Time */
    u32     max_hi_thr;                     /* 0x014: Max High Threshold */
    u32     upper_thr;                      /* 0x018: Upper Threshold */
    u32     lower_thr;                      /* 0x01C: Lower Threshold */
    u32     min_lo_thr;                     /* 0x020: Min Low Threshold */
    u32     oc_value;                       /* 0x024: Over-Current Value */
    u32     _reserved1[22];                 /* 0x028 - 0x07C: Reserved */
} CH_REGS;

/*
 * HPS side registers
 */
typedef volatile struct {
    u32     ch_sel;                         /* 0x000: Channel Select */
    u32     ser_ctrl;                       /* 0x004: Serial Control */
    u32     ser_stat;                       /* 0x008: Serial Status */
    u32     tx_count;                       /* 0x00C: Tx Data Count */
    u32     ps_en;                          /* 0x010: Channel PS Enable */
    u32     baud;                           /* 0x014: Baud Rate */
} HPS_REGS;

/*
 * Calibration data
 */
#define OC_GAIN_CONST       0x10000000UL

typedef volatile struct {
    u32     volt_gain[NUM_CHANNELS];        /* 0x000: Voltage Cal Gain Factor */
    u32     _reserved1[16];                 /* 0x040 - 0x07C: Reserved */
    u32     volt_offset[NUM_CHANNELS];      /* 0x080: Voltage Cal Offset */
    u32     _reserved2[16];                 /* 0x0C0 - 0x0FC: Reserved */
    u32     current_gain[NUM_CHANNELS];     /* 0x100: Current Cal Gain Factor */
    u32     _reserved3[16];                 /* 0x140 - 0x17C: Reserved */
    u32     current_offset[NUM_CHANNELS];   /* 0x180: Current Cal Offset */
    u32     _reserved4[16];                 /* 0x1C0 - 0x1FC: Reserved */
    u32     oc_gain[NUM_CHANNELS];          /* 0x200: Over Current Cal Gain Factor */
} CAL_DATA;

/*
 * ID registers
 */
typedef volatile struct {
    u32     id;      /* 0x00: ID (1=DT1, 2=DT2, etc.) */
} ID_REGS;

#endif /* __MODULE_H__ */

