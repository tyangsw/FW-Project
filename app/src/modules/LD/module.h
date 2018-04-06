/*
 * LDx/SDx module specific definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __MODULE_H__
#define __MODULE_H__

/*
 * Module memory layout
 */
#define MODULE_MEM_MAP_REV  1

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

//------------------------------------------------
// 				Defines
//------------------------------------------------
#define MAX_CHAN		        	4
#define	RESOLVER 					0
#define	FOUR_WIRE					1
#define	TWO_WIRE 					2
#define	SYNCHRO 					3

#define	FILTER_LOW					0
#define	FILTER_HIGH					1

#define NUM_OF_SIG_FILT_COEF		34		// Default for LoFreq units (FPass:4500Hz   FStop:7500Hz  	APass 0.5db 	AStop 70db)
#define NUM_OF_BIT_FILT_COEF		686		// Default for LoFreq units (FStop1:7310Hz  FPass1:7510Hz  	FPass2:7610Hz  	FStop2:7810Hz	APass 1.0db	AStop 109db)

//------------------------------------------------
// 				Define scales
//------------------------------------------------
#if MODULE_SD5 || MODULE_LD5
	#define	SIG_RMS_SCALE			1.0394305699891564137291639029175e-4    // This was the scale as of 2015-06-04
	#define MAX_SIG_HI_VLL			90
	#define MAX_SIG_MULT			3.53573		// This is to stop multiplier from getting to large with a small signal applied. (minimum voltage s/b 28Vrms which would be 3.2143 (I added 10%))
	#define BIT_SIG_GAIN_MULT		0x41900000	// Float representation of 18  (DACs at full scale)
	#define BIT_REF_GAIN_MULT		0x42340000	// Float representation of 45  (DACs at full scale)
    #define	BIT_DAC_FREQ			0x03F6AF9F	// default to 7.560Khz
	#define	BIT_DAC_SCALE			0x20000000	// FULL Scale
#else
	#define	SIG_RMS_SCALE			3.1348165490787384589868572781544e-5 		// With LPF (Fpass:4500, Fstop:7500, Apass:-0.5db, Astop:-70db 40Taps) (@400Hz amplitude is -0.2143872db or 0.97562 of signal)
	#define MAX_SIG_HI_VLL	   		28
	#define MAX_SIG_MULT			33.000		// This is to stop multiplier from getting to large with a small signal applied. (minimum voltage s/b 1Vrms which would be 30(I added 10%))
	#define BIT_SIG_GAIN_MULT		0x413D0790	// Float representation of  11.814345991561181  (sig volt is at 2.37VLL)
	#define BIT_REF_GAIN_MULT		0x4314F6B7	// Float representation of 148.963730569948186  (ref volt is at 19.30Vrms * 25 for synth ref)
    #define	BIT_DAC_FREQ			0x03F6AF9F	//	default to 7.560Khz
	#define	BIT_DAC_SCALE			0x20000000	// ************* Need to address this **************
#endif
	#define MAX_REF_HI_VRMS			115
	#define MAX_REF_LO_VRMS			28
	#define MAX_REF_MULT			126.5		// This is to stop multiplier from getting to large with a small signal applied. (minimum voltage s/b 1Vrms which would be 115 (I added 10%))

	// Set BIT Bandwidth to 100Hz
	#define	BIT_INTEGRATOR_VALUE	0x41098000	//    8.59375000000
	#define	BIT_PROPORTIONAL_VALUE	0x43BB8000	//  375.00000000000
	#define	BIT_FILTER_COEF			0xBF5DB2BB	//   -0.866008466898807
	#define	BIT_FILTER_GAIN			0x3D930F49	//    0.0718064979222085

#define	REF_LO_RMS_SCALE				6.1015885119389985576190455410107e-6	// With LPF (Fpass:4500, Fstop:7500, Apass:-0.5db, Astop:-70db 40Taps) (@400Hz amplitude is -0.2143872db or 0.97562 of signal)
#define	REF_HI_RMS_SCALE				2.3091919902868718858710088169992e-5	// With LPF (Fpass:4500, Fstop:7500, Apass:-0.5db, Astop:-70db 40Taps) (@400Hz amplitude is -0.2143872db or 0.97562 of signal)

#define	REF_LO2HI_THRESHOLD				2.23528154e+17
#define	REF_HI2LO_THRESHOLD				1.04520155e+16
#define VELOCITY_SCALE		 			6.2550105174983038972205651023392e+2
#define BKGND_CAL_AVG_TIME				32768				// average for about 671 ms   		(20.48us time interval)
#define BKGND_CAL_WAIT_TIME				24414				// wait    for about 500ms second 	(20.48us time interval)
#define D0_BIT_CAL_AVG_TIME				32768				// average for about 671 ms   		(20.48us time interval)
#define D0_BIT_CAL_WAIT_TIME			24414				// wait    for about 500ms second 	(20.48us time interval)
#define D0_BIT_FREQ_WAIT_TIME			48828				// wait    for about 1 seconds	 	(20.48us time interval)
#define D3_BIT_CAL_AVG_TIME				32768				// average for about 671 ms   		(20.48us time interval)
#define D3_BIT_CAL_WAIT_TIME			24414				// wait    for about 500ms second 	(20.48us time interval)
#define D3_BIT_FREQ_WAIT_TIME			97656				// wait    for about 2 seconds	 	(20.48us time interval)
#define LOSS_DETECT_WAITTIME			24414				// wait    for about 500ms second	(20.48us time interval)
#define FILTER_DETECT_WAITTIME			97656				// wait    for about 2 seconds   	(20.48us time interval)


//------------------------------------------------
// 				Default values
//------------------------------------------------
#define	DEFAULT_BANDWIDTH				40
#define DEFAULT_BANDWIDTH_SEL			0
#define DEFAULT_MULTISPEED_RATIO		1
#define DEFAULT_DELTA_ANG				0x00000000
#if MODULE_SD5 || MODULE_LD5
	#define	DEFAULT_SIG_LOSS_THRESHOLD 	6.30e+3	// 63.00 VLL	(70% of  90 VLL)
	#define DEFAULT_REF_LOSS_THRESHOLD	8.05e+3	// 80.50 Ref	(70% of 115 VRef)
#else
	#define	DEFAULT_SIG_LOSS_THRESHOLD	8.26e+2	//  8.26 VLL	(70% of 11.8 VLL)
	#define DEFAULT_REF_LOSS_THRESHOLD	1.82e+3	// 18.20 Ref	(70% of 26.0 VRef)
#endif
#if MODULE_LD1 || MODULE_LD2 || MODULE_LD3 || MODULE_LD4 || MODULE_LD5
	#define	DEFAULT_CHAN_MODE		FOUR_WIRE
#else
	#define	DEFAULT_CHAN_MODE		RESOLVER
#endif
#define	DEFAULT_LVDT_SCALE				0x7FFFFFFF
//------------------------------------------------
// 					MISC
//------------------------------------------------
#define MAX_SAMPLES		4096
#define ANGLE_LSB32		8.381903171539306640625e-8
#define POSITION_LSB32	2.3283064365386962890625e-8
typedef enum
{
    CHANNEL_1 = 0,
    CHANNEL_2,
    CHANNEL_3,
    CHANNEL_4,
    NUM_CHANNELS
} CHANNEL;

//--------------------------------------------
//			FUNCTION PROTOTYPES
//--------------------------------------------
extern void WaitForDataReady(void);
extern void CheckBandwidth(void);
extern void CheckMultispeedRatio(void);
extern void CheckDeltaAngle(void);
extern void CheckSigThreshold(void);
extern void CheckRefThreshold(void);
extern void CheckChanModeSelect(void);
extern void CheckLVDTScale(void);
extern void CheckBITMode(void);
extern void CheckFIFOCriteria(void);
extern void Calculate_RMSValues(void);
extern void Calculate_GainValues(void);
extern void Calculate_BkgndDCCal(void);
extern void Scale_VelocityOut(void);
extern void CheckRefVoltIn(void);
extern void CheckSigLoss(void);
extern void CheckRefLoss(void);
extern void CheckFilterSelection(void);
extern void CheckTroubleshootReg(void);
extern void CheckBkgndDCRegs(void);
extern void EndOfFunctions(void);

extern void SetBkgndBitChannel(void);
extern void WaitFor_BitFreqSettled(void);
extern void SetBkgndBitAngle(void);
extern void WaitForAngleSettled(void);
extern void GetBkgndBitAngle(void);
extern void UpdateBkgndBitStatus(void);
extern void EndOfBITFunctions(void);

extern void Init_D0_BitMode(void);
extern void Set_D0_BitAngle(void);
extern void WaitFor_D0_FreqSettled(void);
extern void WaitFor_D0_AngleSettled(void);
extern void Get_D0_BitAngle(void);
extern void Update_D0_BitStatus(void);
extern void EndOf_D0_BITFunctions(void);

extern void Init_D3_BitMode(void);
extern void Set_D3_BitAngle(void);
extern void WaitFor_D3_FreqSettled(void);
extern void WaitFor_D3_AngleSettled(void);
extern void Get_D3_BitAngle(void);
extern void Update_D3_BitStatus(void);
extern void EndOf_D3_BITFunctions(void);


//--------------------------------------------
//			GLOBAL VARIABLES
//--------------------------------------------
char	Tempbuf[256];
u32		FunctionIndex;
u32		BitFunctionIndex;
u32		D0_BitFunctionIndex;
u32		D3_BitFunctionIndex;
u32		ChanIndex;
u32		InBITMode;
u32  	WaitTimeBeforeSigLossDetect;
u32  	WaitTimeBeforeRefLossDetect;
u16		RMSFreqIndex[MAX_CHAN];
u16		UserChanMode[MAX_CHAN];
//----------------------------------
//	D0 Bit Mode(TEST Mode)
//----------------------------------
float	D0BitAngleExpected;
float	D0BitAngleActual[MAX_CHAN];
float	D0BitAngleError[MAX_CHAN];
double	D0BitAngleAccumDouble[MAX_CHAN];
s64		D0BitAngleAccumSigned[MAX_CHAN];
float	MaxD0BitError[MAX_CHAN];
u32		D0BitWaitTime;
u32		D0BitFreqWaitTime;
u32		AverageD0BitAngleIndex;
u16		D0BitFailed[MAX_CHAN];
bool    LeftD0Tests;
bool	InD0Tests;

//----------------------------------
//	D2 Bit Mode(Background BIT)
//----------------------------------
float	BkgndBitAngleExpected;
float	BkgndBitAngleActual;
float	BkgndBitAngleError;
u64		BkgndBitAngleAccumulated;
float	MaxBkgndBitError;
u32		BkgndBitWaitTime;
u32		AverageBkgndBitAngleIndex;
u16		BitFailed[MAX_CHAN];
u8		FilterState[MAX_CHAN];
u32     BkgndBitChanIndex;
//----------------------------------
//			D3 Bit Mode
//----------------------------------
float	D3BitAngleExpected;
float	D3BitAngleActual[MAX_CHAN];
float	D3BitAngleError[MAX_CHAN];
u64		D3BitAngleAccumulated[MAX_CHAN];
float	MaxD3BitError[MAX_CHAN];
u32		D3BitWaitTime;
u32		D3BitFreqWaitTime;
u32		AverageD3BitAngleIndex;
u16		D3BitFailed[MAX_CHAN];
bool    LeftD3Tests;
bool	InD3Tests;
//----------------------------------
//		RMS , FREQ Calculations
//----------------------------------
u32		Frequency[MAX_CHAN];
u32		RefFrequency[MAX_CHAN];
u32		PrevRefFrequency[MAX_CHAN];
u32		UserFrequency[MAX_CHAN];
float	Sig_SumOfSq[MAX_CHAN];
float	Ref_SumOfSq[MAX_CHAN];
u32		Cycle_SampleCount[MAX_CHAN];
float	SigGainMult[MAX_CHAN];
float	RefGainMult[MAX_CHAN];
float	SynthRefGainMult[MAX_CHAN];
float	Sig_RMS[MAX_CHAN];
float	Ref_RMS[MAX_CHAN];
u16		RefSelState[MAX_CHAN];
s32		VelocityFPGA[MAX_CHAN];

// Troubleshooting variables
u32		DataValue[8][MAX_SAMPLES];
float   Integrator[2048];


//--------------------------------------------
//			USER ADDRESSES (HOST)
//--------------------------------------------
#define DPRAM_BIT_MODE							0x40000248
#define DPRAM_D2_VERIFY							0x4000024C
#define DPRAM_BKGND_CAL_CHAN					0x40000258
#define DPRAM_CALIBRATION_ENABLE				0x40000284
#define DPRAM_CALIBRATION_ADDRESS				0x40000288
#define DPRAM_CALIBRATION_DATA					0x4000028C
#define DPRAM_CALIBRATION_READ_WRT				0x40000290
#define DPRAM_BIT_ANGLE							0x40000294

// STATUS (0)
#define DPRAM_BIT_STATUS_DYNAMIC				0x40000800
#define DPRAM_BIT_STATUS_LATCHED				0x40000804
#define DPRAM_BIT_STATUS_MASK					0x40000808
#define DPRAM_BIT_STATUS_EDGE_LEVEL				0x4000080C
// STATUS (1)
#define DPRAM_SIG_LOSS_STATUS_DYNAMIC			0x40000810
#define DPRAM_SIG_LOSS_STATUS_LATCHED			0x40000814
#define DPRAM_SIG_LOSS_STATUS_MASK				0x40000818
#define DPRAM_SIG_LOSS_STATUS_EDGE_LEVEL		0x4000081C
// STATUS (2)
#define DPRAM_REF_LOSS_STATUS_DYNAMIC			0x40000820
#define DPRAM_REF_LOSS_STATUS_LATCHED			0x40000824
#define DPRAM_REF_LOSS_STATUS_MASK				0x40000828
#define DPRAM_REF_LOSS_STATUS_EDGE_LEVEL		0x4000082C
// STATUS (3)
#define DPRAM_LOCK_LOSS_STATUS_DYNAMIC			0x40000830
#define DPRAM_LOCK_LOSS_STATUS_LATCHED			0x40000834
#define DPRAM_LOCK_LOSS_STATUS_MASK				0x40000838
#define DPRAM_LOCK_LOSS_STATUS_EDGE_LEVEL		0x4000083C
// STATUS (4)
#define DPRAM_DELTA_ANG_STATUS_DYNAMIC			0x40000840
#define DPRAM_DELTA_ANG_STATUS_LATCHED			0x40000844
#define DPRAM_DELTA_ANG_STATUS_MASK				0x40000848
#define DPRAM_DELTA_ANG_STATUS_EDGE_LEVEL		0x4000084C
// STATUS (31)
#define DPRAM_TEST_STATUS_DYNAMIC				0x400009F0
#define DPRAM_TEST_STATUS_LATCHED				0x400009F4
#define DPRAM_TEST_STATUS_MASK					0x400009F8
#define DPRAM_TEST_STATUS_EDGE_LEVEL			0x400009FC


#define DPRAM_ANGLE_CH1							0x40001000
#define DPRAM_VELOCITY_CH1						0x40001004
// unused 0x40001008
#define DPRAM_BANDWIDTH_CH1						0x4000100C
#define DPRAM_BAND_SEL_CH1						0x40001010
// unused 0x40001014
#define DPRAM_DELTA_ANG_CH1						0x40001018
#define DPRAM_INIT_ANG_CH1						0x4000101C
// unused 0x40001020
#define DPRAM_MEAS_REF_CH1						0x40001024
#define DPRAM_MEAS_SIG_CH1						0x40001028
#define DPRAM_MEAS_FREQ_CH1						0x4000102C
#define DPRAM_SIG_LOSS_THRESHOLD_CH1			0x40001030
#define DPRAM_REF_LOSS_THRESHOLD_CH1			0x40001034
#define DPRAM_MODE_SEL_CH1						0x40001038
#define DPRAM_LVDT_SCALE_CH1					0x4000103C
#define DPRAM_SIN_RMS_CH1						0x40001040
#define DPRAM_COS_RMS_CH1						0x40001044
// unused 0x40001048 - 0x40001048
#define DPRAM_AVG_ANGLE_CH1						0x4000104C
//--------------------------------------------
#define DPRAM_ANGLE_CH2							0x40001050
#define DPRAM_VELOCITY_CH2						0x40001054
// unused 0x40001058
#define DPRAM_BANDWIDTH_CH2						0x4000105C
#define DPRAM_BAND_SEL_CH2						0x40001060
#define DPRAM_RATIO_CH1_2						0x40001064
#define DPRAM_DELTA_ANG_CH2						0x40001068
#define DPRAM_INIT_ANG_CH2						0x4000106C
#define DPRAM_COMBINED_ANGLE_CH1_2				0x40001070
#define DPRAM_MEAS_REF_CH2						0x40001074
#define DPRAM_MEAS_SIG_CH2						0x40001078
#define DPRAM_MEAS_FREQ_CH2						0x4000107C
#define DPRAM_SIG_LOSS_THRESHOLD_CH2			0x40001080
#define DPRAM_REF_LOSS_THRESHOLD_CH2			0x40001084
#define DPRAM_MODE_SEL_CH2						0x40001088
#define DPRAM_LVDT_SCALE_CH2					0x4000108C
#define DPRAM_SIN_RMS_CH2						0x40001090
#define DPRAM_COS_RMS_CH2						0x40001094
// unused 0x40001098 - 0x40001098
#define DPRAM_AVG_ANGLE_CH2						0x4000109C
//--------------------------------------------
#define DPRAM_ANGLE_CH3							0x400010A0
#define DPRAM_VELOCITY_CH3						0x400010A4
// unused 0x400010A8
#define DPRAM_BANDWIDTH_CH3						0x400010AC
#define DPRAM_BAND_SEL_CH3						0x400010B0
// unused 0x400010B4
#define DPRAM_DELTA_ANG_CH3						0x400010B8
#define DPRAM_INIT_ANG_CH3						0x400010BC
// unused 0x400010C0
#define DPRAM_MEAS_REF_CH3						0x400010C4
#define DPRAM_MEAS_SIG_CH3						0x400010C8
#define DPRAM_MEAS_FREQ_CH3						0x400010CC
#define DPRAM_SIG_LOSS_THRESHOLD_CH3			0x400010D0
#define DPRAM_REF_LOSS_THRESHOLD_CH3			0x400010D4
#define DPRAM_MODE_SEL_CH3						0x400010D8
#define DPRAM_LVDT_SCALE_CH3					0x400010DC
#define DPRAM_SIN_RMS_CH3						0x400010E0
#define DPRAM_COS_RMS_CH3						0x400010E4
// unused 0x400010E8 - 0x400010E8
#define DPRAM_AVG_ANGLE_CH3						0x400010EC
//--------------------------------------------
#define DPRAM_ANGLE_CH4							0x400010F0
#define DPRAM_VELOCITY_CH4						0x400010F4
// unused 0x400010F8
#define DPRAM_BANDWIDTH_CH4						0x400010FC
#define DPRAM_BAND_SEL_CH4						0x40001100
#define DPRAM_RATIO_CH3_4						0x40001104
#define DPRAM_DELTA_ANG_CH4						0x40001108
#define DPRAM_INIT_ANG_CH4						0x4000110C
#define DPRAM_COMBINED_ANGLE_CH3_4				0x40001110
#define DPRAM_MEAS_REF_CH4						0x40001114
#define DPRAM_MEAS_SIG_CH4						0x40001118
#define DPRAM_MEAS_FREQ_CH4						0x4000111C
#define DPRAM_SIG_LOSS_THRESHOLD_CH4			0x40001120
#define DPRAM_REF_LOSS_THRESHOLD_CH4			0x40001124
#define DPRAM_MODE_SEL_CH4						0x40001128
#define DPRAM_LVDT_SCALE_CH4					0x4000112C
#define DPRAM_SIN_RMS_CH4						0x40001130
#define DPRAM_COS_RMS_CH4						0x40001134
// unused 0x40001138 - 0x40001138
#define DPRAM_AVG_ANGLE_CH4						0x4000113C
//--------------------------------------------
#define DPRAM_BKGND_BIT_VALUE					0x400011F4
#define DPRAM_BKGND_BIT_EXPECTED				0x400011F8
#define DPRAM_BKGND_BIT_ACTUAL					0x400011FC
//--------------------------------------------
//			FIFO ADDRESSES
//--------------------------------------------
#define DPRAM_FIFO_DATA_CH1						0x40001200
#define DPRAM_FIFO_WORD_COUNT_CH1				0x40001204
#define DPRAM_FIFO_STATUS_CH1					0x40001208
#define DPRAM_FIFO_HI_THRESHOLD_CH1				0x4000120C
#define DPRAM_FIFO_LO_THRESHOLD_CH1				0x40001210
#define DPRAM_FIFO_DELAY_CH1					0x40001214
#define DPRAM_FIFO_NUM_OF_SAMPLES_CH1			0x40001218
#define DPRAM_FIFO_DATA_RATE_CH1				0x4000121C
#define DPRAM_FIFO_CLEAR_CH1					0x40001220
#define DPRAM_FIFO_DATA_TYPE_CH1				0x40001224
#define DPRAM_FIFO_TRIGGER_SRC_CH1				0x40001228
#define DPRAM_INTERRUPT_ENABLE_CH1				0x4000122C
//......................................
#define DPRAM_FIFO_DATA_CH2						0x40001240
#define DPRAM_FIFO_WORD_COUNT_CH2				0x40001244
#define DPRAM_FIFO_STATUS_CH2					0x40001248
#define DPRAM_FIFO_HI_THRESHOLD_CH2				0x4000124C
#define DPRAM_FIFO_LO_THRESHOLD_CH2				0x40001250
#define DPRAM_FIFO_DELAY_CH2					0x40001254
#define DPRAM_FIFO_NUM_OF_SAMPLES_CH2			0x40001258
#define DPRAM_FIFO_DATA_RATE_CH2				0x4000125C
#define DPRAM_FIFO_CLEAR_CH2					0x40001260
#define DPRAM_FIFO_DATA_TYPE_CH2				0x40001264
#define DPRAM_FIFO_TRIGGER_SRC_CH2				0x40001268
#define DPRAM_INTERRUPT_ENABLE_CH2				0x4000126C
//......................................
#define DPRAM_FIFO_DATA_CH3						0x40001280
#define DPRAM_FIFO_WORD_COUNT_CH3				0x40001284
#define DPRAM_FIFO_STATUS_CH3					0x40001288
#define DPRAM_FIFO_HI_THRESHOLD_CH3				0x4000128C
#define DPRAM_FIFO_LO_THRESHOLD_CH3				0x40001290
#define DPRAM_FIFO_DELAY_CH3					0x40001294
#define DPRAM_FIFO_NUM_OF_SAMPLES_CH3			0x40001298
#define DPRAM_FIFO_DATA_RATE_CH3				0x4000129C
#define DPRAM_FIFO_CLEAR_CH3					0x400012A0
#define DPRAM_FIFO_DATA_TYPE_CH3				0x400012A4
#define DPRAM_FIFO_TRIGGER_SRC_CH3				0x400012A8
#define DPRAM_INTERRUPT_ENABLE_CH3				0x400012AC
//......................................
#define DPRAM_FIFO_DATA_CH4						0x400012C0
#define DPRAM_FIFO_WORD_COUNT_CH4				0x400012C4
#define DPRAM_FIFO_STATUS_CH4					0x400012C8
#define DPRAM_FIFO_HI_THRESHOLD_CH4				0x400012CC
#define DPRAM_FIFO_LO_THRESHOLD_CH4				0x400012D0
#define DPRAM_FIFO_DELAY_CH4					0x400012D4
#define DPRAM_FIFO_NUM_OF_SAMPLES_CH4			0x400012D8
#define DPRAM_FIFO_DATA_RATE_CH4				0x400012DC
#define DPRAM_FIFO_CLEAR_CH4					0x400012E0
#define DPRAM_FIFO_DATA_TYPE_CH4				0x400012E4
#define DPRAM_FIFO_TRIGGER_SRC_CH4				0x400012E8
#define DPRAM_INTERRUPT_ENABLE_CH4				0x400012EC
//......................................
#define DPRAM_FIFO_SOFTWARE_TRIGGER				0x40001300
//......................................

//--------------------------------------------
//			DEBUGGING ADDRESSES
//--------------------------------------------
#define DPRAM_OFFSET_SELECT						0x40003F00
#define DPRAM_FORCE_SYNTH_REF					0x40003F04
#define DPRAM_FORCE_EXT_REF						0x40003F08
// unused 0x40003F0C
#define DPRAM_FREEZE_NCO						0x40003F10
// unused 0x40003F14 - 0x40003F18
#define DPRAM_FREEZE_PHACCUM					0x40003F1C
#define DPRAM_USER_CLR_INTEGRATOR				0x40003F20
#define DPRAM_INTEGRATOR_SATURATED				0x40003F24
#define DPRAM_DISABLE_LPF						0x40003F28
#define DPRAM_DISABLE_BKGND_DC_OFFSET			0x40003F2C
#define DPRAM_BKGND_DC_SIN_CH1					0x40003F30
#define DPRAM_BKGND_DC_COS_CH1					0x40003F34
#define DPRAM_BKGND_DC_SIN_CH2					0x40003F38
#define DPRAM_BKGND_DC_COS_CH2					0x40003F3C
#define DPRAM_BKGND_DC_SIN_CH3					0x40003F40
#define DPRAM_BKGND_DC_COS_CH3					0x40003F44
#define DPRAM_BKGND_DC_SIN_CH4					0x40003F48
#define DPRAM_BKGND_DC_COS_CH4					0x40003F4C
#define DPRAM_DETECT_RMS_SIN_CH1				0x40003F50
#define DPRAM_DETECT_RMS_COS_CH1				0x40003F54
#define DPRAM_DETECT_RMS_SIN_CH2				0x40003F58
#define DPRAM_DETECT_RMS_COS_CH2				0x40003F5C
#define DPRAM_DETECT_RMS_SIN_CH3				0x40003F60
#define DPRAM_DETECT_RMS_COS_CH3				0x40003F64
#define DPRAM_DETECT_RMS_SIN_CH4				0x40003F68
#define DPRAM_DETECT_RMS_COS_CH4				0x40003F6C



// unused 0x40003F34 - 0x40003F5C
//--------------------------------------------

#define DPRAM_FUNCTION_SELECT					0x40003F60
#define DPRAM_MAX_DELAY							0x40003F64
#define DPRAM_CHAN_SELECT						0x40003F68
// unused 0x40003F6C - 0x40003FDC
// unused 0x40003FF0 - 0x40003FF8
#define DPRAM_INTERNAL_REG_1					0x40003FE0
#define DPRAM_INTERNAL_REG_2					0x40003FE4
#define DPRAM_INTERNAL_REG_3					0x40003FE8
#define DPRAM_INTERNAL_REG_4					0x40003FEC

#define DPRAM_INTERNAL_REV						0x40003FFC


//--------------------------------------------
//			HARDWARE ADDRESSES
//--------------------------------------------
#define FPGA_VELOCITY_CH1				0x600000B0
#define FPGA_VELOCITY_CH2				0x600000B4
#define FPGA_VELOCITY_CH3				0x600000B8
#define FPGA_VELOCITY_CH4				0x600000BC
#define FPGA_REF_SELECT					0x600000C0
// UNSUSED 0x00C4 - 0x00CC
#define FPGA_BIT_STATUS					0x600000D0
#define FPGA_SIG_LOSS_STATUS			0x600000D4
#define FPGA_REF_LOSS_STATUS			0x600000D8
// 0x600000DC (LOCK LOSS) Read from separate circuit
#define FPGA_DELTA_ANG_STATUS			0x600000E0
// UNSUSED 0x00E4 - 0x00EC
#define FPGA_CLEAR_LOCK_LOSS			0x600000F0
#define FPGA_CLEAR_PS_FAULT				0x600000F4
#define FPGA_SM_DONE					0x600000F8
#define FPGA_CLR_DONE					0x600000FC
// UNSUSED 0x0100 - 0x01FC
#define FPGA_DAC_CONFIG_MODE			0x60000200
#define FPGA_DAC_CONFIG_WORD			0x60000204
// UNSUSED 0x0208
#define FPGA_ANGLE_VALUE_BIT			0x6000020C
// UNSUSED 0x0210
#define FPGA_IN_BIT_MODE				0x60000214
#define FPGA_BIT_DAC_GAIN_MULTIPLIER	0x60000218
#define FPGA_DISABLE_FILTERS			0x6000021C
#define FPGA_BIT_CHAN_SELECT			0x60000220
#define FPGA_USER_BIT_ANGLE				0x60000224
#define FPGA_ARM_BIT_ANGLE				0x60000228
// UNSUSED 0x022C
#define FPGA_BIT_FREQ_CH1				0x60000230
#define FPGA_BIT_FREQ_CH2				0x60000234
#define FPGA_BIT_FREQ_CH3				0x60000238
#define FPGA_BIT_FREQ_CH4				0x6000023C
#define FPGA_DAC_SIN_GAIN_MULT_CH1		0x60000240
#define FPGA_DAC_SIN_GAIN_MULT_CH2		0x60000244
#define FPGA_DAC_SIN_GAIN_MULT_CH3		0x60000248
#define FPGA_DAC_SIN_GAIN_MULT_CH4		0x6000024C
#define FPGA_RESET_FILTERS_CH1			0x60000250
#define FPGA_RESET_FILTERS_CH2			0x60000254
#define FPGA_RESET_FILTERS_CH3			0x60000258
#define FPGA_RESET_FILTERS_CH4			0x6000025C
#define FPGA_BitFiltTapCountCh1			0x60000260
#define FPGA_BitFiltTapCountCh2			0x60000264
#define FPGA_BitFiltTapCountCh3			0x60000268
#define FPGA_BitFiltTapCountCh4			0x6000026C
#define FPGA_SigFiltTapCountCh1			0x60000270
#define FPGA_SigFiltTapCountCh2			0x60000274
#define FPGA_SigFiltTapCountCh3			0x60000278
#define FPGA_SigFiltTapCountCh4			0x6000027C

//...............................................................................................
// 										FIFOS
//...............................................................................................
#define FPGA_FifoEmpty_CH1				0x600002C0
#define FPGA_FifoEmpty_CH2				0x600002C4
#define FPGA_FifoEmpty_CH3				0x600002C8
#define FPGA_FifoEmpty_CH4				0x600002CC
#define FPGA_FifoFull_CH1				0x600002D0
#define FPGA_FifoFull_CH2				0x600002D4
#define FPGA_FifoFull_CH3				0x600002D8
#define FPGA_FifoFull_CH4				0x600002DC
#define FPGA_InFifoCount_CH1			0x600002E0
#define FPGA_InFifoCount_CH2			0x600002E4
#define FPGA_InFifoCount_CH3			0x600002E8
#define FPGA_InFifoCount_CH4			0x600002EC
#define FPGA_FifoStatus_CH1				0x600002F0
#define FPGA_FifoStatus_CH2				0x600002F4
#define FPGA_FifoStatus_CH3				0x600002F8
#define FPGA_FifoStatus_CH4				0x600002FC
#define FPGA_RdData_CH1					0x60000300
#define FPGA_RdData_CH2					0x60000304
#define FPGA_RdData_CH3					0x60000308
#define FPGA_RdData_CH4					0x6000030C
#define FPGA_TotalCount_CH1				0x60000310
#define FPGA_TotalCount_CH2				0x60000314
#define FPGA_TotalCount_CH3				0x60000318
#define FPGA_TotalCount_CH4				0x6000031C
#define FPGA_FIFO_DDR_START_ADDRS_CH1	0x60000320
#define FPGA_FIFO_DDR_START_ADDRS_CH2	0x60000324
#define FPGA_FIFO_DDR_START_ADDRS_CH3	0x60000328
#define FPGA_FIFO_DDR_START_ADDRS_CH4  	0x6000032C
#define FPGA_FIFO_FULL_VALUE_CH1 		0x60000330
#define FPGA_FIFO_FULL_VALUE_CH2		0x60000334
#define FPGA_FIFO_FULL_VALUE_CH3		0x60000338
#define FPGA_FIFO_FULL_VALUE_CH4		0x6000033C
#define FPGA_ENABLE_FIFOS				0x60000340
#define FPGA_RESET_FIFO_CTRL			0x60000344
// UNSUSED 0x0348 - 0x034C
//...............................................................................................
// 								TROUBLESHOOTING
//...............................................................................................
#define	FPGA_INTERNAL_REG_1				0x60000350
#define	FPGA_INTERNAL_REG_2				0x60000354
#define	FPGA_INTERNAL_REG_3				0x60000358
#define	FPGA_INTERNAL_REG_4				0x6000035C

// UNSUSED 0x03D0 - 0x03D8
#define FPGA_BIT_FREQ_TROUBLESHOOT		0x600003DC
#define FPGA_TROUBLESHOOT_DATABUS1		0x600003E0
#define FPGA_TROUBLESHOOT_DATABUS2		0x600003E4
#define FPGA_TROUBLESHOOT_DATABIT1		0x600003E8
#define FPGA_TROUBLESHOOT_DATABIT2		0x600003EC
#define FPGA_RD_FIFO_DATA1				0x600003F0
#define FPGA_RD_FIFO_DATA2				0x600003F4
#define FPGA_ENABLE_FIFO_DATA			0x600003F8
//...............................................................................................
// 					BW_COEF_RAM	(0x0400)
//...............................................................................................
#define FPGA_COEF_INTEGRATOR_CH1		0x60000400
#define FPGA_COEF_INTEGRATOR_CH2		0x60000404
#define FPGA_COEF_INTEGRATOR_CH3		0x60000408
#define FPGA_COEF_INTEGRATOR_CH4		0x6000040C
#define FPGA_COEF_PROPORTIONAL_CH1		0x60000410
#define FPGA_COEF_PROPORTIONAL_CH2		0x60000414
#define FPGA_COEF_PROPORTIONAL_CH3		0x60000418
#define FPGA_COEF_PROPORTIONAL_CH4		0x6000041C
#define FPGA_GAIN_FILTER_CH1			0x60000420
#define FPGA_GAIN_FILTER_CH2			0x60000424
#define FPGA_GAIN_FILTER_CH3			0x60000428
#define FPGA_GAIN_FILTER_CH4			0x6000042C
#define FPGA_COEF_FILTER_CH1			0x60000430
#define FPGA_COEF_FILTER_CH2			0x60000434
#define FPGA_COEF_FILTER_CH3			0x60000438
#define FPGA_COEF_FILTER_CH4			0x6000043C
#define FPGA_SIG_GAIN_MULT_CH1			0x60000440
#define FPGA_SIG_GAIN_MULT_CH2			0x60000444
#define FPGA_SIG_GAIN_MULT_CH3			0x60000448
#define FPGA_SIG_GAIN_MULT_CH4			0x6000044C
#define FPGA_REF_GAIN_MULT_CH1			0x60000450
#define FPGA_REF_GAIN_MULT_CH2			0x60000454
#define FPGA_REF_GAIN_MULT_CH3			0x60000458
#define FPGA_REF_GAIN_MULT_CH4			0x6000045C
#define FPGA_SYNTH_GAIN_MULT_CH1		0x60000460
#define FPGA_SYNTH_GAIN_MULT_CH2		0x60000464
#define FPGA_SYNTH_GAIN_MULT_CH3		0x60000468
#define FPGA_SYNTH_GAIN_MULT_CH4		0x6000046C
#define FPGA_BW_COEF_INTEGRATOR_BIT		0x60000470
#define FPGA_BW_COEF_PROPORTIONAL_BIT	0x60000474
#define FPGA_BW_GAIN_FILTER_BIT			0x60000478
#define FPGA_BW_COEF_FILTER_BIT			0x6000047C
#define FPGA_SIG_GAIN_MULT_BIT			0x60000480
#define FPGA_REF_GAIN_MULT_BIT			0x60000484
#define FPGA_SYNTH_GAIN_MULT_BIT		0x60000488
//...............................................................................................
// 					RAM_SIG_AD_DATA	(0x0800)
//...............................................................................................
#define FPGA_PRE_FILTER_SIN_AD_CH1		0x60000800
#define FPGA_PRE_FILTER_COS_AD_CH1		0x60000804
#define FPGA_PRE_FILTER_SIN_AD_CH2		0x60000808
#define FPGA_PRE_FILTER_COS_AD_CH2		0x6000080C
#define FPGA_PRE_FILTER_SIN_AD_CH3		0x60000810
#define FPGA_PRE_FILTER_COS_AD_CH3		0x60000814
#define FPGA_PRE_FILTER_SIN_AD_CH4		0x60000818
#define FPGA_PRE_FILTER_COS_AD_CH4		0x6000081C
//...............................................................................................
// 					RAM_REF_AD_DATA	(0x0C00)
//...............................................................................................
#define FPGA_PRE_FILTER_REF_LV_AD_CH1	0x60000C00
#define FPGA_PRE_FILTER_REF_LV_AD_CH2	0x60000C04
#define FPGA_PRE_FILTER_REF_LV_AD_CH3	0x60000C08
#define FPGA_PRE_FILTER_REF_LV_AD_CH4	0x60000C0C
#define FPGA_PRE_FILTER_REF_HV_AD_CH1	0x60000C10
#define FPGA_PRE_FILTER_REF_HV_AD_CH2	0x60000C14
#define FPGA_PRE_FILTER_REF_HV_AD_CH3	0x60000C18
#define FPGA_PRE_FILTER_REF_HV_AD_CH4	0x60000C1C
#define FPGA_PRE_FILTER_REF_AD_CH1		0x60000C20
#define FPGA_PRE_FILTER_REF_AD_CH2		0x60000C24
#define FPGA_PRE_FILTER_REF_AD_CH3		0x60000C28
#define FPGA_PRE_FILTER_REF_AD_CH4		0x60000C2C
//...............................................................................................
//					RAM_FILTERED_AD_DATA	(0x1000)
//...............................................................................................
#define FPGA_FILTERED_SIN_AD_CH1		0x60001000
#define FPGA_FILTERED_SIN_AD_CH2		0x60001004
#define FPGA_FILTERED_SIN_AD_CH3		0x60001008
#define FPGA_FILTERED_SIN_AD_CH4		0x6000100C
#define FPGA_FILTERED_COS_AD_CH1		0x60001010
#define FPGA_FILTERED_COS_AD_CH2		0x60001014
#define FPGA_FILTERED_COS_AD_CH3		0x60001018
#define FPGA_FILTERED_COS_AD_CH4		0x6000101C
#define FPGA_FILTERED_REF_AD_CH1		0x60001020
#define FPGA_FILTERED_REF_AD_CH2		0x60001024
#define FPGA_FILTERED_REF_AD_CH3		0x60001028
#define FPGA_FILTERED_REF_AD_CH4		0x6000102C
#define FPGA_SIN_AD_BKND_BIT			0x60001030
#define FPGA_COS_AD_BKND_BIT			0x60001034
#define FPGA_REF_AD_BKND_BIT			0x60001038
//...............................................................................................
// 					RAM_SCALED_AD_DATA	(0x1400)
//...............................................................................................
#define FPGA_SCALED_SIN_CH1				0x60001400
#define FPGA_SCALED_SIN_CH2				0x60001404
#define FPGA_SCALED_SIN_CH3				0x60001408
#define FPGA_SCALED_SIN_CH4				0x6000140C
#define FPGA_SCALED_COS_CH1				0x60001410
#define FPGA_SCALED_COS_CH2				0x60001414
#define FPGA_SCALED_COS_CH3				0x60001418
#define FPGA_SCALED_COS_CH4				0x6000141C
#define FPGA_SCALED_REF_CH1				0x60001420
#define FPGA_SCALED_REF_CH2				0x60001424
#define FPGA_SCALED_REF_CH3				0x60001428
#define FPGA_SCALED_REF_CH4				0x6000142C
#define FPGA_SCALED_SYNTH_REF_4W_CH1	0x60001430
#define FPGA_SCALED_SYNTH_REF_4W_CH2	0x60001434
#define FPGA_SCALED_SYNTH_REF_4W_CH3	0x60001438
#define FPGA_SCALED_SYNTH_REF_4W_CH4	0x6000143C
#define FPGA_SCALED_SYNTH_REF_2W_CH1	0x60001440
#define FPGA_SCALED_SYNTH_REF_2W_CH2	0x60001444
#define FPGA_SCALED_SYNTH_REF_2W_CH3	0x60001448
#define FPGA_SCALED_SYNTH_REF_2W_CH4	0x6000144C
#define FPGA_SCALED_SIN_AD_BIT			0x60001450
#define FPGA_SCALED_COS_AD_BIT			0x60001454
#define FPGA_SCALED_REF_AD_BIT			0x60001458
#define FPGA_SCALED_SYNTH_REF_4W_BIT	0x6000145C
#define FPGA_SCALED_SYNTH_REF_2W_BIT	0x60001460
//...............................................................................................
// 					RAM_DMOD_CALC	(0x1800)
//...............................................................................................
#define FPGA_DEMOD_ERROR_CH1			0x60001800
#define FPGA_DEMOD_ERROR_CH2			0x60001804
#define FPGA_DEMOD_ERROR_CH3			0x60001808
#define FPGA_DEMOD_ERROR_CH4			0x6000180C
#define FPGA_ACCUM_REF_PHASE_LO_CH1		0x60001810
#define FPGA_ACCUM_REF_PHASE_HI_CH1		0x60001814
#define FPGA_ACCUM_REF_PHASE_LO_CH2		0x60001818
#define FPGA_ACCUM_REF_PHASE_HI_CH2		0x6000181C
#define FPGA_ACCUM_REF_PHASE_LO_CH3		0x60001820
#define FPGA_ACCUM_REF_PHASE_HI_CH3		0x60001824
#define FPGA_ACCUM_REF_PHASE_LO_CH4		0x60001828
#define FPGA_ACCUM_REF_PHASE_HI_CH4		0x6000182C
#define FPGA_SIN_COS_ERROR_CH1			0x60001830
#define FPGA_SIN_COS_ERROR_CH2			0x60001834
#define FPGA_SIN_COS_ERROR_CH3			0x60001838
#define FPGA_SIN_COS_ERROR_CH4 			0x6000183C
#define FPGA_SYNTH_REF_CH1				0x60001840
#define FPGA_SYNTH_REF_CH2				0x60001844
#define FPGA_SYNTH_REF_CH3				0x60001848
#define FPGA_SYNTH_REF_CH4				0x6000184C
#define FPGA_SYNTH_REAL_REF_SEL_CH1		0x60001850
#define FPGA_SYNTH_REAL_REF_SEL_CH2		0x60001854
#define FPGA_SYNTH_REAL_REF_SEL_CH3		0x60001858
#define FPGA_SYNTH_REAL_REF_SEL_CH4		0x6000185C
#define FPGA_DEMOD_ERROR_BIT			0x60001860
#define FPGA_ACCUM_REF_PHASE_LO_BIT		0x60001864
#define FPGA_ACCUM_REF_PHASE_HI_BIT		0x60001868
#define FPGA_SIN_COS_ERROR_BIT			0x6000186C
#define FPGA_SYNTH_REF_BIT				0x60001870
#define FPGA_SYNTH_REAL_REF_SEL_BIT		0x60001874
//...............................................................................................
// 					RAM_PI_CALC	(0x1C00)
//...............................................................................................
#define FPGA_PI_ERROR_CH1				0x60001C00
#define FPGA_PI_ERROR_CH2				0x60001C04
#define FPGA_PI_ERROR_CH3				0x60001C08
#define FPGA_PI_ERROR_CH4				0x60001C0C
#define FPGA_INTEGRATOR_VAL_CH1			0x60001C10
#define FPGA_INTEGRATOR_VAL_CH2			0x60001C14
#define FPGA_INTEGRATOR_VAL_CH3			0x60001C18
#define FPGA_INTEGRATOR_VAL_CH4			0x60001C1C
// SKIP 0x1C20 - 0x1C2C
#define FPGA_PI_ERROR_BIT				0x60001C30
#define FPGA_INTEGRATOR_VAL_BIT			0x60001C34
// SKIP 0x1C38 - 0x1C3C
#define FPGA_MIN_INTEGRATOR_CH1			0x60001C40
#define FPGA_MIN_INTEGRATOR_CH2			0x60001C44
#define FPGA_MIN_INTEGRATOR_CH3			0x60001C48
#define FPGA_MIN_INTEGRATOR_CH4			0x60001C4C
#define FPGA_MAX_INTEGRATOR_CH1			0x60001C50
#define FPGA_MAX_INTEGRATOR_CH2			0x60001C54
#define FPGA_MAX_INTEGRATOR_CH3			0x60001C58
#define FPGA_MAX_INTEGRATOR_CH4			0x60001C5C


//...............................................................................................
// 					RAM_NCO_CALC	(0x2000)
//...............................................................................................
#define FPGA_SIN_NCO_DSP_CH1			0x60002000
#define FPGA_COS_NCO_DSP_CH1			0x60002004
#define FPGA_SIN_NCO_DSP_CH2			0x60002008
#define FPGA_COS_NCO_DSP_CH2			0x6000200C
#define FPGA_SIN_NCO_DSP_CH3			0x60002010
#define FPGA_COS_NCO_DSP_CH3			0x60002014
#define FPGA_SIN_NCO_DSP_CH4			0x60002018
#define FPGA_COS_NCO_DSP_CH4			0x6000201C
#define FPGA_SIN_NCO_DSP_BIT			0x60002020
#define FPGA_COS_NCO_DSP_BIT			0x60002024
//...............................................................................................
// 					RAM_RMS_FREQ_CALC	(0x2400)
//...............................................................................................
#define FPGA_FREQUENCY_CH1				0x60002400
#define FPGA_FREQUENCY_CH2				0x60002404
#define FPGA_FREQUENCY_CH3				0x60002408
#define FPGA_FREQUENCY_CH4				0x6000240C
#define FPGA_CYCLE_PERIOD_CH1			0x60002410
#define FPGA_CYCLE_PERIOD_CH2			0x60002414
#define FPGA_CYCLE_PERIOD_CH3			0x60002418
#define FPGA_CYCLE_PERIOD_CH4			0x6000241C
#define FPGA_SAMPLE_COUNT_CH1			0x60002420
#define FPGA_SAMPLE_COUNT_CH2			0x60002424
#define FPGA_SAMPLE_COUNT_CH3			0x60002428
#define FPGA_SAMPLE_COUNT_CH4			0x6000242C
#define FPGA_SUM_OF_SQ_REF_CH1			0x60002430
#define FPGA_SUM_OF_SQ_REF_CH2			0x60002434
#define FPGA_SUM_OF_SQ_REF_CH3			0x60002438
#define FPGA_SUM_OF_SQ_REF_CH4			0x6000243C
#define FPGA_SUM_OF_SQ_SIG_CH1			0x60002440
#define FPGA_SUM_OF_SQ_SIG_CH2			0x60002444
#define FPGA_SUM_OF_SQ_SIG_CH3			0x60002448
#define FPGA_SUM_OF_SQ_SIG_CH4			0x6000244C
#define FPGA_BKGND_DC_SIN_CH1			0x60002450
#define FPGA_BKGND_DC_COS_CH1			0x60002454
#define FPGA_BKGND_DC_SIN_CH2			0x60002458
#define FPGA_BKGND_DC_COS_CH2			0x6000245C
#define FPGA_BKGND_DC_SIN_CH3			0x60002460
#define FPGA_BKGND_DC_COS_CH3			0x60002464
#define FPGA_BKGND_DC_SIN_CH4			0x60002468
#define FPGA_BKGND_DC_COS_CH4			0x6000246C
//...............................................................................................
// 					RAM_BANDPASS_COEF_CH1	(0x4000)
//...............................................................................................
#define FPGA_BANDPASS_COEF_CH1			0x60004000
//...............................................................................................
// 					RAM_BANDPASS_COEF_CH2	(0x5000)
//...............................................................................................
#define FPGA_BANDPASS_COEF_CH2			0x60005000
//...............................................................................................
// 					RAM_BANDPASS_COEF_CH3	(0x6000)
//...............................................................................................
#define FPGA_BANDPASS_COEF_CH3			0x60006000
//..............................................................................................
// 					RAM_BANDPASS_COEF_CH4   (0x7000)
//...............................................................................................
#define FPGA_BANDPASS_COEF_CH4			0x60007000

//...............................................................................................
// 					RAM_SIGNAL_COEF_CH1	(0x8000)
//...............................................................................................
#define FPGA_SIGNAL_COEF_CH1			0x60008000
//...............................................................................................
// 					RAM_SIGNAL_COEF_CH2	(0x8800)
//...............................................................................................
#define FPGA_SIGNAL_COEF_CH2			0x60008800
//...............................................................................................
// 					RAM_SIGNAL_COEF_CH3	(0x9000)
//...............................................................................................
#define FPGA_SIGNAL_COEF_CH3			0x60009000
//..............................................................................................
// 					RAM_SIGNAL_COEF_CH4   (0x9800)
//...............................................................................................
#define FPGA_SIGNAL_COEF_CH4			0x60009800


//...............................................................................................
// UNSUSED 0x60003800 - 0x60003FF4
#define FPGA_LOOP_TIME					0x60003FF8
//...............................................................................................


#endif /* __MODULE_H__ */

