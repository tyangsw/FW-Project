/*
 * RTx module specific definitions.
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
#define SYS_REGS_BASE       (MODULE_REGS_BASE + 0x1000)
#define CTRL_REGS_BASE      (MODULE_PRIV_REGS_BASE + 0x40)
#define CH_REGS_BASE        (MODULE_PRIV_REGS_BASE + 0x1000)

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
    NUM_CHANNELS
} CHANNEL;

#define CHANNEL_MASK        ((1UL << NUM_CHANNELS) - 1)

#define SPI_TIMEOUT         1000    // tCONV = 200.295 ms at 5 SPS

//#define RESOLUTION_1000UA   1.192093038e-7  // 1000ua*1k / (2e23-1)
#define RESOLUTION_1000UA   238.4186075e-9  // 1000ua*2k / (2e23-1)
#define RESOLUTION_500UA    59.60465188e-9  // 500ua*1k / (2e23-1)
#define RESOLUTION_100UA    9.536744301e-8  // 100ua*8k / (2e23-1)

#define IEXC_500UA          500e-6
#define IEXC_100UA          100e-6
#define IEXC_1000UA         1000e-6

#define CONST_ADC_CONV      2.44140625e-7

#define BIT_TOLERANCE       5.0f

/*
 * HOST side registers
 */
typedef enum
{
    RES0_100_OHM = 0,
    RES0_500_OHM,
    RES0_1000_OHM,
    NUM_RES0
} RES0;

typedef enum
{
    MODE_2_WIRE = 0,
    MODE_3_WIRE,
    MODE_4_WIRE,
    NUM_MODES
} MODE;

typedef volatile struct {
    float   res;                /* 0x00: Resistance */
    float   temp;               /* 0x04: Temperature in Celsius */
    float   temp_f;             /* 0x08: Temperature in Fahrenheit */
    u32     res0;               /* 0x0C: Resistance at 0 degrees Celsius */
    u32     mode;               /* 0x10: Wire mode (2/3/4) */
    float   res_comp;           /* 0x14: 2-wire mode resistance compensation */
    float   low1_thr;           /* 0x18: Low 1 (alert) temperature threshold (in deg C) */
    float   low2_thr;           /* 0x1C: Low 2 (alarm) temperature threshold (in deg C) */
    float   high1_thr;          /* 0x20: High 1 (alert) temperature threshold (in deg C) */
    float   high2_thr;          /* 0x24: High 2 (alarm) temperature threshold (in deg C) */
    u32     _reserved[6];       /* 0x18 - 0x3C: Reserved */
} HOST_REGS;

typedef volatile struct {
    u32     _reserved[1024];    /* 0x000 - 0xFFC: Reserved */
    u32     bit_interval;       /* 0x1000: BIT interval (in milliseconds) */
} SYS_REGS;

/*
 * HPS side registers
 */
#define MUX0_BURNOUT_OFF        (0x00 << 6)     // Burnout current source off (default)
#define MUX0_BURNOUT_ON         (0x01 << 6)     // Burnout current source on, 0.5µA
#define MUX0_AIN0_P             (0x00 << 3)     // AIN0+
#define MUX0_AIN4_P             (0x04 << 3)     // AIN4+
#define MUX0_AIN1_N             (0x01 << 0)     // AIN1-
#define MUX0_AIN5_N             (0x05 << 0)     // AIN5-
#define MUX0_NORMAL_MODE        (MUX0_BURNOUT_OFF | MUX0_AIN0_P | MUX0_AIN1_N)
#define MUX0_BIT                (MUX0_BURNOUT_OFF | MUX0_AIN4_P | MUX0_AIN5_N)
#define MUX0_OC                 (MUX0_BURNOUT_ON | MUX0_AIN0_P | MUX0_AIN1_N)

#define MUX1_VREFCON_INT        (0x01 << 5)     // Internal Vref On
#define MUX1_REFSELT_REF1       (0x01 << 3)     // REF1 input pair selected 
#define MUX1_REFSELT_ONCHIP     (0x02 << 3)     // Onboard reference selected
#define MUX1_MUXCAL_NORMAL_OP   (0x00 << 0)     // Normal operation
#define MUX1_MUXCAL_TEMP        (0x03 << 0)     // Temperature diode
#define MUX1_MUXCAL_AVDD        (0x06 << 0)     // AVDD measurement
#define MUX1_MUXCAL_DVDD        (0x07 << 0)     // DVDD measurement
#define MUX1_TEMP_MODE          (MUX1_VREFCON_INT | MUX1_REFSELT_ONCHIP | MUX1_MUXCAL_TEMP)
#define MUX1_AVDD_MODE          (MUX1_VREFCON_INT | MUX1_REFSELT_ONCHIP | MUX1_MUXCAL_AVDD)
#define MUX1_DVDD_MODE          (MUX1_VREFCON_INT | MUX1_REFSELT_ONCHIP | MUX1_MUXCAL_DVDD)
#define MUX1_NORMAL_MODE        (MUX1_VREFCON_INT | MUX1_REFSELT_REF1 | MUX1_MUXCAL_NORMAL_OP)

#define IDAC0_DRDY_MODE         (0x01 << 3)     // DRDY MODE
#define IDAC0_IMAG_100UA        (0x02 << 0)     // IMAG = 100uA
#define IDAC0_IMAG_1000UA       (0x06 << 0)     // IMAG = 1000uA
#define IDAC0_NORMAL_MODE       (IDAC0_DRDY_MODE | IDAC0_IMAG_100UA)
#define IDAC0_BIT               (IDAC0_DRDY_MODE | IDAC0_IMAG_1000UA)

#define IDAC1_OUTPUT2_IEXT1     (0x08 << 0)     // Output pin 2 = IEXT1
#define IDAC1_OUTPUT2_NONE      (0x0F << 0)     // Output pin 2 = Disconnected
#define IDAC1_OUTPUT1_AIN2      (0x02 << 4)     // Output pin 1 = AIN2
#define IDAC1_OUTPUT1_AIN3      (0x03 << 4)     // Output pin 1 = AIN3
#define IDAC1_OUTPUT1_AIN6      (0x06 << 4)     // Output pin 1 = AIN6
#define IDAC1_OUTPUT1_IEXT1     (0x08 << 4)     // Output pin 1 = IEXT1
#define IDAC1_2_WIRE            (IDAC1_OUTPUT1_IEXT1 | IDAC1_OUTPUT2_NONE)
#define IDAC1_3_WIRE            (IDAC1_OUTPUT1_AIN3 | IDAC1_OUTPUT2_IEXT1)
#define IDAC1_4_WIRE            (IDAC1_OUTPUT1_AIN2 | IDAC1_OUTPUT2_NONE)
#define IDAC1_BIT               (IDAC1_OUTPUT1_AIN6 | IDAC1_OUTPUT2_NONE)

#define GPIO_2K_RREF            0x01            // GPIO outputs 01 = 2k Rref
#define GPIO_4K_RREF            0x02            // GPIO outputs 10 = 4k Rref
#define GPIO_8K_RREF            0x03            // GPIO outputs 11 = 8k Rref
#define GPIO_SWITCH             0x80            // GPIO7 = switch ON
#define GPIO_2_WIRE             (GPIO_SWITCH | GPIO_8K_RREF)
#define GPIO_3_WIRE             GPIO_4K_RREF
#define GPIO_4_WIRE             GPIO_8K_RREF
#define GPIO_BIT                GPIO_2K_RREF

#define CMD_WAKEUP              0x01            // 0000 000x            - Exit sleep mode
#define CMD_SYNC                0x03            // 0000 010x, 0000-010x - Synchronize the A/D conversion
#define CMD_RESET               0x04            // 0000 011x            - Reset to power-up values
#define CMD_NOP                 0x05            // 1111 1111            - No operation (outputs 0xff)
#define CMD_RDATA               0x06            // 0001 001x            - Read data once
#define CMD_RDATAC              0x07            // 0001 010x            - Read data continuously
#define CMD_SDATAC              0x08            // 0001 011x            - Stop reading data continuously
#define CMD_SELFOCAL            0x0B            // 0110 0010            - Self offset calibration

typedef volatile struct {
    u32     cmd;                /* 0x40:  */
    u32     _reserved1[3];      /* 0x44 - 0x4C: Reserved */
    u32     spi_rdy;            /* 0x50:  */
    u32     _reserved2[3];      /* 0x54 - 0x5C: Reserved */
    u32     adc_data;           /* 0x60:  */
    u32     _reserved3[3];      /* 0x64 - 0x6C: Reserved */
    u32     adc_start;          /* 0x70:  */
    u32     _reserved4[3];      /* 0x74 - 0x7C: Reserved */
    u32     bit;                /* 0x80:  */
    u32     _reserved5[3];      /* 0x84 - 0x8C: Reserved */
    u32     oc;                 /* 0x90:  */
    u32     _reserved6[3];      /* 0x94 - 0x9C: Reserved */
    u32     check_drdy;         /* 0xA0:  */
    u32     _reserved7[3];      /* 0xA4 - 0xAC: Reserved */
    u32     low1;               /* 0xB0:  */
    u32     _reserved8[3];      /* 0xB4 - 0xBC: Reserved */
    u32     low2;               /* 0xC0:  */
    u32     _reserved9[3];      /* 0xC4 - 0xCC: Reserved */
    u32     high1;              /* 0xD0:  */
    u32     _reserved10[3];     /* 0xD4 - 0xDC: Reserved */
    u32     high2;              /* 0xE0:  */
} CTRL_REGS;

typedef volatile struct {
    /* ADC registers (0x00 - 0x38) */
    struct {
        u32     mux0;           /* 0x00: Multiplexer Control Register 0 */
        u32     vbias;          /* 0x04: Bias Voltage Register */
        u32     mux1;           /* 0x08: Multiplexer Control Register 1 */
        u32     sys0;           /* 0x0C: System Control Register 0 */
        u32     ofc0;           /* 0x10: Offset Calibration Coefficient Register 0 */
        u32     ofc1;           /* 0x14: Offset Calibration Coefficient Register 1 */
        u32     ofc2;           /* 0x18: Offset Calibration Coefficient Register 2 */
        u32     fsc0;           /* 0x1C: Full-Scale Calibration Coefficient Register 0 */
        u32     fsc1;           /* 0x20: Full-Scale Calibration Coefficient Register 1 */
        u32     fsc2;           /* 0x24: Full-Scale Calibration Coefficient Register 2 */
        u32     idac0;          /* 0x28: IDAC Control Register 0 */
        u32     idac1;          /* 0x2C: IDAC Control Register 1 */
        u32     gpiocfg;        /* 0x30: GPIO Configuration Register */
        u32     gpiodir;        /* 0x34: GPIO Direction Register */
        u32     gpiodat;        /* 0x38: GPIO Data Register */
    } adc;

    /* Reserved (0x3C) */
    u32         _reserved1;

    /* Control registers (0x40 - 0xE0) */
    CTRL_REGS   ctrl;

    /* Reserved (0xE4 - 0xFFC) */
    u32         _reserved2[967];
} CH_REGS;

/*
 * Calibration data
 */
typedef struct {
    float   offset;             /* 0x00: Offset calibration datum */
    float   gain;               /* 0x04: Gain calibration datum */
    float   temp;               /* 0x08: Temperature at calibration time */
    u32     _reserved;          /* 0x0C: Reserved */
} CALD;

typedef volatile struct {
    CALD    pt100[NUM_MODES];   /* 0x000 - 0x02C: PT100 calibration datum */
    u32     _reserved1[4];      /* 0x030 - 0x03C: Reserved */
    CALD    pt500[NUM_MODES];   /* 0x040 - 0x06C: PT500 calibration datum */
    u32     _reserved2[4];      /* 0x070 - 0x07C: Reserved */
    CALD    pt1000[NUM_MODES];  /* 0x080 - 0x0AC: PT1000 calibration datum */
    u32     _reserved3[84];     /* 0x0B0 - 0x1FC: Reserved */
} CAL_DATA;

/*
 * Global pointers
 */
extern HOST_REGS *pHostRegs[NUM_CHANNELS];
extern SYS_REGS  *pSysRegs;
extern CTRL_REGS *pCtrlRegs;
extern CH_REGS   *pChRegs[NUM_CHANNELS];
extern CAL_DATA  *pCalData[NUM_CHANNELS];

/*
 * Function prototypes
 */
XStatus rtd_read(void);
XStatus bit_me(bool verbose);
XStatus adc_write_reg(CHANNEL ch, volatile u32 *addr, u32 val);
XStatus adc_read_reg(CHANNEL ch, volatile u32 *addr, u32 *val);
XStatus adc_get_data(CHANNEL ch);
XStatus adc_get_data_all(void);
XStatus adc_sync(void);

#endif /* __MODULE_H__ */

