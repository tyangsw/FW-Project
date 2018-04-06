/*
 * DSx module specific definitions.
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
#define CAL_FILE_REQUIRED   	1

/*
 * Are programmable CPLDs implemented on this module?
 */
#if MODULE_DSJ
	#define CPLD1_IMPLEMENTED       0
	#define CPLD1_PROGRAM_LATE      0
#else
	#define CPLD1_IMPLEMENTED       1
	#define CPLD1_PROGRAM_LATE      1
#endif

#define CPLD2_IMPLEMENTED       0
#define CPLD2_PROGRAM_LATE      0



typedef enum {
	CHANNEL_1 = 0, CHANNEL_2 = 1, CHANNEL_3 = 2, CHANNEL_4 = 3, NUM_CHANNELS
} CHANNEL;

/*
 * Insert module specific definitions here...
 */

// === INSERTING CODE FROM JERRY'S S/D MODULE ===
#define LV_TO_HV_RATIO			1.2857142857142857142857142857143// This is the ratio of the wrap A/D values.  The Full Scale Range is 90% for LV and only 70% for HV so 90/70 = 1.2857
#if MODULE_DS8 || MODULE_DSE // High voltage
	#define MAX_SIG_HI_VLL			90*LV_TO_HV_RATIO			// Maximum signal voltage
	#define MAX_SIG_MULT			3.53573						// This is to stop multiplier from getting to large with a small signal applied. (minimum voltage s/b 28Vrms which would be 3.2143 (I added 10%))
#define BIT_SIG_GAIN_MULT		0x41900000						// Float representation of 18  (DACs at full scale)
#define BIT_REF_GAIN_MULT		0x42340000						// Float representation of 45  (DACs at full scale)

#else // MODULE_DSJ (low voltage)
	#define MAX_SIG_HI_VLL	   		28							// Maximum signal voltage
	#define MAX_SIG_MULT			33.000						// This is to stop multiplier from getting to large with a small signal applied. (minimum voltage s/b 1Vrms which would be 30(I added 10%))
	#define BIT_SIG_GAIN_MULT		0x413D0790					// Float representation of  11.814345991561181  (sig volt is at 2.37VLL)
	#define BIT_REF_GAIN_MULT		0x4314F6B7					// Float representation of 148.963730569948186  (ref volt is at 19.30Vrms * 25 for synth ref)
#endif
#define MAX_REF_HI_VRMS			115								// Maximum reference voltage
#define MAX_REF_MULT			126.5							// This is to stop multiplier from getting to large with a small signal applied. (minimum voltage s/b 1Vrms which would be 115 (I added 10%))


// =============================================

#define CURRENT_AD_LSB_DS8		134.3e-6 					// This is using a 0.12ohm shunt, a gain of 50 measurement diff-amp, and a 12bit 3.3V A/D
#define CURRENT_AD_LSB_DSE		67.7e-6 					// This is using a 0.24ohm shunt, a gain of 50 measurement diff-amp, and a 12bit 3.3V A/D
#define VELOCITY_SCALE		 	6.2498635710280455068031977630051e+2 // Came from Jerry
#define HV_VLL_OUTPUT_SCALE		0x22B4F//0x1B297			// Scale to make a cal value of "0x4000" give an output of about 90volts when VLL is set to 90
#define LV_VLL_OUTPUT_SCALE 	0x14FBB						// Scale to make a cal value of "0x4000" give an output of about 26volts when VLL is set to 26
#define NUM_D2_BITS				12							// Number of bits for number of samples
#define NUM_D2_SAMPLES			pow(2,NUM_D2_BITS) 			// Number of samples to average (4096)
#define NUM_D3_BITS				12							// Number of bits for number of samples
#define NUM_D3_SAMPLES			pow(2,NUM_D3_BITS) 			// Number of samples to average (4096)
#define MAX_BIT_ERROR			36							// This is ((0.2 Degrees)/(2^16)) / (360/2^32)
#define BIT_DELAY_TIME			150000  					// roughly 1 second if HPS is running at 250MHz
#define MAX_OFFSET_SAMPLES	 	65535						// Maximum number of samples allowed for wrap offset measurement

#define MAX_BKGND_ANG_SAMPLES   4096						// Number of samples for background angle correction
#define MIN_BKGND_ANG_CORR		23860						// 5 degrees = 5/(360/2^32)
#define TESTGEN_FREQ_SCALE		21594						// Scale to get test generator frequency delta


//============================================
//  HOST REGISTERS
//============================================
typedef volatile struct {
	u32 ds_Angle[NUM_CHANNELS];       //
	u32 ds_Voltage[NUM_CHANNELS];
	u32 ExpectedReference[NUM_CHANNELS];
	u32 ds_PhaseOffset[NUM_CHANNELS];
	u32 ds_OutputMode[NUM_CHANNELS];
	u32 wrap_Angle[NUM_CHANNELS];
	u32 wrap_Velocity[NUM_CHANNELS];
	u32 meas_Frequency[NUM_CHANNELS];
	u32 meas_VLL[NUM_CHANNELS];
	u32 meas_Ref[NUM_CHANNELS];
	u32 meas_Current[NUM_CHANNELS];
	u32 sig_Threshold[NUM_CHANNELS];
	u32 ref_Threshold[NUM_CHANNELS];
	u32 cur_Threshold[NUM_CHANNELS];
	u32 SynRsl_sel[NUM_CHANNELS];
	u32 RotationMode[NUM_CHANNELS];
	u32 SetStopAng[NUM_CHANNELS];
	u32 RotationRate[NUM_CHANNELS];
	u32 StartRotation;
	u32 StopRotation;
	u32 reserved_[2];
	u32 TrigSelect[NUM_CHANNELS];
	u32 DSRatioMode;

	u32 reserved_2[3];
	u32 DLVMode[NUM_CHANNELS];

	u32 reserved_3[104];

	u32 reset_Fifo;
	u32 enable_Fifo1Capture;
	u32 enable_Fifo2Capture;
	u32 OpenDigCtrlLoop;
	u32 EnableTestGen;
	u32 PwmScaleCh1;
	u32 PwmScaleCh2;
	u32 PwmScaleCh3;
	u32 dbg_OpenPhaseLock;
	u32 dbg_PausePhaseCorr;
	u32 dbg_Chan;
	u32 wr_ArmDelAmt;
	u32 DebugCtrlSin;
	u32 DebugCtrlCos;
	u32 PWM_DC_Value;
	u32 En_BkgrndCal;
	u32 TestGenFreq;

	u32 reserved_4[15];
	u32 raw_curr[6];
	u32 RmsCurrEachSide[6];

} HOST_REGS;


//============================================
//  HPS REGISTERS
//============================================
typedef volatile struct {
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Misc Module Control (0x00 - 0x1C)
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	u32 Pwr_En;   			// (0x00000000)
	u32 Wr_BitStatus;       // (0x00000004)
	u32 Wr_ChanProgram;		// (0x00000008)
	u32 ModuleType;			// (0x0000000C)
	u32 ExtRefSel;			// (0x00000010)
	u32 HVLV_sel;			//
	u32 _reserved0[2];

	/* Debug Control */// (0x00000020)
	u32 Rst_Fifo;
	u32 D3TestEn;
	u32 D3TestWord;
	u32 Rd_fifo1Data;
	u32 Rd_fifo2Data;
	u32 OpenDigCtrlLoop;
	u32 EnableTestGen;
	u32 Wr_TestGenFreq;
	//-----------------------
	u32 PwmScaleCh1;
	u32 PwmScaleCh2;
	u32 PwmScaleCh3;
	u32 dbg_OpenPhaseLock;
	u32 dbg_PausePhaseCorr;
	u32 dbg_Chan;
	u32 wr_ArmDelAmt;
	u32 SynchroEnable;
	//-----------------------
	u32 Wr_DigCtrlEnable;
	u32 Wr_DigCtrlAddr;
	s32 Wr_DigCtrlData;
	u32 ChanStatusEnable;
	u32 Wr_WrapDbg;
	u32 Rd_RotationStatus;
	u32 Wr_Multispeed;
	u32 _reserved2[1];
	//-----------------------
	u32 Wr_SigLossThreshold[NUM_CHANNELS];
	u32 Wr_RefLossThreshold[NUM_CHANNELS];
	//-----------------------
	u32 Wr_CurrentThreshold[NUM_CHANNELS];
	u32 Wr_VLLScale[NUM_CHANNELS];
	//-----------------------
	u32 Rd_RefVoltage[NUM_CHANNELS];
	u32 Rd_RawVelocity[NUM_CHANNELS];
	u32 Rd_WrapAngle[NUM_CHANNELS];
	u32 Rd_Frequency[NUM_CHANNELS];
	u32 Wr_DSVLL[NUM_CHANNELS];
	u32 Rd_ExpectedAngle[NUM_CHANNELS];
	//-----------------------
	u32 RdCurrent[6]; 					// 0x120-0x134
	u32 _reserved3[2];					//
	u32 RdMeasWrapOffs[6];				//Chan, sin/cos
	u32 _reserved4[2];					//
	u32 WrWrapOffsetCorr[6]; 			//Chan, sin/cos
	u32 _reserved5[2]; 					//
	u32 WrMaxOffsCycles[NUM_CHANNELS];   // 0x60000180-18C
	u32 Wr_FixedModeScale[NUM_CHANNELS]; // 0x60000190-19C
	u32 Rd_RefSumOfSquares[NUM_CHANNELS*2];// 0x600001A0-1BC
	u32 Rd_PeriodCnt[NUM_CHANNELS];		// 0x600001C0-1CC
	u32 Rd_RefRmsStrd[NUM_CHANNELS];	// 0x600001D0-1DC
	u32 Wr_RefScale[NUM_CHANNELS];		// 0x600001E0-1EC
	u32 Rd_MeasVll[NUM_CHANNELS];		// 0x600001F0-1FC


	/*   union {
	 uint32_t    reg;
	 struct {
	 uint32_t    te_en       :  1;   // Bit  0
	 uint32_t    de_en       :  1;   // Bit  1
	 uint32_t    _reserved   : 30;   // Bits 2-31
	 } bits;
	 } tcvr_ctrl;                // (0x00000008)
	 */

} HPS_REGS;

//============================================
//  CALIBRATION REGISTERS
//============================================
	//...............................................................................................
	// 					BW_COEF_RAM	(0x0400)
	//...............................................................................................
	#define FPGA_COEF_INTEGRATOR_CH1		0x60000200
	#define FPGA_COEF_INTEGRATOR_CH2		0x60000204
	#define FPGA_COEF_INTEGRATOR_CH3		0x60000208
	#define FPGA_COEF_INTEGRATOR_CH4		0x6000020C
	#define FPGA_COEF_PROPORTIONAL_CH1		0x60000210
	#define FPGA_COEF_PROPORTIONAL_CH2		0x60000214
	#define FPGA_COEF_PROPORTIONAL_CH3		0x60000218
	#define FPGA_COEF_PROPORTIONAL_CH4		0x6000021C
	#define FPGA_GAIN_FILTER_CH1			0x60000220
	#define FPGA_GAIN_FILTER_CH2			0x60000224
	#define FPGA_GAIN_FILTER_CH3			0x60000228
	#define FPGA_GAIN_FILTER_CH4			0x6000022C
	#define FPGA_COEF_FILTER_CH1			0x60000230
	#define FPGA_COEF_FILTER_CH2			0x60000234
	#define FPGA_COEF_FILTER_CH3			0x60000238
	#define FPGA_COEF_FILTER_CH4			0x6000023C
	#define FPGA_SIG_GAIN_MULT_CH1			0x60000240
	#define FPGA_SIG_GAIN_MULT_CH2			0x60000244
	#define FPGA_SIG_GAIN_MULT_CH3			0x60000248
	#define FPGA_SIG_GAIN_MULT_CH4			0x6000024C
	#define FPGA_REF_GAIN_MULT_CH1			0x60000250
	#define FPGA_REF_GAIN_MULT_CH2			0x60000254
	#define FPGA_REF_GAIN_MULT_CH3			0x60000258
	#define FPGA_REF_GAIN_MULT_CH4			0x6000025C
	#define FPGA_SYNTH_GAIN_MULT_CH1		0x60000260
	#define FPGA_SYNTH_GAIN_MULT_CH2		0x60000264
	#define FPGA_SYNTH_GAIN_MULT_CH3		0x60000268
	#define FPGA_SYNTH_GAIN_MULT_CH4		0x6000026C
	#define FPGA_BW_COEF_INTEGRATOR_BIT		0x60000270
	#define FPGA_BW_COEF_PROPORTIONAL_BIT	0x60000274
	#define FPGA_BW_GAIN_FILTER_BIT			0x60000278
	#define FPGA_BW_COEF_FILTER_BIT			0x6000027C
	#define FPGA_SIG_GAIN_MULT_BIT			0x60000280
	#define FPGA_REF_GAIN_MULT_BIT			0x60000284
	#define FPGA_SYNTH_GAIN_MULT_BIT		0x60000288

	//...............................................................................................
	// 					RAM_SCALED_AD_DATA	(0x1400)
	//...............................................................................................
	#define FPGA_SCALED_SIN_CH1				0x60000300
	#define FPGA_SCALED_SIN_CH2				0x60000304
	#define FPGA_SCALED_SIN_CH3				0x60000308
	#define FPGA_SCALED_SIN_CH4				0x6000030C
	#define FPGA_SCALED_COS_CH1				0x60000310
	#define FPGA_SCALED_COS_CH2				0x60000314
	#define FPGA_SCALED_COS_CH3				0x60000318
	#define FPGA_SCALED_COS_CH4				0x6000031C
	#define FPGA_SCALED_REF_CH1				0x60000320
	#define FPGA_SCALED_REF_CH2				0x60000324
	#define FPGA_SCALED_REF_CH3				0x60000328
	#define FPGA_SCALED_REF_CH4				0x6000032C
	#define FPGA_SCALED_SYNTH_REF_4W_CH1	0x60000330
	#define FPGA_SCALED_SYNTH_REF_4W_CH2	0x60000334
	#define FPGA_SCALED_SYNTH_REF_4W_CH3	0x60000338
	#define FPGA_SCALED_SYNTH_REF_4W_CH4	0x6000033C
	#define FPGA_SCALED_SYNTH_REF_2W_CH1	0x60000340
	#define FPGA_SCALED_SYNTH_REF_2W_CH2	0x60000344
	#define FPGA_SCALED_SYNTH_REF_2W_CH3	0x60000348
	#define FPGA_SCALED_SYNTH_REF_2W_CH4	0x6000034C
	#define FPGA_SCALED_SIN_AD_BIT			0x60000350
	#define FPGA_SCALED_COS_AD_BIT			0x60000354
	#define FPGA_SCALED_REF_AD_BIT			0x60000358
	#define FPGA_SCALED_SYNTH_REF_4W_BIT	0x6000035C
	#define FPGA_SCALED_SYNTH_REF_2W_BIT	0x60000360

	//...............................................................................................
	// 					RAM_DMOD_CALC	(0x1800)
	//...............................................................................................
	#define FPGA_DEMOD_ERROR_CH1			0x60000400
	#define FPGA_DEMOD_ERROR_CH2			0x60000404
	#define FPGA_DEMOD_ERROR_CH3			0x60000408
	#define FPGA_DEMOD_ERROR_CH4			0x6000040C
	#define FPGA_ACCUM_REF_PHASE_LO_CH1		0x60000410
	#define FPGA_ACCUM_REF_PHASE_HI_CH1		0x60000414
	#define FPGA_ACCUM_REF_PHASE_LO_CH2		0x60000418
	#define FPGA_ACCUM_REF_PHASE_HI_CH2		0x6000041C
	#define FPGA_ACCUM_REF_PHASE_LO_CH3		0x60000420
	#define FPGA_ACCUM_REF_PHASE_HI_CH3		0x60000424
	#define FPGA_ACCUM_REF_PHASE_LO_CH4		0x60000428
	#define FPGA_ACCUM_REF_PHASE_HI_CH4		0x6000042C
	#define FPGA_SIN_COS_ERROR_CH1			0x60000430
	#define FPGA_SIN_COS_ERROR_CH2			0x60000434
	#define FPGA_SIN_COS_ERROR_CH3			0x60000438
	#define FPGA_SIN_COS_ERROR_CH4 			0x6000043C
	#define FPGA_SYNTH_REF_CH1				0x60000440
	#define FPGA_SYNTH_REF_CH2				0x60000444
	#define FPGA_SYNTH_REF_CH3				0x60000448
	#define FPGA_SYNTH_REF_CH4				0x6000044C
	#define FPGA_SYNTH_REAL_REF_SEL_CH1		0x60000450
	#define FPGA_SYNTH_REAL_REF_SEL_CH2		0x60000454
	#define FPGA_SYNTH_REAL_REF_SEL_CH3		0x60000458
	#define FPGA_SYNTH_REAL_REF_SEL_CH4		0x6000045C
	#define FPGA_DEMOD_ERROR_BIT			0x60000460
	#define FPGA_ACCUM_REF_PHASE_LO_BIT		0x60000464
	#define FPGA_ACCUM_REF_PHASE_HI_BIT		0x60000468
	#define FPGA_SIN_COS_ERROR_BIT			0x6000046C
	#define FPGA_SYNTH_REF_BIT				0x60000470
	#define FPGA_SYNTH_REAL_REF_SEL_BIT		0x60000474

	//...............................................................................................
	// 					RAM_PI_CALC	(0x1C00)
	//...............................................................................................
	#define FPGA_PI_ERROR_CH1				0x60000500
	#define FPGA_PI_ERROR_CH2				0x60000504
	#define FPGA_PI_ERROR_CH3				0x60000508
	#define FPGA_PI_ERROR_CH4				0x6000050C
	#define FPGA_INTEGRATOR_VAL_CH1			0x60000510
	#define FPGA_INTEGRATOR_VAL_CH2			0x60000514
	#define FPGA_INTEGRATOR_VAL_CH3			0x60000518
	#define FPGA_INTEGRATOR_VAL_CH4			0x6000051C
	// SKIP 0x1C20 - 0x1C2C                       
	#define FPGA_PI_ERROR_BIT				0x60000530
	#define FPGA_INTEGRATOR_VAL_BIT			0x60000534
	#define FPGA_MIN_INTEGRATOR_CH1			0x60000540
	#define FPGA_MIN_INTEGRATOR_CH2			0x60000544
	#define FPGA_MIN_INTEGRATOR_CH3			0x60000548
	#define FPGA_MIN_INTEGRATOR_CH4			0x6000054C
	#define FPGA_MAX_INTEGRATOR_CH1			0x60000550
	#define FPGA_MAX_INTEGRATOR_CH2			0x60000554
	#define FPGA_MAX_INTEGRATOR_CH3			0x60000558
	#define FPGA_MAX_INTEGRATOR_CH4			0x6000055C

	//...............................................................................................
	// 					RAM_NCO_CALC	(0x2000)
	//...............................................................................................
	#define FPGA_SIN_NCO_DSP_CH1			0x60000600
	#define FPGA_COS_NCO_DSP_CH1			0x60000604
	#define FPGA_SIN_NCO_DSP_CH2			0x60000608
	#define FPGA_COS_NCO_DSP_CH2			0x6000060C
	#define FPGA_SIN_NCO_DSP_CH3			0x60000610
	#define FPGA_COS_NCO_DSP_CH3			0x60000614
	#define FPGA_SIN_NCO_DSP_CH4			0x60000618
	#define FPGA_COS_NCO_DSP_CH4			0x6000061C
	#define FPGA_SIN_NCO_DSP_BIT			0x60000620
	#define FPGA_COS_NCO_DSP_BIT			0x60000624

	//-----------------------

typedef volatile struct {

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Calibration (0x000 - 0x1FC)
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	u32 ds_voltage[NUM_CHANNELS];   			// [NUM_CHANNELS]
	u32 ds_angle[NUM_CHANNELS];    				// [NUM_CHANNELS]
	u32 ds_offset[NUM_CHANNELS][2]; 			// [CHAN][Sin/Cos]
	u32 ds_phseCorr[NUM_CHANNELS]; 				// [NUM_CHANNELS]
	u32 ds_WrapCorr[NUM_CHANNELS];
	u32 ds_VllMeasCorr[NUM_CHANNELS];
	u32 ds_RefMeasCorr[NUM_CHANNELS];
	u32 ds_MeasCurOffset[6];					// [chan][sin/cos];
	u32 _reserved1[2];
	u32 ds_PwmGain[NUM_CHANNELS];
	u32 ds_PwmPhase[NUM_CHANNELS];
	u32 ds_PwmOffset[6]; 						// [chan][sin/cos];
	u32 _reserved2[2];
	u32 ds_MeasCurGain[6];						// [chan][sin/cos];
	u32 _reserved3[2];
	u32 ds_voltage_FixedMode[NUM_CHANNELS];   	// [NUM_CHANNELS]

	u32 _reserved4[60];
	u32 CurrLimSetting[NUM_CHANNELS];			// [chan];


} CAL_REGS;

#endif /* __MODULE_H__ */

