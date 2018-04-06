/*
 * DT1 module specific definitions.
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
#define BANK_REGS_BASE          CH_REGS_BASE
#define PATTERN_GEN_RAM_BASE    (MODULE_REGS_BASE + 0x2000)
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

typedef enum
{
    BANK_A = 0,
    BANK_B,
    BANK_C,
    BANK_D,
    NUM_BANKS
} BANK;

/*
 * System registers
 */
typedef volatile struct {
    u32     read_io;                /* 0x000: Read I/O */
    u32     polarity;               /* 0x004: Polarity */
    u32     _reserved1[3];          /* 0x008 - 0x010: Reserved */
    u32     start_patt_out;         /* 0x014: Start Pattern Ram Output */
    u32     master_sel;             /* 0x018: Master Select */
    u32     bit_err_irq_interval;   /* 0x01C: BIT error interrupt interval */
    u32     _reserved2;             /* 0x020: Reserved */
    u32     write_out;              /* 0x024: Write Outputs (DATAOUT register) */
    u32     _reserved3;             /* 0x028: Reserved */
    u32     ram_period;             /* 0x02C: Ram Period */
    u32     _reserved4;             /* 0x030: Reserved */
    u32     io_mode_en;             /* 0x034: IO Mode enable */
    u32     io_format_lo;           /* 0x038: IO Format LO */
    u32     io_format_hi;           /* 0x03C: IO Format HI */
    u32     _reserved5[48];         /* 0x040 - 0x0FC: Reserved */
    u32     oc_reset;               /* 0x100: Overcurrent Reset */
    u32     pullup_down;            /* 0x104: Select pullup or pulldown */
} SYS_REGS;

/*
 * Channel registers
 */
typedef volatile struct {
    u32     reset_mode_timers;      /* 0x000: reset MODE and Timers */
    u32     reset_timers;           /* 0x004: reset Timers only */
    u32     reset_fifo;             /* 0x008: reset FIFO */
    u32     period_debounce;        /* 0x00C: Period (output modes 11,12) or Debounce(any input mode) */
    u32     duty_cycle;             /* 0x010: Duty cycle (high time, modes 11,12) */
    u32     num_cycles;             /* 0x014: Number of cycles  'n' (mode 12) */
    u32     mode;                   /* 0x018: MODE register */
    u32     start_pwm;              /* 0x01C: Start PWM output */
    u32     fifo_data;              /* 0x020: Read FIFO data */
    u32     fifo_stat;              /* 0x024: Read FIFO status  data: 0x00000yyy */
    u32     _reserved1[6];          /* 0x028 - 0x03C: Reserved */
    u32     max_hi_thr;             /* 0x040: Max high threshold (above is sometimes an error) */
    u32     upper_thr;              /* 0x044: Upper threshold (below this, hysteresis applies) */
    u32     lower_thr;              /* 0x048: Lower threshold (above this hysteresis applies) */
    u32     min_lo_thr;             /* 0x04C: Min low threshold (below is sometimes an error) */
    u32     _reserved2;             /* 0x050: Reserved */
    u32     debounce_time;          /* 0x054: Debounce time in ticks (temporary) */
    u32     _reserved3[2];          /* 0x058 - 0x05C: Reserved */
    u32     volt;                   /* 0x060: Measured channel voltage */
    u32     current;                /* 0x064: Measured channel current */
    u32     bit_volt;               /* 0x068: Measured BIT voltage */
    u32     _reserved4[5];          /* 0x06C - 0x07C: Reserved */
} CH_REGS;

/*
 * Bank registers
 */
typedef volatile struct {
    u32     _reserved1[20];         /* 0x000 - 0x04C: Reserved */
    u32     source_sink_current;    /* 0x050: Source/Sink Current */
    u32     _reserved2[6];          /* 0x054 - 0x068: Reserved */
    u32     vcc;                    /* 0x06C: Measured VCC */
    u32     _reserved3[4];          /* 0x070 - 0x07C: Reserved */
} BANK_REGS;

/*
 * HPS side registers
 */
typedef volatile struct {
    u32     cpld_en;                /* 0x000: CPLD Enable (Mod Go) */
} HPS_REGS;

/*
 * Calibration data
 */
typedef volatile struct {
    u32     _reserved1;                         /* 0x000: Reserved */
    u32     volt_gain_ch[NUM_CHANNELS];         /* 0x004 - 0x060: voltage gain calibration datum */
    u32     volt_gain_bank[NUM_BANKS];          /* 0x064 - 0x070: voltage gain calibration datum */
    u32     _reserved2[4];                      /* 0x074 - 0x080: Reserved */
    u32     current_gain[NUM_CHANNELS];         /* 0x084 - 0x0E0: current gain calibration datum */
    u32     _reserved3[8];                      /* 0x0E4 - 0x100: Reserved */
    u32     bit_volt_gain[NUM_CHANNELS];        /* 0x104 - 0x160: BIT voltage gain calibration datum */
    u32     _reserved4[7];                      /* 0x164 - 0x17C: Reserved */
    struct {                                    /* 0x180 - 0x19C: Pullup/Pulldown multiplier */
        u32     pullup_dis_mult;                /*         0x000: Pullup disabled multiplier */
        u32     pulldown_en_mult;               /*         0x004: Pulldown enabled multiplier */
    } pull_mult1[NUM_BANKS];
    struct {                                    /* 0x1A0 - 0x1BC: Pullup/Pulldown multiplier */
        u32     pullup_en_mult;                 /*         0x000: Pullup enabled multiplier */
        u32     pulldown_dis_mult;              /*         0x004: Pulldown disabled multiplier */
    } pull_mult2[NUM_BANKS];
    u32     _reserved5[17];                     /* 0x1C0 - 0x200: Reserved */
    u32     volt_offset_ch[NUM_CHANNELS];       /* 0x204 - 0x260: voltage offset calibration datum */
    u32     volt_offset_bank[NUM_BANKS];        /* 0x264 - 0x270: voltage offset calibration datum */
    u32     _reserved6[4];                      /* 0x274 - 0x280: Reserved */
    u32     current_offset[NUM_CHANNELS];       /* 0x284 - 0x2E0: current offset calibration datum */
    u32     _reserved7[8];                      /* 0x2E4 - 0x300: Reserved */
    u32     bit_volt_offset[NUM_CHANNELS];      /* 0x304 - 0x360: BIT voltage offset calibration datum */
    u32     _reserved8[7];                      /* 0x364 - 0x37C: Reserved */
    struct {                                    /* 0x380 - 0x39C: Pullup/Pulldown offset */
        u32     pullup_dis_offset;              /*         0x000: Pullup disabled offset */
        u32     pulldown_en_offset;             /*         0x004: Pulldown enabled offset */
    } pull_offset1[NUM_BANKS];
    struct {                                    /* 0x3A0 - 0x3BC: Pullup/Pulldown offset */
        u32     pullup_en_offset;               /*         0x000: Pullup enabled offset */
        u32     pulldown_dis_offset;            /*         0x004: Pulldown disabled offset */
    } pull_offset2[NUM_BANKS];
    u32     _reserved9[177];                    /* 0x3C0 - 0x680: Reserved */
    u32     current_offset_60v[NUM_CHANNELS];   /* 0x684 - 0x6E0: current offset 60V calibration datum */
} CAL_DATA;

/*
 * ID registers
 */
typedef volatile struct {
    u32     id;      /* 0x00: ID (1=DT1, 2=DT2, etc.) */
} ID_REGS;

#endif /* __MODULE_H__ */

