/*
 * DA1 module specific definitions.
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
#if MODULE_DA1 || MODULE_DA2
    CHANNEL_5,
    CHANNEL_6,
    CHANNEL_7,
    CHANNEL_8,
    CHANNEL_9,
    CHANNEL_10,
    CHANNEL_11,
    CHANNEL_12,
#endif
#if MODULE_DA2
    CHANNEL_13,
    CHANNEL_14,
    CHANNEL_15,
    CHANNEL_16,
#endif
    NUM_CHANNELS
} CHANNEL;

#define CAL_CHANNELS 	16
#define NUM_RANGES 		4

#define MAX_D2_ERR	131
#define D2SAMPLEBITS 		10
#define D2SAMPLECNT 		1<<D2SAMPLEBITS

#define D3SAMPLEBITS 		10
#define D3SAMPLECNT 		1<<D3SAMPLEBITS

#define BIT_DELAY			0x40000


/*
 * Insert module specific definitions here...
 */

typedef volatile struct {

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  MISC Controls (0x000 - 0x01C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Using constant 16 since
	u32	SetDAData[NUM_CHANNELS];				    // (0x00000000)
	u32 reserved1[20];
	u32 PolAndRange[NUM_CHANNELS/2];				// (0x00000004)
	u32 reserved2[10];
	u32 CapSelect[NUM_CHANNELS/2];
	u32 reserved3[10];
	u32 WrapReading[NUM_CHANNELS];
	u32 reserved4[20];
	u32 CurrentReading[NUM_CHANNELS];
	u32 reserved5[20];
	u32 OutputDataTrig[NUM_CHANNELS];
	u32 reserved6[20];

	u32 VIMode;
	u32 DAResetToZero;
	u32 DARetryOverload;
	u32 DAResetOverload;
	u32 OverCurrOverride;
	u32 DASampRate;
	u32 PllFreqMeas;
	u32 SoftwareTrigger;

	u32 reserved7[22];

	u32 EnableExtAdChanSel;
	u32 ExteAdChan;

	u32 EnableCalMode;
	u32 CalAddress;
	u32 CalData;
	u32 ResetCalData;



} HOST_REGS;

typedef volatile struct {

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  MISC Controls (0x000 - 0x01C)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Using constant 16 since
	u32	starti2c_rdwr;				    // (0x00000000)
	u32 plli2c_rdwr_hsdvn1;				// (0x00000004)
	u32 plli2c_rdwr_rfreqLO;			// (0x00000008)
	u32 plli2c_rdwr_rfreqHI;			// (0x0000000C)
	u32	plldac_data;					// (0x00000010)
	u32	plldac_start;				    // (0x00000014)
	u32 PllFreqMeasTime;			    // (0x00000018)
	u32 PllFreqMeas;	   				// (0x0000001C)

	u32 BitStatus;						// (0x00000020)
	u32 D3Select;						// (0x00000024)
	u32 PowerEn;						// (0x00000028)
	u32 UseOurTxCLK;					// (0x0000002C)
	u32 ForceSpanUpdate;				// (0x00000030)
	u32 reserved1[3];					// (0x00000034-3C)
	u32 BitData[NUM_CHANNELS];			// (0x00000040-6C)
	u32 reserved2[4];					// (0x00000070-7C)
	u32 D3TestWord[NUM_CHANNELS];		// (0x00000080-B0)


} HPS_REGS;

typedef volatile struct {

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  A/D Signal Calibration (0x000 - 0x7FC)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Using constant 16 since
	u32	dacCal_VOffs_uni[NUM_RANGES][CAL_CHANNELS];		// (0x00000000 - 0x000000FC)
	u32	dacCal_VGain_uni[NUM_RANGES][CAL_CHANNELS];		// (0x00000100 - 0x000001FC)
	u32	dacCal_VOffs_bip[NUM_RANGES][CAL_CHANNELS];		// (0x00000200 - 0x000002FC)
	u32	dacCal_VGain_bip[NUM_RANGES][CAL_CHANNELS];		// (0x00000300 - 0x000003FC)

	u32	dacCal_IOffs_uni[NUM_RANGES][CAL_CHANNELS];		// (0x00000400 - 0x000004FC)
	u32	dacCal_IGain_uni[NUM_RANGES][CAL_CHANNELS];		// (0x00000500 - 0x000005FC)
	u32	dacCal_IOffs_bip[NUM_RANGES][CAL_CHANNELS];		// (0x00000600 - 0x000006FC)
	u32	dacCal_IGain_bip[NUM_RANGES][CAL_CHANNELS];		// (0x00000700 - 0x000007FC)

	u32	adCal_Offs_uni[NUM_RANGES][CAL_CHANNELS];		// (0x00000800 - 0x000008FC)
	u32	adCal_Gain_uni[NUM_RANGES][CAL_CHANNELS];		// (0x00000900 - 0x000009FC)
	u32	adCal_Offs_bip[NUM_RANGES][CAL_CHANNELS];		// (0x00000A00 - 0x00000AFC)
	u32	adCal_Gain_bip[NUM_RANGES][CAL_CHANNELS];		// (0x00000B00 - 0x00000BFC)

	u32	adCurrCal_Offs[CAL_CHANNELS];		// (0x00000C00 - 0x00000C3C)
	u32	adCurrCal_Gain[CAL_CHANNELS];		// (0x00000C40 - 0x00000C7C)



} CAL_REGS;


#endif /* __MODULE_H__ */

