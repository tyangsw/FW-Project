/*
 * ARx module specific definitions.
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
#define CH_REGS_BASE        (MODULE_REGS_BASE + 0x100)

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
    NUM_CHANNELS
} CHANNEL;


/*
 * System registers
 */
typedef volatile struct {
    u32     tx_trigger;         /* 0x000: Tx Trigger */
    u32     tx_pause;           /* 0x004: Tx Pause */
    u32     tx_stop;            /* 0x008: Tx Stop */
    u32     ts_control;         /* 0x00C: Timestamp Control */
    u32     ts;                 /* 0x010: Timestamp */
    u32     reset;              /* 0x014: Module Reset */
    u32     rx_data;            /* 0x018: Rx Data Unbuffered  */
} SYS_REGS;

/*
 * Channel registers
 */
typedef volatile struct {
    u32     tx_buf;             /* 0x000: Tx Buffer */
    u32     rx_buf;             /* 0x004: Rx Buffer */
    u32     rx_fifo_thr;        /* 0x008: Rx FIFO Threshold */
    u32     tx_fifo_thr;        /* 0x00C: Tx FIFO Threshold */
    u32     rx_fifo_level;      /* 0x010: Rx FIFO Level */
    u32     tx_fifo_level;      /* 0x014: Tx FIFO Level */
    u32     control;            /* 0x018: Channel Control */
    u32     tx_fifo_rate;       /* 0x01C: Channel Tx FIFO Rate */
    u32     async_tx_data;      /* 0x020: Async Tx Data */
    u32     _reserved[55];      /* 0x024 - 0x0FC: Reserved */
} CH_REGS;

#endif /* __MODULE_H__ */

