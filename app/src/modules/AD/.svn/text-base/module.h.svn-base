/*
 * ADx module specific definitions.
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
#if MODULE_AD4 || MODULE_AD5 || MODULE_AD6
    CHANNEL_13,
    CHANNEL_14,
    CHANNEL_15,
    CHANNEL_16,
#endif
    NUM_CHANNELS
} CHANNEL;

typedef enum
{
    RANGE_0 = 0,
    RANGE_1,
    RANGE_2,
    RANGE_3,
    NUM_RANGES
} RANGE;


//~~~~~~~~~~~~~~~~~~~~~~~~
// AD5 CONSTANTS
//~~~~~~~~~~~~~~~~~~~~~~~~
#define PI                      3.141592653589793
#define SQRT2                   1.414213562373095
#define ImodeNormalizer			2       // When using current mode in AD4 this normalizes range

#define D3_ERROR_LIMIT          655 	//@@@ FIXME Changing to 1% for now for LCAC until we find out why BIT A/D has more noise at the lower ranges //131     // 0.2% of Full scale range (65535)
#define MAX_D3_VOLTAGES         21      // Tests 21 different voltages
#define D0_EN_BIT               0x1     // Bit position for enabling D0 testing
#define D2_EN_BIT               0x4     // Bit position for enabling D2 testing
#define D3_EN_BIT               0x8     // Bit position for enabling D3 testing

#define RANGE_MASK				0xF		// Lower 4 bits are for range selection
#define POLARITY_MASK			0x10	// Bit 5 is for polarity selection

// Background Cal
#define AMOUNTOFSAMPLES			12      // 2^12 = Number of samples for each background calibration
#define MIN_SAMPRATE			1000	// 1kHz per channel

typedef volatile struct {
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // A/D Reading                          (0x1000 - 0x107C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    ad_reading[NUM_CHANNELS];       // (0x00001000 - 0x0000103C)
    uint32_t    reserved_1[16];                 // (0x00001040 - 0x0000107C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Polarity and Range                   (0x1080 - 0x10FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    ad_polrange[NUM_CHANNELS];      // (0x00001080 - 0x000010BC)
    uint32_t    reserved_2[16];                 // (0x000010C0 - 0x000010FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Filter Break Frequency               (0x1100 - 0x117C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    breakFreq[NUM_CHANNELS];        // (0x00001100 - 0x0000113C)
    uint32_t    reserved_3[16];                 // (0x00001140 - 0x0000117C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Buffer Data                     (0x1180 - 0x11FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifodata[NUM_CHANNELS];         // (0x00001180 - 0x000011BC)
    uint32_t    reserved_4[16];                 // (0x000011C0 - 0x000011FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Word Count                      (0x1200 - 0x127C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifocount[NUM_CHANNELS];        // (0x00001200 - 0x0000123C)
    uint32_t    reserved_5[16];                 // (0x00001240 - 0x0000127C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Almost Empty Mark               (0x1280 - 0x12FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifoEmptyMark[NUM_CHANNELS];    // (0x00001280 - 0x000012BC)
    uint32_t    reserved_6[16];                 // (0x000012C0 - 0x000012FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Almost Full Mark                (0x1300 - 0x137C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifoFullMark[NUM_CHANNELS];     // (0x00001300 - 0x0000133C)
    uint32_t    reserved_7[16];                 // (0x00001340 - 0x0000137C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Low Mark                        (0x1380 - 0x13FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifoLoMark[NUM_CHANNELS];       // (0x00001380 - 0x000013BC)
    uint32_t    reserved_8[16];                 // (0x000013C0 - 0x000013FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO High Mark                       (0x1400 - 0x147C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifoHiMark[NUM_CHANNELS];       // (0x00001400 - 0x0000143C)
    uint32_t    reserved_9[16];                 // (0x00001440 - 0x0000147C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Buffer Delay                    (0x1480 - 0x14FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifoBufferDelay[NUM_CHANNELS];  // (0x00001480 - 0x000014BC)
    uint32_t    reserved_10[16];                // (0x000014C0 - 0x000014FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Buffer Size                     (0x1500 - 0x157C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifoBufferSize[NUM_CHANNELS];   // (0x00001500 - 0x0000153C)
    uint32_t    reserved_11[16];                // (0x00001540 - 0x0000157C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Sample Rate                          (0x1580 - 0x15FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    sampleRate[NUM_CHANNELS];       // (0x00001580 - 0x000015BC)
    uint32_t    reserved_12[16];                // (0x000015C0 - 0x000015FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Clear FIFO                           (0x1600 - 0x167C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    clearFifo[NUM_CHANNELS];        // (0x00001600 - 0x0000163C)
    uint32_t    reserved_13[16];                // (0x00001640 - 0x0000167C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Buffer Control                  (0x1680 - 0x16FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    union {
        uint32_t    reg;
        struct {
            uint32_t    data_hi     :  1;   // Bit  0 (16 bit high)
            uint32_t    data_lo     :  1;   // Bit  1 ( 8 bit low)
            uint32_t    data_type   :  1;   // Bit  2 (0:Raw, 1:Filtered)
            uint32_t    _reserved1  :  1;   // Bit  3
            uint32_t    timestamp   :  1;   // Bit  4 (0:None, 1:16bit timestamp)
            uint32_t    _reserved2  : 27;   // Bits 5-31
        } bits;
    } fifoBufferCtrl[NUM_CHANNELS];             // (0x00001680 - 0x000016BC)

    uint32_t    reserved_14[16];                // (0x000016C0 - 0x000016FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Trigger Control                 (0x1700 - 0x177C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    union {
        uint32_t    reg;
        struct {
            uint32_t    source      :  2;   // Bit  0-1 (0:Ext2, 1:Ext1, 2:Software)
            uint32_t    _reserved1  :  2;   // Bit  2-3
            uint32_t    type        :  4;   // Bit  4-7 (b0:Pos/Neg Slope, b1:PulseEn, b2:Pulse/EnSelect, b3:Clear Trig)
            uint32_t    _reserved2  :  24;  // Bit  8-31
        } bits;
    } fifoTrigCtrl[NUM_CHANNELS];               // (0x00001700 - 0x0000173C)

    uint32_t    reserved_15[16];                // (0x00001740 - 0x0000177C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Status                          (0x1780 - 0x17FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifoStatus[NUM_CHANNELS];       // (0x00001780 - 0x000017BC)
    uint32_t    reserved_16[16];                // (0x000017C0 - 0x000017FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // FIFO Interrupt Enable                (0x1800 - 0x187C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifoIntrptEn[NUM_CHANNELS];     // (0x00001800 - 0x0000183C)
    uint32_t    reserved_17[16];                // (0x00001840 - 0x0000187C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Miscellaneous Controls               (0x1880 - 0x18FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    latchAD;                        // (0x00001880) (0:Unlatched, 1:Latched)
    uint32_t    triggerSel;                     // (0x00001884) (Fifo Trigger Control)
    uint32_t    softwareTrig;                   // (0x00001888) (1:trigger)
    uint32_t    sampleRatePerChan;              // (0x0000188C) (Used to calculate the PLL Freq)
    uint32_t    reserved_18;                    // (0x00001890)
    uint32_t    iv_mode;                        // (0x00001894) (0:voltage, 1:current) bitmapped
    uint32_t    activeChan;                     // (0x00001898) (0:inactive, 1:active) bitmapped
    uint32_t    clearOverCurrent;               // (0x0000189C) (1:clear) bitmapped
    uint32_t    ad_rate;                    	// (0x000018A0)
    uint32_t    decimator_cnt;                  // (0x000018A4)
    uint32_t    reserved_19;                   	// (0x000018A8)
    uint32_t    reserved_20;                    // (0x000018AC)
    uint32_t    reserved_21;                    // (0x000018B0)
    uint32_t    BypassCal;                      // (0x000018B4) // Decoded on User bus only
    uint32_t    PLLClockFreq;                   // (0x000018B8)
    uint32_t    UseOurTxCLK;                    // (0x000018BC)
    uint32_t    reserved_24;                    // (0x000018C0)
    uint32_t    reserved_25;                    // (0x000018C4)
    uint32_t    reserved_26;                    // (0x000018C8) (possibly extra register for disabling power if unit starts as powered on by default)
    uint32_t    reserved_27[6];             	// (0x000018CC-0x000018E0)


    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// OtherMisc Internal Registers                (0x18E4 - 0x197C)
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //Calibration registers
    uint32_t    cal_en;
    uint32_t    cal_chan;
    uint32_t    cal_mode;
    uint32_t    sig_cal_val;
    uint32_t    sig_avg_val;
    uint32_t    bit_cal_val;
    uint32_t    bit_avg_val;

    // External PLL Registers
    uint32_t    PllFreqMeasTime;   				 // (0x00001900)
    uint32_t    PllFreqMeas;   					 // (0x00001904)
    uint32_t    PllVadcLock;					 // (0x00001908)
    u32 PreservingReg;							 // (0x0000190C)
    u32 TroubleshootEn;							 // (0x00001910)
    u32 TXRstStatus;							 // (0x00001914)
    u32 ManualBitChan;							 // (0x00001918)
    u32 BitADReading;							 // (0x0000191C)
    u32 reserved_28[24];

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    u32 ThresholdDetect1[NUM_CHANNELS];
    u32 reserved_29[16];
    u32 ThresholdDetCtrl1[NUM_CHANNELS];
    u32 reserved_30[16];
    u32 ThresholdDetect2[NUM_CHANNELS];
	u32 reserved_31[16];
	u32 ThresholdDetCtrl2[NUM_CHANNELS];
	u32 reserved_32[16];

} HOST_REGS;


typedef volatile struct {
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Misc Module Control (0x00 - 0x1C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    bit_chan;   // (0x00000000)
    uint32_t    dac_rstn;       // (0x00000004)

    /* TCVR Control */
    union {
        uint32_t    reg;
        struct {
            uint32_t    te_en       :  1;   // Bit  0
            uint32_t    de_en       :  1;   // Bit  1
            uint32_t    _reserved   : 30;   // Bits 2-31
        } bits;
    } tcvr_ctrl;                // (0x00000008)

    union {
        uint32_t    reg;
        struct {
            uint32_t    enable      :  1;   // Bit  0
            uint32_t    Fiforst     :  1;   // Bit  1
            uint32_t    FifoEn      :  1;   // Bit  2
            uint32_t    _reserved   : 29;   // Bits 3-31
        } bits;
    } fifo_init;                    // (0x0000000C)

    uint32_t    bit_status;         // (0x00000010)
    uint32_t    filter_enable;      // (0x00000014)
    uint32_t    ad_readForBit;      // (0x00000018)
    uint32_t    bit_reading;        // (0x0000001C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  Bit Controls (0x20 - 0x3C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    check_sel;      // (0x00000020)
    uint32_t    check_en;       // (0x00000024)
    uint32_t    bit_enable;     // (0x00000028)
    uint32_t    bit_sel;        // (0x0000002C)
    uint32_t    bit_gain;       // (0x00000030)
    uint32_t    connect_dac;    // (0x00000034)
    uint32_t    connect_gnd;    // (0x00000038)
    uint32_t    dac_data;       // (0x0000003C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  PLL Reconfiguration (0x40 - 0x5C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    forceUpdateADRange;	// (0x00000040)
    uint32_t    disableDecimation;  // (0x00000044)
    uint32_t    decimator_sel;     	// (0x00000048)
    uint32_t    MaxPolyCnt;      	// (0x0000004C)
    uint32_t    PolyBlkSize;      	// (0x00000050)
    uint32_t    PolyCoeffLen;      	// (0x00000054)
    uint32_t    PolyMacLen;  		// (0x00000058)
    uint32_t    activeChan;     	// (0x0000005C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  A/D Range Settings (0x60 - 0x9C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    union {
            uint32_t    reg;
            struct {
                uint32_t    range               :  4;   // Bits 0-3
                uint32_t    polarity            :  1;   // Bit  4
                uint32_t    _reserved           : 27;   // Bits 5-31
            } bits;
        } ad_range[NUM_CHANNELS];               // (0x00000060 - 0x0000009C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  FIFO Start Address (0x0A0 - 0x0DC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifo_strtAddy[NUM_CHANNELS];    // (0x000000A0 - 0x000000DC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  FIFO Full Count (0x0E0 - 0x11C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    fifo_fullCnt[NUM_CHANNELS];     // (0x000000E0 - 0x0000011C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  Other controls (0x120 - 0x13C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    Vrange_normalizer;              // (0x00000120)
    uint32_t    allow_currMode;                 // (0x00000124)
    uint32_t    powerEnable;                    // (0x00000128)
    uint32_t	Irange_normalizer;				// (0x0000012C)
    uint32_t	currentLimit;					// (0x00000130)
    uint32_t	firstBankChan;					// (0x00000134)
    uint32_t	bankSync;						// (0x00000138)
    uint32_t    lastBankChan;                  	// (0x0000013C)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  AD Readings (0x140 - 0x17C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    uint32_t    ad_reading[NUM_CHANNELS];       // (0x00000140 - 0x0000017C)

    uint32_t    fifoBufferSize[NUM_CHANNELS];   // (0x00000180 - 0x000001BC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//  External PLL CTRL (0x1C0 - 0x1FC)
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	u32 	UseOurTxCLK;						// (0x000001C0)
	u32		DacRange;							// (0x000001C4)
	u32 	EnSyncAdLatch;						// (0x000001C8) b(0):Enable synchronization, b(4):Start state machine
    u32		PSRstTriggered;						// (0x000001CC) 0=Not Triggered, 1=Triggered RST
	u32		ClearPSRstStatus;					// (0x000001D0) Write a 1 to clear PSRstTriggered status
	u32		TurnOnIIR;							// (0x000001D4) // bitmapped, 0:disable, 1:enable
	uint32_t    reserved_6[2];                  // (0x000001D8 - 0x000001DC)
	uint32_t	starti2c_rdwr;				    // (0x000001E0)
	uint32_t    plli2c_rdwr_hsdvn1;			    // (0x000001E4)
	uint32_t    plli2c_rdwr_rfreqLO;			// (0x000001E8)
	uint32_t    plli2c_rdwr_rfreqHI;			// (0x000001EC)
	uint32_t	plldac_data;					// (0x000001F0)
	uint32_t	plldac_start;				    // (0x000001F4)
	uint32_t    PllFreqMeasTime;			    // (0x000001F8)
	uint32_t    PllFreqMeas;	   				// (0x000001FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  Post IIR Filter Cutoff Coefficients (0x200 - 0x3FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    struct {
             float    b2;
             float    b1;
             float    b0;
             float    a2;
             float    a1;
             float    _reserved1;
             float    _reserved2;
             float    _reserved3;

           } filter_coeff[NUM_CHANNELS];                    // (0x00000200 - 0x000003FC)


	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// AD Multiplier (0x400 - 0x43C)
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    u32 adMultiplier[NUM_CHANNELS];	// (0x00000400-0x0000043c)

} HPS_REGS;


typedef volatile struct {

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  A/D Signal Calibration (0x000 - 0x1FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	u32    ad_offset[NUM_CHANNELS][NUM_RANGES];        // (0x00000000 - 0x000000FC)
	u32      ad_gain[NUM_CHANNELS][NUM_RANGES];        // (0x00000100 - 0x000001FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  A/D Signal Calibration (0x200 - 0x3FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	u32     bit_offset[NUM_CHANNELS][NUM_RANGES];      // (0x00000200 - 0x000002FC)
	u32    bit_gainCal[NUM_CHANNELS][NUM_RANGES];      // (0x00000300 - 0x000003FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  A/D Signal Calibration (0x400 - 0x5FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	u32    Imode_offset[NUM_CHANNELS][NUM_RANGES];      // (0x00000400 - 0x000004FC)
	u32    Imode_gain[NUM_CHANNELS][NUM_RANGES];        // (0x00000500 - 0x000005FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  A/D Signal Calibration (0x600 - 0x7FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	u32    Imode_bit_offset[NUM_CHANNELS][NUM_RANGES];  // (0x00000600 - 0x000006FC)
	u32    Imode_bit_gainCal[NUM_CHANNELS][NUM_RANGES]; // (0x00000700 - 0x000007FC)

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  A/D DAC Calibration (0x800 - 0x81C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    u32 bit_DacCal[2][NUM_RANGES];               		// (0x00000800 - 0x0000081C) [Offset/Gain][Range]
    u32	reserved_8[24];									// (0x00000820 - 0x0000087C)
    u32 sig_DacCal[2][NUM_CHANNELS];					// (0x00000880 - 0x000008FC) [Offset/Gain][Channel]


} CAL_REGS;

#endif /* __MODULE_H__ */

