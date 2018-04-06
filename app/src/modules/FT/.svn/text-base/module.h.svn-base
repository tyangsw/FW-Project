/*
 * FTx module specific definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __MODULE_H__
#define __MODULE_H__

/*
 * Module memory layout
 */
#define MODULE_MEM_MAP_REV      1
#define CH_REGS_BASE            MODULE_REGS_BASE
#define CH_MEM_BASE             (MODULE_REGS_BASE + 0x1F000)
#define CH_STRIDE               0x40000
#define CHx_REGS_BASE(ch)       (CH_REGS_BASE + (ch * CH_STRIDE))
#define CHx_MEM_BASE(ch)        (CH_MEM_BASE + (ch * CH_STRIDE))
#define HPS_REGS_BASE           MODULE_PRIV_REGS_BASE
#define CHx_HPS_REGS_BASE(ch)   (HPS_REGS_BASE + (ch * sizeof(HPS_REGS)))
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
    NUM_CHANNELS
} CHANNEL;

/*
 * HOST side registers
 */
#define AUX_MISC_RT_ADR_LAT_BIT         (1UL << 3)
#define AUX_MISC_RTAD_SW_EN_BIT         (1UL << 4)

#define AUX_CTRL_ASSIST_MODE_EN_BIT     (1UL << 0)
#define AUX_CTRL_CLR_RX_FIFO_BIT        (1UL << 1)
#define AUX_CTRL_CLR_TX_FIFO_BIT        (1UL << 2)
#define AUX_CTRL_UNLOAD_RX_FIFO_BIT     (1UL << 3)
#define AUX_CTRL_LOAD_TX_FIFO_BIT       (1UL << 4)

typedef volatile struct {
    /* SITAL core registers (0x00 - 0x3E) */
    struct {
        u16     irq_en1;                /* 0x00: Interrupt Mask Register #1 (RD/WR) */
        u16     config1;                /* 0x02: Configuration Register #1 (RD/WR) */
        u16     config2;                /* 0x04: Configuration Register #2 (RD/WR) */
        union {
            u16     start_reset;        /* 0x06: Start/Reset Register (WR) */
            u16     stack_ptr;          /* 0x06: Stack Pointer Register (RD) */
        };
        u16     rt_subaddr_ctrl;        /* 0x08: RT Subaddress Control Word Register (RD) */
        u16     time_tag;               /* 0x0A: Time Tag Register (RD/WR) */
        u16     irq_stat1;              /* 0x0C: Interrupt Status Register #1(RD) */
        u16     config3;                /* 0x0E: Configuration Register #3 (RD/WR) */
        u16     config4;                /* 0x10: Configuration Register #4 (RD/WR) */
        u16     config5;                /* 0x12: Configuration Register #5 (RD/WR) */
        u16     _reserved1;             /* 0x14: Reserved */
        u16     bc_frame_time_left;     /* 0x16: BC Frame Timing Remaining (RD) */
        u16     bc_msg_time_left;       /* 0x18: BC Message Timing Remaining (RD) */
        union {
            u16     rt_last_cmd;        /* 0x1A: RT Last Command (RD) */
            u16     bc_frame_time;      /* 0x1A: BC Frame Time Register (WR) */
        };
        u16     rt_stat;                /* 0x1C: RT Status Word Register (RD) */
        u16     rt_bit;                 /* 0x1E: RT BIT Word Register (RD) */
        u16     _reserved2;             /* 0x20: Reserved */
        u16     test;                   /* 0x22: Test register */
        u16     bus_a_coupl;            /* 0x24: Bus A coupling to bus information */
        u16     bus_b_coupl;            /* 0x26: Bus B coupling to bus information */
        u16     _reserved3;             /* 0x28: Reserved */
        u16     latched_time_tag;       /* 0x2A: Latched Time Tag Register */
        u16     _reserved4[2];          /* 0x2C - 0x2E: Reserved */
        u16     config6;                /* 0x30: Configuration Register #6 (RD/WR) */
        u16     config7;                /* 0x32: Configuration Register #7 (RD/WR) */
        u16     version;                /* 0x34: version of Core (RD) */
        union {
            u16     ebc_cond;           /* 0x36: eBC Conditions register (RD) */
            u16     ebc_gp_flag;        /* 0x36: eBC General Purpose Flag setting (WR) */
        };
        u16     bit_stat;               /* 0x38: BIT Test Status Register (RD) */
        u16     irq_en2;                /* 0x3A: Interrupt Mask Register #2 (RD/WR) */
        u16     irq_stat2;              /* 0x3C: Interrupt Status Register #2 (RD) */
        u16     ebc_gp_q_ptr;           /* 0x3E: eBC General Purpose Queue pointer (WR/RD) */
    } core;

    /* Reserved (0x40 - 0x7E) */
    u16     _reserved1[32];

    /* Auxiliary registers (0x80 - 0xAF) */
    struct {
        u16     bus_rt_addr;            /* 0x80: RT address from backplane */
        u16     reset;                  /* 0x82: Reset */
        u16     misc;                   /* 0x84: Misc Bits */
        u16     ext_trigger;            /* 0x86: Ext Trigger (WO) */
        u16     strobe_tt;              /* 0x88: Core strobe TT input (WO) */
        u16     tag_clk_ticks;          /* 0x8A: Number of 10 us ticks between TAG_CLK pulses */
        u16     echo[2];                /* 0x8C - 0x8E: Echo Registers */
        u16     rx_msg_count;           /* 0x90: RX FIFO message count */
        u16     rx_frag_count;          /* 0x92: RX FIFO fragment count */
        u16     rx_fifo;                /* 0x94: RX FIFO */
        u16     _reserved1;             /* 0x96: Reserved */
        u16     tx_msg_count;           /* 0x98: TX FIFO message count */
        u16     tx_frag_count;          /* 0x9A: TX FIFO fragment count */
        u16     tx_fifo;                /* 0x9C: TX FIFO */
        u16     _reserved2;             /* 0x9E: Reserved */
        u16     ctrl;                   /* 0xA0: Assisted Mode Control */
        u16     _reserved3[6];          /* 0xA2 - 0xAC: Reserved */
        u8      id[2];                  /* 0xAE: Core ID */
    } aux;
} CH_REGS;

/*
 * HPS side registers
 */
#define START_TRANSFER_BIT      (1UL << 0)

typedef volatile struct {
    u16     hdr_addr;           /* 0x00: Write *word* address of header */
    u16     msg_addr;           /* 0x02: Write *word* address of payload */
    u16     count;              /* 0x04: number of words to transfer (1..32) */
    u16     start;              /* 0x06: Write 1 to LSB to start transfer. Bit will return to 0 when transfer is complete */
    u16     last_tx;            /* 0x08: Last Transmit Msg Addr/subaddr/count */
    u16     last_rx;            /* 0x0A: Last Receivev Msg Addr/subaddr/count */
    u16     _reserved[10];      /* 0x0C - 0x1E: Reserved */
} HPS_REGS;

/*
 * Channel memory map
 */
typedef volatile struct {
    //@@@ define memory layout
    u16     _reserved;      /* 0x00:  */
} CH_MEM;

/*
 * ID registers
 */
typedef volatile struct {
    u32     id;      /* 0x00: ID (1=FT1, 2=FT2, etc.) */
} ID_REGS;

#endif /* __MODULE_H__ */

