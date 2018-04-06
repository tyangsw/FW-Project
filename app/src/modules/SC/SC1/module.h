/*
 * SC1 module specific definitions.
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

#define MAX_FIFO_SIZE           0x200000UL      // 2M DWORDs (8 MB)
#define SDRAM_TO_FIFO_SHIFT     23              // top 11 bits; in DWORDs
#define FIFO_WATERMARK          0x1FFF9B

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
    /* 0x000 - 0x00C: Tx Buffer */
    u32 tx_fifo[NUM_CHANNELS];

    /* 0x010 - 0x01C: Rx Buffer */
    u32 rx_fifo[NUM_CHANNELS];

    /* 0x020 - 0x02C: Number Of Words Tx Buffer */
    u32 tx_count[NUM_CHANNELS];

    /* 0x030 - 0x03C: Number Of Words Rx Buffer */
    u32 rx_count[NUM_CHANNELS];

    /* 0x040 - 0x04C: Protocol */
    union {
        u32 reg;
        struct {
            u32 mode        :  3;   // Bits 0-2
            u32 _reserved   : 29;   // Bits 3-31
        } bits;
    } proto[NUM_CHANNELS];

    /* 0x050 - 0x05C: Clock Mode */
    union {
        u32 reg;
        struct {
            u32 mode        :  1;   // Bit  0
            u32 _reserved1  :  5;   // Bits 1-5
            u32 inv_tx_clk  :  1;   // Bit  6
            u32 inv_rx_clk  :  1;   // Bit  7
            u32 _reserved2  : 24;   // Bits 8-31
        } bits;
    } clk_mode[NUM_CHANNELS];

    /* 0x060 - 0x06C: Interface Levels */
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
    } if_levels[NUM_CHANNELS];

    /* 0x070 - 0x07C: Tx-Rx Configuration */
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
    } tx_rx_cfg[NUM_CHANNELS];

    /* 0x080 - 0x08C: Control */
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
    } ctrl[NUM_CHANNELS];

    /* 0x090 - 0x09C: Data Configuration */
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
    } data_cfg[NUM_CHANNELS];

    /* 0x0A0 - 0x0AC: Baud Rate */
    u32 baud[NUM_CHANNELS];

    /* 0x0B0 - 0x0BC: Preamble */
    union {
        u32 reg;
        struct {
            u32 value       :  8;   // Bit  0-7
            u32 _reserved1  :  4;   // Bits 8-11
            u32 count       :  4;   // Bits 12-15
            u32 _reserved2  : 16;   // Bits 16-31
        } bits;
    } preamble[NUM_CHANNELS];

    /* 0x0C0 - 0x0CC: Tx Buffer Almost Empty */
    u32 tx_fifo_a_empty[NUM_CHANNELS];

    /* 0x0D0 - 0x0DC: Rx Buffer Almost Full */
    u32 rx_fifo_a_full[NUM_CHANNELS];

    /* 0x0E0 - 0x0EC: Rx Buffer High Watermark */
    u32 rx_fifo_hi[NUM_CHANNELS];

    /* 0x0F0 - 0x0FC: Rx Buffer Low Watermark */
    u32 rx_fifo_lo[NUM_CHANNELS];

    /* 0x100 - 0x10C: HDLC Rx Address/Sync Char */
    u32 hdlc_rx_char[NUM_CHANNELS];

    /* 0x110 - 0x11C: HDLC Tx Address/Sync Char */
    u32 hdlc_tx_char[NUM_CHANNELS];

    /* 0x120 - 0x12C: XON Character */
    u32 xon_char[NUM_CHANNELS];

    /* 0x130 - 0x13C: XOFF Character */
    u32 xoff_char[NUM_CHANNELS];

    /* 0x140 - 0x14C: Termination Character */
    u32 term_char[NUM_CHANNELS];

    /* 0x150 - 0x15C: Time Out Value */
    u32 timeout[NUM_CHANNELS];

    /* 0x160 - 0x16C: FIFO Status */
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
    } fifo_stat[NUM_CHANNELS];

    /* 0x170 - 0x17C: Tx FIFO Size */
    u32 tx_fifo_size[NUM_CHANNELS];

    /* 0x180 - 0x18C: Rx FIFO Size */
    u32 rx_fifo_size[NUM_CHANNELS];
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

#define HPS_COMM_PARAMS_RS232_ASYNC     0x7610
#define HPS_COMM_PARAMS_RS232_SYNC      0x7410
#define HPS_COMM_PARAMS_RS422_INT_CLK   0xD690
#define HPS_COMM_PARAMS_RS422_EXT_CLK   0xD090
#define HPS_COMM_PARAMS_RS485_INT_CLK   0xC690
#define HPS_COMM_PARAMS_RS485_EXT_CLK   0xC090
#define HPS_COMM_PARAMS_RS423           0x9484
#define HPS_COMM_PARAMS_M_LOOPBACK      0xFC90
#define HPS_COMM_PARAMS_TRISTATE        0xF498

typedef volatile struct {
    /* 0x00 - 0x0C: DDR Tx Fifo Start Address */
    u32 tx_fifo_start[NUM_CHANNELS];

    /* 0x10 - 0x1C: DDR Rx Fifo Start Address */
    u32 rx_fifo_start[NUM_CHANNELS];

    /* 0x20 - 0x2C: DDR Tx Fifo Full Count */
    u32 tx_fifo_full[NUM_CHANNELS];

    /* 0x30 - 0x3C: DDR Rx Fifo Full Count */
    u32 rx_fifo_full[NUM_CHANNELS];

    /* 0x40 - 0x4C: HPS Control */
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
    } hps_ctrl[NUM_CHANNELS];

    /* 0x50 - 0x5C: Baud Rate */
    u32 baud[NUM_CHANNELS];

    /* 0x60 - 0x6C: PSEnable */
    union {
        u32 reg;
        struct {
            u32 enable     :  1;   // Bit  0
            u32 _reserved  : 31;   // Bits 1-31
        } bits;
    } ps_en[NUM_CHANNELS];

    /* 0x70 - 0x7C: CommParams */
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
    } comm_params[NUM_CHANNELS];
} HPS_REGS;

/*
 * Global pointers
 */
extern HOST_REGS *pHostRegs;
extern HPS_REGS  *pHpsRegs;

/*
 * Function prototypes
 */
XStatus bit_me(CHANNEL ch, bool verbose);

#endif /* __MODULE_H__ */

