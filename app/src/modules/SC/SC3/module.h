/*
 * SC3 module specific definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __MODULE_H__
#define __MODULE_H__

/*
 * Module memory layout
 */
#define MODULE_MEM_MAP_REV  1
#define HOST_REGS_BASE      MODULE_REGS_BASE
#define HPS_REGS_BASE       MODULE_PRIV_REGS_BASE

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
    NUM_CHANNELS
} CHANNEL;

typedef enum
{
    INVALID_MODE    = 0,
    HDLC_MODE       = 1,
    ASYNC_MODE      = 2
} MODE;

#define MAX_BAUD_RATE           20000000
#define BAUD_MULTIPLIER         34.359738368

#define MAX_FIFO_SIZE           0x100000UL      // 1M DWORDs (4 MB)
#define SDRAM_TO_FIFO_SHIFT     22              // top 12 bits; in DWORDs
#define FIFO_WATERMARK          0xFFF9B

#define FIFO_BASE               0x4000000UL     // use upper 64MB of SDRAM
#define TX_FIFO(ch)             (FIFO_BASE + (ch * (MAX_FIFO_SIZE * sizeof(u32))))
#define RX_FIFO(ch)             (FIFO_BASE + ((ch + NUM_CHANNELS) * (MAX_FIFO_SIZE * sizeof(u32))))

#define BIT_DATA_LEN            32UL    // 32 DWORDs

#define HOST2FPGA(reg)          (reg = reg)
#define CH_IRQ(ch)              (ch + 1)

typedef volatile struct {
    u32 proto;
    u32 clk_mode;
    u32 if_levels;
    u32 tx_rx_cfg;
    u32 data_cfg;
    u32 baud;
    u32 preamble;
    u32 tx_fifo_a_empty;
    u32 rx_fifo_a_full;
    u32 rx_fifo_hi;
    u32 rx_fifo_lo;
    u32 hdlc_rx_char;
    u32 hdlc_tx_char;
    u32 xon_char;
    u32 xoff_char;
    u32 term_char;
    u32 timeout;
} CONFIG;

/*
 * HOST side registers
 */
#define HOST_PROTO_MODE_ASYNC           0
#define HOST_PROTO_MODE_HDLC            3

#define HOST_CLOCK_MODE_INTERNAL        0
#define HOST_CLOCK_MODE_EXTERNAL        1

#define HOST_IF_LEVELS_MODE_RS232       0
#define HOST_IF_LEVELS_MODE_RS422       2
#define HOST_IF_LEVELS_MODE_RS485       3
#define HOST_IF_LEVELS_MODE_RS423       6
#define HOST_IF_LEVELS_MODE_M_LOOPBACK  4
#define HOST_IF_LEVELS_MODE_TRISTATE    5
#define HOST_IF_LEVELS_FPGA_LOOPBACK    (1UL << 6)
#define HOST_IF_LEVELS_GPIO             (1UL << 14)

#define HOST_TX_RX_CFG_RUN_BIT          (1UL << 27)

#define HOST_CTRL_TRISTATE_TX_BIT       (1UL << 8)
#define HOST_CTRL_CHANNEL_RESET_BIT     (1UL << 13)
#define HOST_CTRL_RX_FIFO_CLEAR_BIT     (1UL << 14)
#define HOST_CTRL_TX_FIFO_CLEAR_BIT     (1UL << 15)
#define HOST_CTRL_TX_INIT_BIT           (1UL << 16)
#define HOST_CTRL_TX_ALWAYS_BIT         (1UL << 17)
#define HOST_CTRL_RCVR_EN_BIT           (1UL << 18)

#define HOST_DATA_CFG_ENC_NONE          0

#define HOST_CH_STAT_RX_DATA_AVAIL_BIT  (1UL << 4)
#define HOST_CH_STAT_TX_COMPLETE_BIT    (1UL << 9)

typedef volatile struct {
    /* 0x00: Tx Buffer */
    u32 tx_fifo;

    /* 0x04: Rx Buffer */
    u32 rx_fifo;

    /* 0x08: Number Of Words Tx Buffer */
    u32 tx_count;

    /* 0x0C: Number Of Words Rx Buffer */
    u32 rx_count;

    /* 0x10: Protocol */
    union {
        u32 reg;
        struct {
            u32 mode        :  3;   // Bits 0-2
            u32 _reserved   : 29;   // Bits 3-31
        } bits;
    } proto;

    /* 0x14: Clock Mode */
    union {
        u32 reg;
        struct {
            u32 mode        :  1;   // Bit  0
            u32 _reserved1  :  5;   // Bits 1-5
            u32 inv_tx_clk  :  1;   // Bit  6
            u32 inv_rx_clk  :  1;   // Bit  7
            u32 _reserved2  : 24;   // Bits 8-31
        } bits;
    } clk_mode;

    /* 0x18: Interface Levels */
    union {
        u32 reg;
        struct {
            u32 mode            :  3;   // Bits 0-2
            u32 _reserved1      :  1;   // Bit  3
            u32 inv_tx_data     :  1;   // Bit  4
            u32 inv_rx_data     :  1;   // Bit  5
            u32 fpga_loopback   :  1;   // Bit  6
            u32 _reserved2      :  7;   // Bits 7-13
            u32 gpio            :  1;   // Bit  14
            u32 _reserved3      : 17;   // Bits 15-31
        } bits;
    } if_levels;

    /* 0x1C: Tx-Rx Configuration */
    union {
        u32 reg;
        struct {
            u32 rts_cts             :  1;   // Bit  0
            u32 _reserved1          :  5;   // Bits 1-5
            u32 addr_rec            :  1;   // Bit  6
            u32 addr_len            :  1;   // Bit  7
            u32 _reserved2          :  2;   // Bits 8-9
            u32 sync_char_len       :  1;   // Bit  10
            u32 sync_char_data      :  1;   // Bit  11
            u32 term_char_det       :  1;   // Bit  12
            u32 xon_xoff            :  1;   // Bit  13
            u32 xon_xoff_char_data  :  1;   // Bit  14
            u32 timeout_det         :  1;   // Bit  15
            u32 crc_reset_val       :  1;   // Bit  16
            u32 crc_type            :  2;   // Bits 17-18
            u32 crc_with_data       :  1;   // Bit  19
            u32 _reserved3          :  1;   // Bit  20
            u32 idle_flag_trans     :  1;   // Bit  21
            u32 data_inversion      :  1;   // Bit  22
            u32 rx_suppression      :  1;   // Bit  23
            u32 channel_en          :  1;   // Bit  24
            u32 invert_cts          :  1;   // Bit  25
            u32 invert_rts          :  1;   // Bit  26
            u32 run_bit             :  1;   // Bit  27
            u32 _reserved4          :  4;   // Bits 28-31
        } bits;
    } tx_rx_cfg;

    /* 0x20: Control */
    union {
        u32 reg;
        struct {
            u32 rts_gpio        :  1;   // Bit  0
            u32 gpio2           :  1;   // Bit  1
            u32 _reserved1      :  6;   // Bits 2-7
            u32 tristate_tx     :  1;   // Bit  8
            u32 _reserved2      :  1;   // Bit  9
            u32 break_sr        :  1;   // Bit  10
            u32 _reserved3      :  2;   // Bits 11-12
            u32 ch_reset        :  1;   // Bit  13
            u32 rx_fifo_clear   :  1;   // Bit  14
            u32 tx_fifo_clear   :  1;   // Bit  15
            u32 tx_init         :  1;   // Bit  16
            u32 tx_always       :  1;   // Bit  17
            u32 rcvr_en         :  1;   // Bit  18
            u32 _reserved4      : 13;   // Bits 19-31
        } bits;
    } ctrl;

    /* 0x24: Data Configuration */
    union {
        u32 reg;
        struct {
            u32 data_bits   :  4;   // Bits 0-3
            u32 parity      :  3;   // Bits 4-6
            u32 _reserved1  :  1;   // Bit  7
            u32 stop_bits   :  2;   // Bits 8-9
            u32 _reserved2  :  2;   // Bits 10-11
            u32 encoding    :  3;   // Bits 12-14
            u32 _reserved3  : 17;   // Bits 15-31
        } bits;
    } data_cfg;

    /* 0x28: Baud Rate */
    u32 baud;

    /* 0x2C: Preamble */
    union {
        u32 reg;
        struct {
            u32 value       :  8;   // Bit  0-7
            u32 _reserved1  :  4;   // Bits 8-11
            u32 count       :  4;   // Bits 12-15
            u32 _reserved2  : 16;   // Bits 16-31
        } bits;
    } preamble;

    /* 0x30: Tx Buffer Almost Empty */
    u32 tx_fifo_a_empty;

    /* 0x34: Rx Buffer Almost Full */
    u32 rx_fifo_a_full;

    /* 0x38: Rx Buffer High Watermark */
    u32 rx_fifo_hi;

    /* 0x3C: Rx Buffer Low Watermark */
    u32 rx_fifo_lo;

    /* 0x40: HDLC Rx Address/Sync Char */
    u32 hdlc_rx_char;

    /* 0x44: HDLC Tx Address/Sync Char */
    u32 hdlc_tx_char;

    /* 0x48: XON Character */
    u32 xon_char;

    /* 0x4C: XOFF Character */
    u32 xoff_char;

    /* 0x50: Termination Character */
    u32 term_char;

    /* 0x54: Time Out Value */
    u32 timeout;

    /* 0x58: FIFO Status */
    union {
        u32 reg;
        struct {
            u32 rx_fifo_a_full      :  1;   // Bit  0
            u32 tx_fifo_a_empty     :  1;   // Bit  1
            u32 hi_water_reached    :  1;   // Bit  2
            u32 lo_water_reached    :  1;   // Bit  3
            u32 rx_empty            :  1;   // Bit  4
            u32 tx_full             :  1;   // Bit  5
            u32 _reserved           : 26;   // Bits 6-31
        } bits;
    } fifo_stat;

    /* 0x5C: Tx FIFO Size */
    u32 tx_fifo_size;

    /* 0x60: Rx FIFO Size */
    u32 rx_fifo_size;

    /* 0x64 - 0x7C: Reserved */
    u32 _reserved[7];
} HOST_REGS;

/*
 * HPS side registers
 */
#define HPS_CTRL_FIFO_EN_BIT            (1UL << 0)
#define HPS_CTRL_FIFO_RESET_BIT         (1UL << 1)
#define HPS_CTRL_SEND_PARAMS_BIT        (1UL << 2)
#define HPS_CTRL_CH_CTRL_BIT            (1UL << 3)
#define HPS_CTRL_BIT_PASSED_BIT         (1UL << 4)

#define HPS_CTRL_CH_CTRL_HPS            0
#define HPS_CTRL_CH_CTRL_HOST           1

#define HPS_COMM_PARAMS_RS232           0x8
#define HPS_COMM_PARAMS_RS422_INT_CLK   0xA
#define HPS_COMM_PARAMS_RS485_INT_CLK   0x2
#define HPS_COMM_PARAMS_M_LOOPBACK      0xC
#define HPS_COMM_PARAMS_TRISTATE        0xB

typedef volatile struct {
    /* 0x00: DDR Tx Fifo Start Address */
    u32 tx_fifo_start;

    /* 0x04: DDR Rx Fifo Start Address */
    u32 rx_fifo_start;

    /* 0x08: DDR Tx Fifo Full Count */
    u32 tx_fifo_full;

    /* 0x0C: DDR Rx Fifo Full Count */
    u32 rx_fifo_full;

    /* 0x10: HPS Control */
    union {
        u32 reg;
        struct {
            u32 fifo_en     :  1;   // Bit  0
            u32 fifo_reset  :  1;   // Bit  1
            u32 send_params :  1;   // Bit  2
            u32 ch_ctrl     :  1;   // Bit  3
            u32 bit_passed  :  1;   // Bit  4
            u32 bit_mode    :  1;   // Bit  5
            u32 _reserved   : 26;   // Bits 6-31
        } bits;
    } hps_ctrl;

    /* 0x14: Baud Rate */
    u32 baud;

    /* 0x18: PSEnable */
    union {
        u32 reg;
        struct {
            u32 enable     :  1;   // Bit  0
            u32 _reserved  : 31;   // Bits 1-31
        } bits;
    } ps_en;

    /* 0x1C: CommParams */
    union {
        u32 reg;
        struct {
            u32 _reserved1 :  2;   // Bits 0-1
            u32 rs423      :  1;   // Bit  2
            u32 rx_en      :  1;   // Bit  3
            u32 clkin_423  :  1;   // Bit  4
            u32 _reserved2 :  2;   // Bits 5-6
            u32 rs485_232  :  1;   // Bit  7
            u32 _reserved3 :  1;   // Bit  8
            u32 clkin      :  1;   // Bit  9
            u32 clkout     :  1;   // Bit  10
            u32 loopback   :  1;   // Bit  11
            u32 term_off   :  1;   // Bit  12
            u32 d2_0       :  3;   // Bits 13-15
            u32 _reserved4 : 16;   // Bits 16-31
        } bits;
    } comm_params;

    /* 0x20 - 0x3C: Reserved */
    u32 _reserved[8];
} HPS_REGS;

/*
 * Global pointers
 */
extern HOST_REGS *pHostRegs[NUM_CHANNELS];
extern HPS_REGS  *pHpsRegs[NUM_CHANNELS];

/*
 * Function prototypes
 */
XStatus bit_me(CHANNEL ch, bool verbose);

#endif /* __MODULE_H__ */

