/*
 * LDx/SDx module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>
#include <BitFilterCoef_LoFreq.h>
#include <BitFilterCoef_HiFreq.h>
#include <SignalFilterCoef_LoFreq.h>
#include <SignalFilterCoef_HiFreq.h>
#include <RMSFreqScaleFactor.h>
#include <IntegratorValues.h>
#include <ProportionalValues.h>
#include <FilterCoef.h>
#include <FilterGain.h>

/* Global variables */
static bool mod_init 	= FALSE;
char 		tempbuf[255];


/* Local functions */
static void init(void);
static void load_params(void);
static void load_filter_coef_values();
static void init_fifo_values();


//---------------------------------------------------------------
//					FUNCTION POINTERS
//---------------------------------------------------------------
void (*ModuleFunctionPtr[])(void) ={
										CheckBandwidth,					// User selected
										CheckMultispeedRatio,			// User selected
										CheckDeltaAngle,				// User selected
										CheckSigThreshold,				// User selected
										CheckRefThreshold,				// User selected
										CheckChanModeSelect,			// User selected
										CheckLVDTScale,					// User selected
										CheckBITMode,					// User selected
										CheckFIFOCriteria,				// User selected

										CheckRefVoltIn,					// FPGA
										Calculate_RMSValues,			// FPGA
										Calculate_GainValues,			// FPGA
										Calculate_BkgndDCCal,			// FPGA
										Scale_VelocityOut,				// FPGA
										CheckSigLoss,					// FPGA
										CheckRefLoss,					// FPGA
										CheckFilterSelection,			// FPGA

										CheckBkgndDCRegs,				// TROUBLESHOOTING

										EndOfFunctions
								   }; // These routines update ~212ms

void (*BackgroundBitPtr[])(void) ={
										WaitFor_BitFreqSettled,
										SetBkgndBitChannel,
										SetBkgndBitAngle,
										WaitForAngleSettled,
										GetBkgndBitAngle,
										UpdateBkgndBitStatus,
										EndOfBITFunctions
								   }; // These routines update ~212ms

void (*D0BitPtr[])(void) =			{
										Init_D0_BitMode,
										WaitFor_D0_FreqSettled,
										Set_D0_BitAngle,
										WaitFor_D0_AngleSettled,
										Get_D0_BitAngle,
										Update_D0_BitStatus,
										EndOf_D0_BITFunctions
								   }; // These routines update ~212ms
void (*D3BitPtr[])(void) =			{
										Init_D3_BitMode,
										WaitFor_D3_FreqSettled,
										Set_D3_BitAngle,
										WaitFor_D3_AngleSettled,
										Get_D3_BitAngle,
										Update_D3_BitStatus,
										EndOf_D3_BITFunctions
								   }; // These routines update ~212ms

/**************************** Exported functions ****************************/
//---------------------------------------------------------------------------------------------------------------
void module_init(void)
{
	u16 n;

    /* Initialize module */
    init();

 	/* Configure DACS */
	Xil_Out32(FPGA_DAC_CONFIG_MODE, 0x00000001);			//	Set DAC to receive data from CONFIG WORD
	//....................
	Xil_Out32(FPGA_DAC_CONFIG_WORD, 0x007FFFFF);			//	Turn OFF Internal Reference for all DACs
	for(n=0; n<10; n++)
		WaitForDataReady();
	//....................
	Xil_Out32(FPGA_DAC_CONFIG_WORD, 0x004F0000);			//	Turn on all 4 DACS
	for(n=0; n<10; n++)
		WaitForDataReady();
	//....................
	Xil_Out32(FPGA_DAC_CONFIG_WORD, 0x005FFFFF);			//	MASK LDACn all DACs
	for(n=0; n<10; n++)
		WaitForDataReady();
	//....................
	Xil_Out32(FPGA_DAC_CONFIG_MODE, 0x00000000);			//	Set DAC to receive data from SINE/COSINE
 	/* End Configure DACS */
}
//---------------------------------------------------------------------------------------------------------------
void module_main(void)
{
	volatile u32   *pDPRAM;

    if (mod_init)
    {
    	Xil_Out32(FPGA_LOOP_TIME, 0x00000001);

    	WaitForDataReady();

   		ModuleFunctionPtr[FunctionIndex]();
   		CheckTroubleshootReg();

        // Calculate pointer for channel
    	pDPRAM 	= (volatile u32*)DPRAM_BIT_MODE;
 		if( ((*pDPRAM & 0x0009) == 0x0000) && (InD0Tests == FALSE) )			// Not in D0 or D3 mode
    	{

 	 		if( (*pDPRAM & 0x0004) == 0x0000)									// Not in D2 mode
 	 		{
 	 			return;															// Do nothing
 	 		}

 	 		//.......................................
 	 		// Let's do BACKGROUND BIT
 	 		//.......................................
	 		Xil_Out32(DPRAM_D2_VERIFY, 0x00000055);								//	Lets user know we are performing background bit

    		BackgroundBitPtr[BitFunctionIndex]();

    		// START - Clear out other modes
    		D0_BitFunctionIndex  	= 0;
    		D0BitWaitTime			= 0;
    		AverageD0BitAngleIndex	= 0;
    		for(int n = 0; n < MAX_CHAN; n++)
    		{
    			D0BitAngleAccumSigned[n] 	= 0;
    			D0BitAngleAccumDouble[n] 	= (double)0.0;
    		}

    		D3_BitFunctionIndex  	= 0;
     		D3BitAngleExpected		= 0.500;									// Instead of defaulting to 0.000, we default to 0.500
    		D3BitWaitTime			= 0;
    		AverageD3BitAngleIndex	= 0;
    		for(int n = 0; n < MAX_CHAN; n++)
        		D3BitAngleAccumulated[n] 	= 0;
    		// END - Clear out other modes

    	}
    	else
    	{
    		// Clear out D2 mode when performing D0 or D3 tests
    		BitFunctionIndex  			= 0;
    		BkgndBitChanIndex 			= 0;
    		BkgndBitAngleExpected		= 0.500;								// Instead of defaulting to 0.000, we default to 0.500
    		BkgndBitAngleAccumulated 	= 0;
    		BkgndBitWaitTime			= 0;
    		AverageBkgndBitAngleIndex   = 0;
     	}
    	Xil_Out32(FPGA_LOOP_TIME, 0x00000000);
    }
}
//---------------------------------------------------------------------------------------------------------------

/****************************** Local functions *****************************/
//===============================================================================================================
//												INITIALIZATION
//===============================================================================================================
static void init(void)
{
	// Coefficients (These values equate to 40Hz bandwidth)(Filter at 250Hz cutoff)
	Xil_Out32(FPGA_COEF_INTEGRATOR_CH1, 	0x3D6147AE);	//    0.055
	Xil_Out32(FPGA_COEF_PROPORTIONAL_CH1, 	0x420A0000);	//   34.5
	Xil_Out32(FPGA_COEF_FILTER_CH1, 		0xBF71B088);	//   -0.944099918145087
	Xil_Out32(FPGA_GAIN_FILTER_CH1, 		0x3CEB8CE6);	//    0.0287537082498563

	Xil_Out32(FPGA_COEF_INTEGRATOR_CH2, 	0x3D6147AE);	//    0.055
	Xil_Out32(FPGA_COEF_PROPORTIONAL_CH2, 	0x420A0000);	//   34.5
	Xil_Out32(FPGA_COEF_FILTER_CH2, 		0xBF71B088);	//   -0.944099918145087
	Xil_Out32(FPGA_GAIN_FILTER_CH2, 		0x3CEB8CE6);	//    0.0287537082498563

	Xil_Out32(FPGA_COEF_INTEGRATOR_CH3, 	0x3D6147AE);	//    0.055
	Xil_Out32(FPGA_COEF_PROPORTIONAL_CH3, 	0x420A0000);	//   34.5
	Xil_Out32(FPGA_COEF_FILTER_CH3, 		0xBF71B088);	//   -0.944099918145087
	Xil_Out32(FPGA_GAIN_FILTER_CH3, 		0x3CEB8CE6);	//    0.0287537082498563

	Xil_Out32(FPGA_COEF_INTEGRATOR_CH4, 	0x3D6147AE);	//    0.055
	Xil_Out32(FPGA_COEF_PROPORTIONAL_CH4, 	0x420A0000);	//   34.5
	Xil_Out32(FPGA_COEF_FILTER_CH4, 		0xBF71B088);	//   -0.944099918145087
	Xil_Out32(FPGA_GAIN_FILTER_CH4, 		0x3CEB8CE6);	//    0.0287537082498563

	Xil_Out32(FPGA_BW_COEF_INTEGRATOR_BIT, 	BIT_INTEGRATOR_VALUE);	//    8.59375000000
	Xil_Out32(FPGA_BW_COEF_PROPORTIONAL_BIT,BIT_PROPORTIONAL_VALUE);//  375.00000000000
	Xil_Out32(FPGA_BW_COEF_FILTER_BIT, 		BIT_FILTER_COEF);		//   -0.866008466898807
	Xil_Out32(FPGA_BW_GAIN_FILTER_BIT, 		BIT_FILTER_GAIN);		//    0.0718064979222085

	Xil_Out32(DPRAM_LVDT_SCALE_CH1,			0x10000000);	//	Initial gain of "1"
	Xil_Out32(DPRAM_LVDT_SCALE_CH2,			0x10000000);	//	Initial gain of "1"
	Xil_Out32(DPRAM_LVDT_SCALE_CH3,			0x10000000);	//	Initial gain of "1"
	Xil_Out32(DPRAM_LVDT_SCALE_CH4,			0x10000000);	//	Initial gain of "1"

	// Sig & Ref Gains (MOD_4CH_SD_#1)
	Xil_Out32(FPGA_SIG_GAIN_MULT_CH1, 0x3F89D89E);			//  Gain of 1.07 (Assuming 26 VLL)
	Xil_Out32(FPGA_SIG_GAIN_MULT_CH2, 0x3F89D89E);			//  Gain of 1.07 (Assuming 26 VLL)
	Xil_Out32(FPGA_SIG_GAIN_MULT_CH3, 0x3F89D89E);			//  Gain of 1.07 (Assuming 26 VLL)
	Xil_Out32(FPGA_SIG_GAIN_MULT_CH4, 0x3F89D89E);			//  Gain of 1.07 (Assuming 26 VLL)

	Xil_Out32(FPGA_REF_GAIN_MULT_CH1, 0x3F89D89E);			//	Gain of 1.07 (Assuming 26 V ref in)
	Xil_Out32(FPGA_REF_GAIN_MULT_CH2, 0x3F89D89E);			//	Gain of 1.07 (Assuming 26 V ref in)
	Xil_Out32(FPGA_REF_GAIN_MULT_CH3, 0x3F89D89E);			//	Gain of 1.07 (Assuming 26 V ref in)
	Xil_Out32(FPGA_REF_GAIN_MULT_CH4, 0x3F89D89E);			//	Gain of 1.07 (Assuming 26 V ref in)

	Xil_Out32(FPGA_SYNTH_GAIN_MULT_CH1, 0x41D76276);		//	Gain of (1.07 * 25) (Assuming 26 VLL)
	Xil_Out32(FPGA_SYNTH_GAIN_MULT_CH2, 0x41D76276);		//	Gain of (1.07 * 25) (Assuming 26 VLL)
	Xil_Out32(FPGA_SYNTH_GAIN_MULT_CH3, 0x41D76276);		//	Gain of (1.07 * 25) (Assuming 26 VLL)
	Xil_Out32(FPGA_SYNTH_GAIN_MULT_CH4, 0x41D76276);		//	Gain of (1.07 * 25) (Assuming 26 VLL)

	Xil_Out32(FPGA_DAC_CONFIG_MODE, 0x00000000);			//	Set DAC to receive data from SINE/COSINE
	Xil_Out32(FPGA_DAC_CONFIG_WORD, 0x00000000);			//	default
	Xil_Out32(FPGA_BIT_FREQ_CH1, BIT_DAC_FREQ);				//	default to 650hz(HF) -or- 7.560khz(LF)
	Xil_Out32(FPGA_BIT_FREQ_CH2, BIT_DAC_FREQ);				//	default to 650hz(HF) -or- 7.560khz(LF)
	Xil_Out32(FPGA_BIT_FREQ_CH3, BIT_DAC_FREQ);				//	default to 650hz(HF) -or- 7.560khz(LF)
	Xil_Out32(FPGA_BIT_FREQ_CH4, BIT_DAC_FREQ);				//	default to 650hz(HF) -or- 7.560khz(LF)

  	Xil_Out32(FPGA_BIT_DAC_GAIN_MULTIPLIER, BIT_DAC_SCALE);	//	Gain of FullScale(HV) or 1/8 Scale(LV)
	Xil_Out32(FPGA_SIG_GAIN_MULT_BIT,   BIT_SIG_GAIN_MULT);	//	Gain of 4    (DACs at 0x20000000)
	Xil_Out32(FPGA_REF_GAIN_MULT_BIT,   BIT_REF_GAIN_MULT);	//	Gain of 100  (DACs at 0x20000000)(not used)
	Xil_Out32(FPGA_SYNTH_GAIN_MULT_BIT, BIT_REF_GAIN_MULT);	//	Gain of 100  (DACs at 0x20000000)

    Xil_Out32(FPGA_MIN_INTEGRATOR_CH1, 0xCB742400);			// -16000000 (131040 DEG/SEC)
    Xil_Out32(FPGA_MAX_INTEGRATOR_CH1, 0x4B742400);			//  16000000 (131040 DEG/SEC)
    Xil_Out32(FPGA_MIN_INTEGRATOR_CH2, 0xCB742400);			// -16000000 (131040 DEG/SEC)
    Xil_Out32(FPGA_MAX_INTEGRATOR_CH2, 0x4B742400);			//  16000000 (131040 DEG/SEC)
    Xil_Out32(FPGA_MIN_INTEGRATOR_CH3, 0xCB742400);			// -16000000 (131040 DEG/SEC)
    Xil_Out32(FPGA_MAX_INTEGRATOR_CH3, 0x4B742400);			//  16000000 (131040 DEG/SEC)
    Xil_Out32(FPGA_MIN_INTEGRATOR_CH4, 0xCB742400);			// -16000000 (131040 DEG/SEC)
    Xil_Out32(FPGA_MAX_INTEGRATOR_CH4, 0x4B742400);			//  16000000 (131040 DEG/SEC)


	Xil_Out32(DPRAM_INTERNAL_REG_1, 0x60002400);			//	Default to SIN AD Chan 1(Can be any valid address though)
	Xil_Out32(DPRAM_INTERNAL_REG_2, 0x60002404);			//	Default to COS AD Chan 1(Can be any valid address though)
	Xil_Out32(DPRAM_INTERNAL_REG_3, 0x60002408);			//	Default to SIN AD Chan 2(Can be any valid address though)
	Xil_Out32(DPRAM_INTERNAL_REG_4, 0x6000240C);			//	Default to COS AD Chan 2(Can be any valid address though)

    Xil_Out32(DPRAM_BIT_ANGLE, 0x15555555);					//	Init to 30 degrees

	Xil_Out32(FPGA_BIT_STATUS, 0x0000);						//	Initialize to PASSED

	Xil_Out32(FPGA_CLEAR_PS_FAULT,	0x0000000F);			//	Turn on Power Supplies
	//-------------------------------------------------------
	//					START Initialize FIFOS
	//-------------------------------------------------------
	// 	1.	First we need to reset the DDR memory.
	// 	2.	Then we need to enter in the Start Addresses of each FIFO
	//	3.	Then we enter the FIFO count size.
	//	4.	Finally, we enable the FIFOs
	//-------------------------------------------------------
	Xil_Out32(FPGA_ENABLE_FIFOS,				0x00000000);//	DISABLE the FIFOs
	Xil_Out32(FPGA_RESET_FIFO_CTRL,				0x00000001);//	Reset the FIFO Controllers
	Xil_Out32(FPGA_RESET_FIFO_CTRL,				0x00000000);//	Reset the FIFO Controllers

	Xil_Out32(FPGA_FIFO_DDR_START_ADDRS_CH1,	0x00000004);//	Start of DDR address. Uses only upper half of DDR (128MB)
	Xil_Out32(FPGA_FIFO_DDR_START_ADDRS_CH2,	0x00000005);//	Since we have 4 channels, the upper 0x0400_0000 is divided by 4 giving us 0x0100_0000 per channel.
	Xil_Out32(FPGA_FIFO_DDR_START_ADDRS_CH3,	0x00000006);//	0x0100_0000 (16MB -or- 4MDWords). Since each FIFO is 4MDW, (0x0040_0000) the CNTR size in the fabric is 22 (2^22)
	Xil_Out32(FPGA_FIFO_DDR_START_ADDRS_CH4,	0x00000007);//	The number we write in the DDR_START_ADDRS registers is this number shifted by 20 bits.
															//  (2 bits less since we are in terms of 32 bit locations)This number is getting concatenated to the CNTR size)

	Xil_Out32(FPGA_FIFO_FULL_VALUE_CH1,			0x00400000);//	0x0040_0000 is 4MDWords
	Xil_Out32(FPGA_FIFO_FULL_VALUE_CH2,			0x00400000);//	0x0040_0000 is 4MDWords
	Xil_Out32(FPGA_FIFO_FULL_VALUE_CH3,			0x00400000);//	0x0040_0000 is 4MDWords
	Xil_Out32(FPGA_FIFO_FULL_VALUE_CH4,			0x00400000);//	0x0040_0000 is 4MDWords

	Xil_Out32(FPGA_ENABLE_FIFOS,				0x00000001);//	Enable the FIFOs
	//-------------------------------------------------------
	//					 END Initialize FIFOS
	//-------------------------------------------------------

	/* Initialize Global Variables */
	FunctionIndex 				= 0;
	BitFunctionIndex 			= 0;
	MaxBkgndBitError			= 0.05;
	InBITMode	  				= 0;
	InD0Tests 					= FALSE;
	InD3Tests 					= FALSE;
	LeftD0Tests 				= FALSE;
	LeftD3Tests 				= FALSE;
	WaitTimeBeforeSigLossDetect = 0;
	WaitTimeBeforeRefLossDetect = 0;
	BkgndBitAngleExpected		= 0.500;					// Instead of defaulting to 0.000, we default to 0.500
	D3BitAngleExpected			= 0.500;					// Instead of defaulting to 0.000, we default to 0.500
	for(int n = 0; n < MAX_CHAN; n++)
	{
		D0BitFailed[n] 			= FALSE;
		D3BitFailed[n] 			= FALSE;
		BitFailed[n]   			= FALSE;
		MaxD0BitError[n]		= 0.05;
		MaxD3BitError[n]		= 0.05;
	}


	/* Load configuration parameters */
    load_params();

    load_filter_coef_values();
    init_fifo_values();


    /* Channel is now initialized */
    mod_init = TRUE;
}
//===============================================================================================================
//												LOAD PARAMS
//===============================================================================================================
static void load_params(void)
{
    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
		// Bandwidth (Not implemented as of yet - defaults to 40Hz regardless)
		Xil_Out32(DPRAM_BANDWIDTH_CH1, 	DEFAULT_BANDWIDTH);
		Xil_Out32(DPRAM_BANDWIDTH_CH2, 	DEFAULT_BANDWIDTH);
		Xil_Out32(DPRAM_BANDWIDTH_CH3, 	DEFAULT_BANDWIDTH);
		Xil_Out32(DPRAM_BANDWIDTH_CH4, 	DEFAULT_BANDWIDTH);

		// Auto Bandwidth setting (Not implemented as of yet - defaults to 40Hz regardless)
		Xil_Out32(DPRAM_BAND_SEL_CH1, 	DEFAULT_BANDWIDTH_SEL);
		Xil_Out32(DPRAM_BAND_SEL_CH2, 	DEFAULT_BANDWIDTH_SEL);
		Xil_Out32(DPRAM_BAND_SEL_CH3, 	DEFAULT_BANDWIDTH_SEL);
		Xil_Out32(DPRAM_BAND_SEL_CH4, 	DEFAULT_BANDWIDTH_SEL);

		// Ratio
    	Xil_Out32(DPRAM_RATIO_CH1_2,	DEFAULT_MULTISPEED_RATIO);
    	Xil_Out32(DPRAM_RATIO_CH3_4,	DEFAULT_MULTISPEED_RATIO);

    	// Default Delta Angle (Not implemented as of yet)
		Xil_Out32(DPRAM_DELTA_ANG_CH1, 	DEFAULT_DELTA_ANG);
		Xil_Out32(DPRAM_DELTA_ANG_CH2, 	DEFAULT_DELTA_ANG);
		Xil_Out32(DPRAM_DELTA_ANG_CH3, 	DEFAULT_DELTA_ANG);
		Xil_Out32(DPRAM_DELTA_ANG_CH4, 	DEFAULT_DELTA_ANG);

		// Default Signal Loss (Not implemented as of yet)
		Xil_Out32(DPRAM_SIG_LOSS_THRESHOLD_CH1, DEFAULT_SIG_LOSS_THRESHOLD);
		Xil_Out32(DPRAM_SIG_LOSS_THRESHOLD_CH2, DEFAULT_SIG_LOSS_THRESHOLD);
		Xil_Out32(DPRAM_SIG_LOSS_THRESHOLD_CH3, DEFAULT_SIG_LOSS_THRESHOLD);
		Xil_Out32(DPRAM_SIG_LOSS_THRESHOLD_CH4, DEFAULT_SIG_LOSS_THRESHOLD);

		// Default Reference Loss (Not implemented as of yet)
		Xil_Out32(DPRAM_REF_LOSS_THRESHOLD_CH1, DEFAULT_REF_LOSS_THRESHOLD);
		Xil_Out32(DPRAM_REF_LOSS_THRESHOLD_CH2, DEFAULT_REF_LOSS_THRESHOLD);
		Xil_Out32(DPRAM_REF_LOSS_THRESHOLD_CH3, DEFAULT_REF_LOSS_THRESHOLD);
		Xil_Out32(DPRAM_REF_LOSS_THRESHOLD_CH4, DEFAULT_REF_LOSS_THRESHOLD);

		/* Channel Mode (RSL, 4WIRE, 2WIRE, SYN) */
		Xil_Out32(DPRAM_MODE_SEL_CH1, 	DEFAULT_CHAN_MODE);
		Xil_Out32(DPRAM_MODE_SEL_CH2, 	DEFAULT_CHAN_MODE);
		Xil_Out32(DPRAM_MODE_SEL_CH3, 	DEFAULT_CHAN_MODE);
		Xil_Out32(DPRAM_MODE_SEL_CH4, 	DEFAULT_CHAN_MODE);

		// LVDT Scale
		Xil_Out32(DPRAM_LVDT_SCALE_CH1,	DEFAULT_LVDT_SCALE);
		Xil_Out32(DPRAM_LVDT_SCALE_CH2, DEFAULT_LVDT_SCALE);
		Xil_Out32(DPRAM_LVDT_SCALE_CH3, DEFAULT_LVDT_SCALE);
		Xil_Out32(DPRAM_LVDT_SCALE_CH4, DEFAULT_LVDT_SCALE);
    }
}
//===============================================================================================================
//			LOAD Initial values for filter coefficients into RAM (Initializes as a Low Frequency Module)
//===============================================================================================================
static void load_filter_coef_values(void)
{
	volatile u32   *pFPGA;
	volatile u32   *pFPGA1, *pFPGA2, *pFPGA3, *pFPGA4;

	//-------------------------------------------------
	// Keep filters in reset until data is transferred
	//-------------------------------------------------
	Xil_Out32(FPGA_RESET_FILTERS_CH1,	1);
	Xil_Out32(FPGA_RESET_FILTERS_CH2,	1);
	Xil_Out32(FPGA_RESET_FILTERS_CH3,	1);
	Xil_Out32(FPGA_RESET_FILTERS_CH4,	1);

	//-------------------------------------------------
	// Transfer over all SIGNAL Filter coefficient data
	//-------------------------------------------------
	pFPGA1	= (volatile u32*)FPGA_SIGNAL_COEF_CH1;
	pFPGA2	= (volatile u32*)FPGA_SIGNAL_COEF_CH2;
	pFPGA3	= (volatile u32*)FPGA_SIGNAL_COEF_CH3;
	pFPGA4	= (volatile u32*)FPGA_SIGNAL_COEF_CH4;

	for(int coefIndex = 0; coefIndex < NUM_OF_SIG_FILT_COEF; coefIndex++)
	{
		*pFPGA1++ = SignalFilterCoefLoFreq[coefIndex];
		*pFPGA2++ = SignalFilterCoefLoFreq[coefIndex];
		*pFPGA3++ = SignalFilterCoefLoFreq[coefIndex];
		*pFPGA4++ = SignalFilterCoefLoFreq[coefIndex];
	}

	//-------------------------------------------------
	// Transfer over all BIT Filter coefficient data
	//-------------------------------------------------
	pFPGA1	= (volatile u32*)FPGA_BANDPASS_COEF_CH1;
	pFPGA2	= (volatile u32*)FPGA_BANDPASS_COEF_CH2;
	pFPGA3	= (volatile u32*)FPGA_BANDPASS_COEF_CH3;
	pFPGA4	= (volatile u32*)FPGA_BANDPASS_COEF_CH4;

	for(int coefIndex = 0; coefIndex < NUM_OF_BIT_FILT_COEF; coefIndex++)
	{
		*pFPGA1++ = BitFilterCoefLoFreq[coefIndex];
		*pFPGA2++ = BitFilterCoefLoFreq[coefIndex];
		*pFPGA3++ = BitFilterCoefLoFreq[coefIndex];
		*pFPGA4++ = BitFilterCoefLoFreq[coefIndex];
	}


	//-------------------------------------------------


	//-------------------------------------------------
	// Initialize NumOfTaps for the SIGNAL Filter (Low/Hi Pass)
	//-------------------------------------------------
	pFPGA	= (volatile u32*)FPGA_SigFiltTapCountCh1;
	for(int chanIndex = 0; chanIndex < MAX_CHAN; chanIndex++)
		*pFPGA++ = NUM_OF_SIG_FILT_COEF;

	//-------------------------------------------------
	// Initialize NumOfTaps for the BIT Filter (BandPass)
	//-------------------------------------------------
	pFPGA	= (volatile u32*)FPGA_BitFiltTapCountCh1;
	for(int chanIndex = 0; chanIndex < MAX_CHAN; chanIndex++)
		*pFPGA++ = NUM_OF_BIT_FILT_COEF;


	//-------------------------------------------------
	// Allow filters to run
	//-------------------------------------------------
	Xil_Out32(FPGA_RESET_FILTERS_CH1,	0);
	Xil_Out32(FPGA_RESET_FILTERS_CH2,	0);
	Xil_Out32(FPGA_RESET_FILTERS_CH3,	0);
	Xil_Out32(FPGA_RESET_FILTERS_CH4,	0);
}
//===============================================================================================================
void init_fifo_values(void)
{
	volatile u32   *pDPRAM;

	for(int n = 0; n < MAX_CHAN; n++)
	{
		// Hi Threshold							(4128768)
		pDPRAM	= (volatile u32*)DPRAM_FIFO_HI_THRESHOLD_CH1;
		pDPRAM += (n * 0020);
		*pDPRAM = 0x003F0000;
		// Lo Threshold							(100)
		pDPRAM	= (volatile u32*)DPRAM_FIFO_LO_THRESHOLD_CH1;
		pDPRAM += (n * 0020);
		*pDPRAM = 0x00000064;
		// Delay 								(No Delay)
		pDPRAM	= (volatile u32*)DPRAM_FIFO_DELAY_CH1;
		pDPRAM += (n * 0020);
		*pDPRAM = 0x00000000;
		// Rate  								(Take every Sample)
		pDPRAM	= (volatile u32*)DPRAM_FIFO_DATA_RATE_CH1;
		pDPRAM += (n * 0020);
		*pDPRAM = 0x00000001;
		// Number of samples 					(8192 samples)
		pDPRAM	= (volatile u32*)DPRAM_FIFO_NUM_OF_SAMPLES_CH1;
		pDPRAM += (n * 0020);
		*pDPRAM = 0x00002000;
		// Data Type							(Angle, Velocity & Timestamp)
		pDPRAM	= (volatile u32*)DPRAM_FIFO_DATA_TYPE_CH1;
		pDPRAM += (n * 0020);
		*pDPRAM = 0x00000007;
		// Trigger Source						(Internal)
		pDPRAM	= (volatile u32*)DPRAM_FIFO_TRIGGER_SRC_CH1;
		pDPRAM += (n * 0020);
		*pDPRAM = 0x00000002;
		// Interrupt Enable						(Disabled)
		pDPRAM	= (volatile u32*)DPRAM_INTERRUPT_ENABLE_CH1;
		pDPRAM += (n * 0020);
		*pDPRAM = 0x00000000;
	}

}
//===============================================================================================================
void CheckTroubleshootReg(void)
{
	volatile u32   *pDPRAM;
	volatile u32   *pFPGA;
	u32				userAddress;
	u32				userRegValue;

	// Calculate pointer for channel
	pDPRAM 	= (volatile u32*)DPRAM_INTERNAL_REG_1;
	pFPGA 	= (volatile u32*)FPGA_INTERNAL_REG_1;

	// Get POINTER
	for(int n = 0; n < MAX_CHAN; n++)
	{
		userAddress 	= *pDPRAM++;
		userRegValue 	= *(volatile u32*)userAddress;
		*pFPGA++ 		= userRegValue;
	}

	// Just for now, let's put the background value out to the user
	pFPGA  = (volatile u32*)FPGA_ANGLE_VALUE_BIT;
	pDPRAM = (volatile u32*)DPRAM_BKGND_BIT_VALUE;
	*pDPRAM= *pFPGA;
}
//===============================================================================================================
//										******  MODULE FUNCTIONS  ******
//===============================================================================================================
void CheckBandwidth(void)
{
	volatile u32   *pDPRAM_autoBW, *pDPRAM_userBW;
	volatile u32   *pFPGA_Integrator, *pFPGA_Proportional, *pFPGA_FilterGain, *pFPGA_FilterCoef;
	volatile u32   *pFPGA_MaxIntegrator, *pFPGA_MinIntegrator;
	float			tempBW;
	double 			minIntegratorValue, maxIntegratorValue;
	u32				tempFreq;
	u32				userBandwidthValue;
	u16				bandwidthIndex;

	// Address
	pDPRAM_userBW		= (volatile u32*)DPRAM_BANDWIDTH_CH1;
	pDPRAM_autoBW		= (volatile u32*)DPRAM_BAND_SEL_CH1;
	pFPGA_Integrator 	= (volatile u32*)FPGA_COEF_INTEGRATOR_CH1;
	pFPGA_Proportional 	= (volatile u32*)FPGA_COEF_PROPORTIONAL_CH1;
	pFPGA_FilterGain 	= (volatile u32*)FPGA_GAIN_FILTER_CH1;
	pFPGA_FilterCoef 	= (volatile u32*)FPGA_COEF_FILTER_CH1;
	pFPGA_MinIntegrator = (volatile u32*)FPGA_MIN_INTEGRATOR_CH1;
	pFPGA_MaxIntegrator = (volatile u32*)FPGA_MAX_INTEGRATOR_CH1;

	// Calculate pointer for channel
	pDPRAM_userBW		+= (ChanIndex * 0x14);
	pDPRAM_autoBW		+= (ChanIndex * 0x14);
	pFPGA_Integrator	+= (ChanIndex * 0x01);
	pFPGA_Proportional	+= (ChanIndex * 0x01);
	pFPGA_FilterGain	+= (ChanIndex * 0x01);
	pFPGA_FilterCoef	+= (ChanIndex * 0x01);
	pFPGA_MinIntegrator	+= (ChanIndex * 0x01);
	pFPGA_MaxIntegrator	+= (ChanIndex * 0x01);

	//----------------------------------------------
	userBandwidthValue = *pDPRAM_userBW;															// Get user current bandwidth value (User or Auto generated)
	bandwidthIndex     = (u16)(userBandwidthValue >> 1);											// calculate index
	//----------------------------------------------

	// Check how user wants to implement bandwidth
	if(*pDPRAM_autoBW == 0x0001)																	// User selected auto bandwidth
	{
		tempFreq = RefFrequency[ChanIndex];															// get the current carrier frequency of the channel
		if( (tempFreq > (PrevRefFrequency[ChanIndex] + (PrevRefFrequency[ChanIndex] >> 3)) ) ||
			(tempFreq < (PrevRefFrequency[ChanIndex] - (PrevRefFrequency[ChanIndex] >> 3)) ) )
		{
			PrevRefFrequency[ChanIndex] = tempFreq;													// update previous frequency value
			tempBW = ((float)tempFreq / 20);														// This is 1/10th of reference frequency divided by 2 for index
			bandwidthIndex = (u16)(tempBW);															// calculate new index
			*pDPRAM_userBW = (bandwidthIndex * 2);													// Write out auto calculated bandwidth to user
		}
	}
	else
	{
		PrevRefFrequency[ChanIndex] = 0;	// This is here to allow auto bandwidth mode to make the change without the actual frequency changing.
	}

	// Set limit on index
	if(bandwidthIndex > 640)																		// Check boundary
		bandwidthIndex = 640;																		// index is half since we have 2hz resolution

	// Write out values to hardware
	*(volatile float*)pFPGA_Integrator 		= IntegratorValues[bandwidthIndex];						// Write Integrator value
	*(volatile float*)pFPGA_Proportional 	= ProportionalValues[bandwidthIndex];					// Write Proportional value
	*(volatile float*)pFPGA_FilterGain 		= FilterGain[bandwidthIndex];							// Write Filter Gain value
	*(volatile float*)pFPGA_FilterCoef	 	= FilterCoef[bandwidthIndex];							// Write Filter Coef value

	// Write out max/min integrator values
	//
	maxIntegratorValue = (10989 * (bandwidthIndex * 2));
	minIntegratorValue = (maxIntegratorValue * -1);
	*(volatile float*)pFPGA_MinIntegrator 	= minIntegratorValue;									// Write Integrator value
	*(volatile float*)pFPGA_MaxIntegrator 	= maxIntegratorValue;									// Write Proportional value


	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;																				// Reset to first channel
		FunctionIndex++;																			// Increment to next function
	}
}
//===============================================================================================================
void CheckMultispeedRatio(void)
{
	volatile u32   *pDPRAM, *pFPGA;
	u32				multispeedRatio;

	// Calculate pointer for channel
	pDPRAM 	= (volatile u32*)DPRAM_RATIO_CH1_2;
	pFPGA 	= (volatile u32*)DPRAM_RATIO_CH1_2;	// FPGA uses same address

	// Get bandwidth requested by USER
	pDPRAM	+= (ChanIndex * 0x28);
	pFPGA	+= (ChanIndex * 0x28);

	multispeedRatio = *pDPRAM;

	// Write out to hardware
	*pFPGA = multispeedRatio;

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= (MAX_CHAN / 2))	// every 2 channels
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void CheckDeltaAngle(void)
{
	volatile u32   *pDPRAM, *pFPGA;
	u32				deltaAngle;

	// Calculate pointer for channel
	pDPRAM 	= (volatile u32*)DPRAM_DELTA_ANG_CH1;
	pFPGA 	= (volatile u32*)DPRAM_DELTA_ANG_CH1;	// FPGA uses same address

	// Get bandwidth requested by USER
	pDPRAM	+= (ChanIndex * 0x14);
	pFPGA	+= (ChanIndex * 0x14);

	deltaAngle = *pDPRAM;

	// Write out to hardware
	*pFPGA = deltaAngle;

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void CheckSigThreshold(void)
{
	volatile u32   *pDPRAM, *pFPGA;
	u32				threshold;

	// Calculate pointer for channel
	pDPRAM 	= (volatile u32*)DPRAM_SIG_LOSS_THRESHOLD_CH1;
	pFPGA 	= (volatile u32*)DPRAM_SIG_LOSS_THRESHOLD_CH1;	// FPGA uses same address

	// Get bandwidth requested by USER
	pDPRAM	+= (ChanIndex * 0x14);
	pFPGA	+= (ChanIndex * 0x14);

	threshold = *pDPRAM;

	// Write out to hardware
	*pFPGA = threshold;

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void CheckRefThreshold(void)
{
	volatile u32   *pDPRAM, *pFPGA;
	u32				threshold;

	// Calculate pointer for channel
	pDPRAM 	= (volatile u32*)DPRAM_REF_LOSS_THRESHOLD_CH1;
	pFPGA 	= (volatile u32*)DPRAM_REF_LOSS_THRESHOLD_CH1;	// FPGA uses same address

	// Get bandwidth requested by USER
	pDPRAM	+= (ChanIndex * 0x14);
	pFPGA	+= (ChanIndex * 0x14);

	threshold = *pDPRAM;

	// Write out to hardware
	*pFPGA = threshold;

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void CheckChanModeSelect(void)
{
	volatile u32   *pDPRAM, *pFPGA;
	u32				chanMode;

	// Calculate pointer for channel
	pDPRAM 	= (volatile u32*)DPRAM_MODE_SEL_CH1;
	pFPGA 	= (volatile u32*)DPRAM_MODE_SEL_CH1;	// FPGA uses same address

	// Get Chan Mode (RSL, 2W, 4W, SYN) requested by USER
	pDPRAM	+= (ChanIndex * 0x14);
	pFPGA	+= (ChanIndex * 0x14);

	chanMode = *pDPRAM;

	// Write out to hardware
	*pFPGA = chanMode;

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void CheckLVDTScale(void)
{
	volatile u32   *pDPRAM, *pFPGA;
	u32				lvdtScale;

	// Calculate pointer for channel
	pDPRAM 	= (volatile u32*)DPRAM_LVDT_SCALE_CH1;
	pFPGA 	= (volatile u32*)DPRAM_LVDT_SCALE_CH1;	// FPGA uses same address

	// Get bandwidth requested by USER
	pDPRAM	+= (ChanIndex * 0x14);
	pFPGA	+= (ChanIndex * 0x14);

	lvdtScale = *pDPRAM;

	// Write out to hardware
	*pFPGA = lvdtScale;

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void CheckRefVoltIn(void)
{
	u32				refState;
	u32				bitMaskHigh;
	u32				bitMaskLow;
	volatile u32   *pFPGA_REF_SELECT;

	// Get current setting of muxes for HI/LO reference
	pFPGA_REF_SELECT = (volatile u32*)FPGA_REF_SELECT;
	refState = *pFPGA_REF_SELECT;

	// Build Mask
	bitMaskHigh = (0x00000001 << ChanIndex);
	bitMaskLow  = ~ bitMaskHigh;
	if(refState & bitMaskHigh)
		RefSelState[ChanIndex] = 1;
	else
		RefSelState[ChanIndex] = 0;

	// Check if voltage exceeds 30VRMS
	// Set new reference state
	if((RefSelState[ChanIndex] == 0) && (Ref_SumOfSq[ChanIndex] > REF_LO2HI_THRESHOLD))
	{
		refState |= bitMaskHigh;
	}
	else if( (RefSelState[ChanIndex] == 1) && (Ref_SumOfSq[ChanIndex] < REF_HI2LO_THRESHOLD) )
	{
		refState &= bitMaskLow;
	}

	// Output new setting
	*pFPGA_REF_SELECT = refState;

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void Calculate_RMSValues(void)
{
	volatile u32 		*pFPGA_SUM_OF_SQ_SIG, *pFPGA_SUM_OF_SQ_REF,	*pFPGA_SAMPLE_COUNT,  *pFPGA_FREQUENCY;
	volatile u32 		*pDPRAM_MEAS_REF, *pDPRAM_MEAS_SIG, *pDPRAM_MODE_SEL, *pDPRAM_MEAS_FREQ;

	// Calculate pointer for channel
	pFPGA_FREQUENCY 		= (volatile u32*)FPGA_FREQUENCY_CH1;
	pFPGA_SUM_OF_SQ_SIG 	= (volatile u32*)FPGA_SUM_OF_SQ_SIG_CH1;
	pFPGA_SUM_OF_SQ_REF 	= (volatile u32*)FPGA_SUM_OF_SQ_REF_CH1;
	pFPGA_SAMPLE_COUNT 		= (volatile u32*)FPGA_SAMPLE_COUNT_CH1;
	pDPRAM_MEAS_FREQ 		= (volatile u32*)DPRAM_MEAS_FREQ_CH1;
	pDPRAM_MEAS_REF 		= (volatile u32*)DPRAM_MEAS_REF_CH1;
	pDPRAM_MEAS_SIG		 	= (volatile u32*)DPRAM_MEAS_SIG_CH1;
	pDPRAM_MODE_SEL			= (volatile u32*)DPRAM_MODE_SEL_CH1;
	pFPGA_FREQUENCY 		+= (ChanIndex * 0x01);
	pFPGA_SAMPLE_COUNT 		+= (ChanIndex * 0x01);
	pFPGA_SUM_OF_SQ_SIG 	+= (ChanIndex * 0x01);
	pFPGA_SUM_OF_SQ_REF 	+= (ChanIndex * 0x01);
	pDPRAM_MEAS_FREQ 		+= (ChanIndex * 0x14);
	pDPRAM_MEAS_REF		 	+= (ChanIndex * 0x14);
	pDPRAM_MEAS_SIG 		+= (ChanIndex * 0x14);
	pDPRAM_MODE_SEL 		+= (ChanIndex * 0x14);

	// Get values from FPGA
	Frequency[ChanIndex]		 = *(volatile int*)	 pFPGA_FREQUENCY;
	Sig_SumOfSq[ChanIndex] 		 = *(volatile float*)pFPGA_SUM_OF_SQ_SIG;
	Ref_SumOfSq[ChanIndex] 		 = *(volatile float*)pFPGA_SUM_OF_SQ_REF;
	Cycle_SampleCount[ChanIndex] = *(volatile int*)	 pFPGA_SAMPLE_COUNT;
//	TODO SEE IF WE CAN TAKE OUT if(!InBITMode)
//	{
		RefFrequency[ChanIndex] = Frequency[ChanIndex];
		RMSFreqIndex[ChanIndex] = RefFrequency[ChanIndex] / 256;
//	}

	// Calculate RMS Value
	if(Cycle_SampleCount[ChanIndex] != 0)
	{
		Sig_RMS[ChanIndex] = sqrtf(Sig_SumOfSq[ChanIndex] / Cycle_SampleCount[ChanIndex]);
		Ref_RMS[ChanIndex] = sqrtf(Ref_SumOfSq[ChanIndex] / Cycle_SampleCount[ChanIndex]);
	}

	// Calculate Signal RMS
	if(Sig_SumOfSq[ChanIndex] < 2.00e+10) // Need to verify this over frequency below 5V
		Sig_RMS[ChanIndex] = 0.0100;
	else
		Sig_RMS[ChanIndex] *= SIG_RMS_SCALE;

	// Adjust for frequency scale
	if(FilterState[ChanIndex] == FILTER_LOW)
		Sig_RMS[ChanIndex] *= RMSFreqScaleFactor[0][RMSFreqIndex[ChanIndex]];
	else
		Sig_RMS[ChanIndex] *= RMSFreqScaleFactor[1][RMSFreqIndex[ChanIndex]];

	// Calculate Reference RMS
	if(Ref_SumOfSq[ChanIndex] < 2.00e+10) // 2.00e+10 is an arbitrary low value < 1V
	{
		Ref_RMS[ChanIndex] = 0.0000;
	}
	else
	{
		if(RefSelState[ChanIndex] == 0)
			Ref_RMS[ChanIndex] *= REF_LO_RMS_SCALE;
		else
			Ref_RMS[ChanIndex] *= REF_HI_RMS_SCALE;
	}
	// Adjust for frequency scale
	if(FilterState[ChanIndex] == FILTER_LOW)
		Ref_RMS[ChanIndex] *= RMSFreqScaleFactor[0][RMSFreqIndex[ChanIndex]];
	else
		Ref_RMS[ChanIndex] *= RMSFreqScaleFactor[1][RMSFreqIndex[ChanIndex]];

	// Output values to User (10mv resolution)
	*pDPRAM_MEAS_REF 	= (u32)(Ref_RMS[ChanIndex] * 100.0);
	*pDPRAM_MEAS_SIG 	= (u32)(Sig_RMS[ChanIndex] * 100.0);
	*pDPRAM_MEAS_FREQ 	= Frequency[ChanIndex];


	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void Calculate_GainValues(void)
{
	volatile u32 		*pFPGA_SIG_GAIN_MULT, *pFPGA_REF_GAIN_MULT, *pFPGA_SYNTH_GAIN_MULT;

	// Calculate pointer for channel
	pFPGA_SIG_GAIN_MULT 	= (volatile u32*)FPGA_SIG_GAIN_MULT_CH1;
	pFPGA_REF_GAIN_MULT 	= (volatile u32*)FPGA_REF_GAIN_MULT_CH1;
	pFPGA_SYNTH_GAIN_MULT 	= (volatile u32*)FPGA_SYNTH_GAIN_MULT_CH1;
	pFPGA_SIG_GAIN_MULT 	+= (ChanIndex * 0x01);
	pFPGA_REF_GAIN_MULT 	+= (ChanIndex * 0x01);
	pFPGA_SYNTH_GAIN_MULT 	+= (ChanIndex * 0x01);

	// Calculate Multipliers
	if(Sig_RMS[ChanIndex] > 0.150)
		SigGainMult[ChanIndex] 		= MAX_SIG_HI_VLL  / Sig_RMS[ChanIndex];
	else
		SigGainMult[ChanIndex] 		= 186.667;
	// Stop multiplier from being to big. MAX_SIG_MULT is 10% greater(arbitrary number) than minimum rms voltage allowed
	if(SigGainMult[ChanIndex] > MAX_SIG_MULT)
		SigGainMult[ChanIndex] = MAX_SIG_MULT;

	if(Ref_RMS[ChanIndex] > 0.150)
		if(Ref_RMS[ChanIndex] > 28.00)
			RefGainMult[ChanIndex] 		= MAX_REF_HI_VRMS / Ref_RMS[ChanIndex];
		else
			RefGainMult[ChanIndex] 		= MAX_REF_LO_VRMS / Ref_RMS[ChanIndex];
	else
		RefGainMult[ChanIndex] 		= 186.667;
	// Stop multiplier from being to big. MAX_REF_MULT is 10% greater(arbitrary number) than minimum rms voltage allowed
	if(RefGainMult[ChanIndex] > MAX_REF_MULT)
		RefGainMult[ChanIndex] = MAX_REF_MULT;

	SynthRefGainMult[ChanIndex] = SigGainMult[ChanIndex]  * 25.0;

	// Output Multipliers to FPGA
	*(volatile float*)pFPGA_SIG_GAIN_MULT 		= SigGainMult[ChanIndex];
	*(volatile float*)pFPGA_REF_GAIN_MULT 		= RefGainMult[ChanIndex];
	*(volatile float*)pFPGA_SYNTH_GAIN_MULT 	= SynthRefGainMult[ChanIndex];

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void Calculate_BkgndDCCal(void)
{
	volatile u32 	*pFPGA_SIN_DC,  *pFPGA_COS_DC;
	int				tempCycleCount;
	int				totalCycleCount;
	int				bkgndSinDCValue, bkgndCosDCValue, dc_calValue;

	// Calculate pointer for channel
	pFPGA_SIN_DC	 		= (volatile u32*)FPGA_BKGND_DC_SIN_CH1;
	pFPGA_COS_DC	 		= (volatile u32*)FPGA_BKGND_DC_COS_CH1;

	pFPGA_SIN_DC 	+=  (ChanIndex * 2);
	pFPGA_COS_DC	+=  (ChanIndex * 2);

	// Get values from FPGA
	bkgndSinDCValue = *(volatile int*) pFPGA_SIN_DC;								// Read accumulated value from RMS_FREQ RAM
	bkgndCosDCValue = *(volatile int*) pFPGA_COS_DC;								// Read accumulated value from RMS_FREQ RAM

	tempCycleCount = (Cycle_SampleCount[ChanIndex] * Frequency[ChanIndex] * 16);	// Calculate the total number of cycles over the minimum 2.68435456 seconds second period
	totalCycleCount = (int)((double)tempCycleCount * 20.48e-6);						// We multiplied by 16 since Cycle_SampleCount[ChanIndex] is over a min:167ms period
																					// and the DC cal accumulated over 16 of these periods.


	if(totalCycleCount != 0)
	{
		// Update the SIN DC Calibration
		dc_calValue = (int)( (double)bkgndSinDCValue / (double)totalCycleCount );		// Divide by the total cycle count
		dc_calValue = dc_calValue / 64;													// TODO What is this?
		*(volatile int*) pFPGA_SIN_DC	= dc_calValue;									// Write to RAM for ADDA to use

		// Update the COS DC Calibration
		dc_calValue = (int)( (double)bkgndCosDCValue / (double)totalCycleCount );		// Divide by the total cycle count
		dc_calValue = dc_calValue / 64;													// TODO What is this?
		*(volatile int*) pFPGA_COS_DC	= dc_calValue;									// Write to RAM for ADDA to use
	}

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void Scale_VelocityOut(void)
{
	float			tempVelocity;
	s32				dpramVelocity;
	volatile u32   *pFPGA_VELOCITY;
	volatile u32   *pDPRAM_VELOCITY;


	// Calculate pointer for channel
	pFPGA_VELOCITY 	= (volatile u32*)FPGA_VELOCITY_CH1;
	pFPGA_VELOCITY 	+= (ChanIndex * 0x01);
	pDPRAM_VELOCITY = (volatile u32*)DPRAM_VELOCITY_CH1;
	pDPRAM_VELOCITY += (ChanIndex * 0x14);

	// Get data
	VelocityFPGA[ChanIndex]	 = *pFPGA_VELOCITY;

	tempVelocity = (float)VelocityFPGA[ChanIndex] / VELOCITY_SCALE;
	dpramVelocity = (s32)(tempVelocity);

	*pDPRAM_VELOCITY = dpramVelocity;


	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void CheckSigLoss(void)
{
	volatile u32    *pFPGA, *pDPRAM_Mode, *pDPRAM_SigValue, *pDPRAM_SigStatus;
	volatile u32    *pFPGA_SigStatus;
	u32				threshold, sigValue, mode;
	u32				maskBitHi, maskBitLo;
	u32				tempStatus;


	// Create proper mask
	maskBitHi = 0x00000001 << ChanIndex;
	maskBitLo = ~maskBitHi;


	// Calculate pointer for channel
	pFPGA 				= (volatile u32*)DPRAM_SIG_LOSS_THRESHOLD_CH1;
	pDPRAM_SigValue		= (volatile u32*)DPRAM_MEAS_SIG_CH1;

	// Get Threshold requested by USER & current signal value
	pFPGA			+= (ChanIndex * 0x14);
	pDPRAM_SigValue	+= (ChanIndex * 0x14);
	threshold 		= *pFPGA;
	sigValue  		= *pDPRAM_SigValue;

	//...................................................................
	// Don't flag a signal loss while in BIT mode (D0 or D3)
	if(WaitTimeBeforeSigLossDetect > 0)
	{
		WaitTimeBeforeSigLossDetect--;
		// Increment to next channel
		ChanIndex++;
		if(ChanIndex >= MAX_CHAN)
		{
			ChanIndex = 0;				// Reset to first channel
			FunctionIndex++;			// Increment to next function
		}
		return;
	}
	//...................................................................
	// Get current signal status
	// Signal Status is bit mapped
	pDPRAM_SigStatus	= (volatile u32*)DPRAM_SIG_LOSS_STATUS_DYNAMIC;
	pFPGA_SigStatus 	= (volatile u32*)FPGA_SIG_LOSS_STATUS;
	// Get mode to mask out SIG Loss when in 2 Wire mode
	pDPRAM_Mode	 = (volatile u32*)DPRAM_MODE_SEL_CH1;
	pDPRAM_Mode	+= (ChanIndex * 0x14);
	mode = *pDPRAM_Mode;

	tempStatus 			= *pDPRAM_SigStatus;
	if((sigValue < threshold) && (mode != TWO_WIRE) )
	{
		*pFPGA_SigStatus = tempStatus | maskBitHi;
	}
	else
	{
		*pFPGA_SigStatus = tempStatus & maskBitLo;
	}

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void CheckRefLoss(void)
{
	volatile u32    *pFPGA, *pDPRAM_RefValue, *pDPRAM_RefStatus;
	volatile u32    *pFPGA_RefStatus;
	u32				threshold, refValue;
	u32				maskBitHi, maskBitLo;
	u32				tempStatus;

	// Create proper mask
	maskBitHi = 0x00000001 << ChanIndex;
	maskBitLo = ~maskBitHi;


	// Calculate pointer for channel
	pFPGA 				= (volatile u32*)DPRAM_REF_LOSS_THRESHOLD_CH1;
	pDPRAM_RefValue		= (volatile u32*)DPRAM_MEAS_REF_CH1;

	// Get Threshold requested by USER & current signal value
	pFPGA			+= (ChanIndex * 0x14);
	pDPRAM_RefValue	+= (ChanIndex * 0x14);
	threshold 		= *pFPGA;
	refValue  		= *pDPRAM_RefValue;

	//...................................................................
	// Don't flag a reference loss while in BIT mode (D0 or D3)
	if(WaitTimeBeforeRefLossDetect > 0)
	{
		WaitTimeBeforeRefLossDetect--;
		// Increment to next channel
		ChanIndex++;
		if(ChanIndex >= MAX_CHAN)
		{
			ChanIndex = 0;				// Reset to first channel
			FunctionIndex++;			// Increment to next function
		}
		return;
	}

	// Get current Reference status
	// Reference Status is bit mapped
	pDPRAM_RefStatus	= (volatile u32*)DPRAM_REF_LOSS_STATUS_DYNAMIC;
	pFPGA_RefStatus 	= (volatile u32*)FPGA_REF_LOSS_STATUS;
	tempStatus 			= *pDPRAM_RefStatus;
	if(refValue < threshold)
	{
		*pFPGA_RefStatus = tempStatus | maskBitHi;
	}
	else
	{
		*pFPGA_RefStatus = tempStatus & maskBitLo;
	}

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void CheckFilterSelection(void)
{
	volatile u32    *pFPGA;
	u32				freqValue;
	u32				numOfCoefficients;


	freqValue = RefFrequency[ChanIndex];
	if((freqValue > 4250) && (FilterState[ChanIndex] == FILTER_LOW))
	{
		// Change filter state
		FilterState[ChanIndex] = FILTER_HIGH;
		//-------------------------------------------------
		// Keep filters in reset until data is transferred
		//-------------------------------------------------
		pFPGA	= (volatile u32*)FPGA_RESET_FILTERS_CH1;
		pFPGA  += ChanIndex;
	   *pFPGA   = 1;

		//-------------------------------------------------
		// Transfer over all SIGNAL Filter coefficient data
		//-------------------------------------------------
		numOfCoefficients = 27;
		pFPGA	= (volatile u32*)FPGA_SigFiltTapCountCh1;
		pFPGA  += ChanIndex;
	   *pFPGA   = numOfCoefficients;

		pFPGA	= (volatile u32*)FPGA_SIGNAL_COEF_CH1;
		pFPGA  += (ChanIndex * 0x0200);
		for(int coefIndex = 0; coefIndex < numOfCoefficients; coefIndex++)
			*pFPGA++ = SignalFilterCoefHiFreq[coefIndex];

		//-------------------------------------------------
		// Transfer over all BIT Filter coefficient data
		//-------------------------------------------------
		numOfCoefficients = 686;
		pFPGA	= (volatile u32*)FPGA_BitFiltTapCountCh1;
		pFPGA  += ChanIndex;
	   *pFPGA   = numOfCoefficients;

		pFPGA	= (volatile u32*)FPGA_BANDPASS_COEF_CH1;
		pFPGA  += (ChanIndex * 0x0400);
		for(int coefIndex = 0; coefIndex < numOfCoefficients; coefIndex++)
			*pFPGA++ = BitFilterCoefHiFreq[coefIndex];

		//-------------------------------------------------
		// Write out frequency
		//-------------------------------------------------
		pFPGA	= (volatile u32*)FPGA_BIT_FREQ_CH1;
		pFPGA  += ChanIndex;
		*pFPGA = 0x00573DD4;	// 650 Hz

		//-------------------------------------------------
		// Allow filters to run
		//-------------------------------------------------
		pFPGA	= (volatile u32*)FPGA_RESET_FILTERS_CH1;
		pFPGA  += ChanIndex;
		*pFPGA  = 0;
	}
	else if((freqValue < 3850) && (FilterState[ChanIndex] == FILTER_HIGH))
	{
		// Change filter state
		FilterState[ChanIndex] = FILTER_LOW;
		//-------------------------------------------------
		// Keep filters in reset until data is transferred
		//-------------------------------------------------
		pFPGA	= (volatile u32*)FPGA_RESET_FILTERS_CH1;
		pFPGA  += ChanIndex;
	   *pFPGA   = 1;

		//-------------------------------------------------
		// Transfer over all SIGNAL Filter coefficient data
		//-------------------------------------------------
		numOfCoefficients = 34;
		pFPGA	= (volatile u32*)FPGA_SigFiltTapCountCh1;
		pFPGA  += ChanIndex;
	   *pFPGA   = numOfCoefficients;

		pFPGA	= (volatile u32*)FPGA_SIGNAL_COEF_CH1;
		pFPGA  += (ChanIndex * 0x0200);
		for(int coefIndex = 0; coefIndex < numOfCoefficients; coefIndex++)
			*pFPGA++ = SignalFilterCoefLoFreq[coefIndex];

		//-------------------------------------------------
		// Transfer over all BIT Filter coefficient data
		//-------------------------------------------------
		numOfCoefficients = 686;
		pFPGA	= (volatile u32*)FPGA_BitFiltTapCountCh1;
		pFPGA  += ChanIndex;
	   *pFPGA   = numOfCoefficients;

		pFPGA	= (volatile u32*)FPGA_BANDPASS_COEF_CH1;
		pFPGA  += (ChanIndex * 0x0400);
		for(int coefIndex = 0; coefIndex < numOfCoefficients; coefIndex++)
			*pFPGA++ = BitFilterCoefLoFreq[coefIndex];

		//-------------------------------------------------
		// Write out frequency
		//-------------------------------------------------
		pFPGA	= (volatile u32*)FPGA_BIT_FREQ_CH1;
		pFPGA  += ChanIndex;
		*pFPGA = 0x03F6AF9F;	// 7560 Hz

		//-------------------------------------------------
		// Allow filters to run
		//-------------------------------------------------
		pFPGA	= (volatile u32*)FPGA_RESET_FILTERS_CH1;
		pFPGA  += ChanIndex;
		*pFPGA  = 0;
	}

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void CheckBkgndDCRegs(void)
{
	volatile u32   *pDPRAM;
	volatile u32   *pFPGA;

	// Calculate pointer for channel (Puts out Sin,Cos all 4 channels)
	pFPGA 	= (volatile u32*)FPGA_BKGND_DC_SIN_CH1;
	pDPRAM 	= (volatile u32*)DPRAM_BKGND_DC_SIN_CH1;

	// Get POINTER
	for(int n = 0; n < MAX_CHAN; n++)
		*pDPRAM++ = *pFPGA++;

	FunctionIndex++;																			// Increment to next function
}
//===============================================================================================================
void EndOfFunctions(void)
{
	FunctionIndex = 0;
}
//===============================================================================================================
//				TEST MODE(D0), BACKGROUND (D2) & D3 BIT MODE FUNCTIONS
//===============================================================================================================
void CheckBITMode(void)
{
	volatile u32    *pDPRAM, *pFPGA;

    // Calculate pointer for channel
	pDPRAM 	= (volatile u32*)DPRAM_BIT_MODE;								// Check if we are in D0 or D3 mode
	//..................................................
	//					D0 mode
	//..................................................
	if( (*pDPRAM & 0x0001)	|| (InD0Tests == TRUE)	)						//  ======= D0 mode =======
	{
		D0BitPtr[D0_BitFunctionIndex]();
	}
	//..................................................
	//					D3 mode
	//..................................................
	else if(*pDPRAM & 0x0008)												//  ======= D3 mode =======
	{
		D3BitPtr[D3_BitFunctionIndex]();
	}
	//..................................................
	//					NO BIT mode
	//..................................................
	else
	{
		if(InD3Tests)
		{
			InD3Tests = FALSE;												// Reset FLAG
			pFPGA 	= (volatile u32*)DPRAM_MODE_SEL_CH1;					// FPGA uses same address
			for(int ChanIndex = 0; ChanIndex < MAX_CHAN; ChanIndex++)
			{
				*pFPGA = UserChanMode[ChanIndex];							//	Restore current user mode
				 pFPGA += 0x14;												// increment to the next channel
			}
		}
		InBITMode = 0;
		pFPGA 	= (volatile u32*)FPGA_IN_BIT_MODE;
		*pFPGA 	= InBITMode;
		FunctionIndex++;													// Increment to next function
	}
}
//===============================================================================================================
void CheckFIFOCriteria(void)
{
	volatile u32   *pDPRAM_FIFO[8];

	// Calculate pointer for channel
	pDPRAM_FIFO[0] 	= (volatile u32*)DPRAM_FIFO_HI_THRESHOLD_CH1;
	pDPRAM_FIFO[1] 	= (volatile u32*)DPRAM_FIFO_LO_THRESHOLD_CH1;
	pDPRAM_FIFO[2] 	= (volatile u32*)DPRAM_FIFO_DELAY_CH1;
	pDPRAM_FIFO[3] 	= (volatile u32*)DPRAM_FIFO_NUM_OF_SAMPLES_CH1;
	pDPRAM_FIFO[4] 	= (volatile u32*)DPRAM_FIFO_DATA_RATE_CH1;
	pDPRAM_FIFO[5] 	= (volatile u32*)DPRAM_FIFO_DATA_TYPE_CH1;
	pDPRAM_FIFO[6] 	= (volatile u32*)DPRAM_FIFO_TRIGGER_SRC_CH1;
	pDPRAM_FIFO[7] 	= (volatile u32*)DPRAM_INTERRUPT_ENABLE_CH1;

	// Calculate pointer for channel
	pDPRAM_FIFO[0]	+= (ChanIndex * 0x10);
	pDPRAM_FIFO[1]	+= (ChanIndex * 0x10);
	pDPRAM_FIFO[2]	+= (ChanIndex * 0x10);
	pDPRAM_FIFO[3]	+= (ChanIndex * 0x10);
	pDPRAM_FIFO[4]	+= (ChanIndex * 0x10);
	pDPRAM_FIFO[5]	+= (ChanIndex * 0x10);
	pDPRAM_FIFO[6]	+= (ChanIndex * 0x10);
	pDPRAM_FIFO[7]	+= (ChanIndex * 0x10);


	// Get FIFO criteria requested by USER
	// Read from user input (serdes) and Write out to hardware
	for(int n = 0; n < 8; n++)
	{
		*pDPRAM_FIFO[n] = *pDPRAM_FIFO[n];	// FPGA Hardware uses the same address
	}

	// Increment to next channel
	ChanIndex++;
	if(ChanIndex >= MAX_CHAN)
	{
		ChanIndex = 0;				// Reset to first channel
		FunctionIndex++;			// Increment to next function
	}
}
//===============================================================================================================
void WaitFor_BitFreqSettled(void)
{
	if(LeftD3Tests == TRUE)
	{
		if(BkgndBitWaitTime < D3_BIT_FREQ_WAIT_TIME)			// wait for 2 seconds
		{
			BkgndBitWaitTime++;
		}
		else
		{
			BkgndBitWaitTime = 0;
			BitFunctionIndex++;									// Increment to next function
		}
	}
	else
	{
		LeftD3Tests = FALSE;
		BitFunctionIndex++;									// Increment to next function
	}
}
//===============================================================================================================
void SetBkgndBitChannel(void)
{
	volatile u32    *pFPGA;
	volatile u32    *pDPRAM;

	if(BkgndBitAngleExpected == 355)							// angles go from 0.5000 to 355.000 in steps of 5 degrees
	{
		if(BkgndBitChanIndex < (MAX_CHAN - 1) )					// Check if we are at the last channel
			BkgndBitChanIndex++;								// If not, go to the next channel
		else
			BkgndBitChanIndex = 0;								// otherwise, go back to the first channel

		 pFPGA  = (volatile u32*)FPGA_BIT_CHAN_SELECT;
		*pFPGA  = BkgndBitChanIndex;							// write out BIT channel to FPGA (selects from mux)
	}

	 pDPRAM = (volatile u32*)DPRAM_BKGND_CAL_CHAN;
	*pDPRAM = (BkgndBitChanIndex + 1);							// Report channel being tested (BIT) to user starting from channel 1
	BitFunctionIndex++;											// Increment to next function
}
//===============================================================================================================
void SetBkgndBitAngle(void)
{
	volatile u32    *pFPGA;
	u32				bit_value;

	pFPGA 	= (volatile u32*)FPGA_ARM_BIT_ANGLE;				// Get angle address

	if(BkgndBitAngleExpected == 355)							// Check if we are at the last angle
		BkgndBitAngleExpected = 0.500;							// If so, reset back to first angle (To avoid 0.000/359.999 issue, set angle to 0.500)
	else if(BkgndBitAngleExpected == 0.500)						// After the first angle,
		BkgndBitAngleExpected = 5;								// we'll do every 5 degrees
	else
		BkgndBitAngleExpected += 5;								// otherwise increment to the next angle

	bit_value = (u32)(BkgndBitAngleExpected / ANGLE_LSB32);		// Get hex value of angle
	*pFPGA = bit_value;											// Output the angle

	BitFunctionIndex++;											// Increment to next function
}
//===============================================================================================================
void WaitForAngleSettled(void)
{
	if(BkgndBitWaitTime < BKGND_CAL_WAIT_TIME)					// wait for 500ms
	{
		BkgndBitWaitTime++;
	}
	else
	{
		BkgndBitWaitTime = 0;
		BitFunctionIndex++;										// Increment to next function
	}

}
//===============================================================================================================
void GetBkgndBitAngle(void)
{

	volatile u32    *pFPGA;
	volatile u32    *pDPRAM1, *pDPRAM2;
	u32				hexValue;

	pFPGA 	= (volatile u32*)FPGA_ANGLE_VALUE_BIT;						// Address of value read in
	pDPRAM1  = (volatile u32*)DPRAM_BKGND_BIT_EXPECTED;
	pDPRAM2  = (volatile u32*)DPRAM_BKGND_BIT_ACTUAL;


	if(AverageBkgndBitAngleIndex < BKGND_CAL_AVG_TIME)
	{
		BkgndBitAngleAccumulated += (*pFPGA);
		AverageBkgndBitAngleIndex++;
	}
	else
	{
		// Send out actual value to USER in hex (They will calculate the proper value)
		hexValue = (u32)(BkgndBitAngleAccumulated / BKGND_CAL_AVG_TIME);					// Hex value of angle or position
		*pDPRAM2 = hexValue;														// For user to read

		*pDPRAM1 = (u32)(BkgndBitAngleExpected / ANGLE_LSB32);					// For user to read (in Hex)
		BkgndBitAngleActual  = ( hexValue * ANGLE_LSB32 );						// For ARM to use
		BkgndBitAngleError   = BkgndBitAngleActual - BkgndBitAngleExpected;
		if( (BkgndBitAngleError > MaxBkgndBitError) | (BkgndBitAngleError < (-MaxBkgndBitError)) )
			BitFailed[BkgndBitChanIndex] = TRUE;

		BkgndBitAngleAccumulated  = 0;
		AverageBkgndBitAngleIndex = 0;
		BitFunctionIndex++;										// Increment to next function
	}
}
//===============================================================================================================
void UpdateBkgndBitStatus(void)
{
	volatile u32    *pDPRAM;
	volatile u32    *pFPGA;
	u32				maskBitHi, maskBitLo;
	u32				tempStatus;

	// Create proper mask
	maskBitHi = 0x00000001 << BkgndBitChanIndex;				// create mask for failure
	maskBitLo = ~maskBitHi;										// create mask for pass

	// BIT Status is bit mapped
	pDPRAM		= (volatile u32*)DPRAM_BIT_STATUS_DYNAMIC;		// Get DPRAM address of status register
	pFPGA		= (volatile u32*)FPGA_BIT_STATUS;				// Get FPGA  address of status register

	tempStatus 	= *pDPRAM;										// Get current value of BIT status register
	if(BitFailed[BkgndBitChanIndex] == TRUE)					// If channel failed,
	{
		*pFPGA = tempStatus | maskBitHi;						// Set BIT status register
		BitFailed[BkgndBitChanIndex] = FALSE;					// Reset flag for next time around
	}
	else
	{
		*pFPGA = tempStatus & maskBitLo;						// Set BIT status register
	}

	BitFunctionIndex++;											// Increment to next function
}
//===============================================================================================================
void EndOfBITFunctions(void)
{
	BitFunctionIndex = 0;
}
//===============================================================================================================
void Init_D0_BitMode(void)	// State 0
{
    u32				tempU32;

	volatile u32    *pFPGA;
	volatile u32 	*pFPGA_SIG_GAIN_MULT,     *pFPGA_REF_GAIN_MULT,     *pFPGA_SYNTH_GAIN_MULT;
	volatile u32	*pFPGA_SIG_GAIN_MULT_BIT, *pFPGA_REF_GAIN_MULT_BIT;
	volatile u32    *pFPGA_Integrator, *pFPGA_Proportinal, *pFPGA_FilterCoef, *pFPGA_FilterGain;

	if(InBITMode == 0)
	{
		InBITMode = 1;														// Let FPGA know we are in BIT mode
		pFPGA 	= (volatile u32*)FPGA_IN_BIT_MODE;							// This will allow the FPGA to choose the correct filter
		*pFPGA 	= InBITMode;												// and set the gain for the SIG & REF accordingly
		WaitTimeBeforeSigLossDetect = LOSS_DETECT_WAITTIME;					// Setup wait count for when we leave BIT mode
		WaitTimeBeforeRefLossDetect = LOSS_DETECT_WAITTIME;					// This should stop false Sig/Ref Loss indications

		// Calculate pointer for channel
		pFPGA_SIG_GAIN_MULT_BIT = (volatile u32*)FPGA_SIG_GAIN_MULT_BIT;	// Point to SigGainMultiplier when in BIT mode
		pFPGA_REF_GAIN_MULT_BIT = (volatile u32*)FPGA_REF_GAIN_MULT_BIT;	// Point to RefGainMultiplier when in BIT mode
		pFPGA_SIG_GAIN_MULT 	= (volatile u32*)FPGA_SIG_GAIN_MULT_CH1;	// Point to start of array for the channel SigGainMultiplier
		pFPGA_REF_GAIN_MULT 	= (volatile u32*)FPGA_REF_GAIN_MULT_CH1;	// Point to start of array for the channel RefGainMultiplier
		pFPGA_SYNTH_GAIN_MULT 	= (volatile u32*)FPGA_SYNTH_GAIN_MULT_CH1;	// Point to start of array for the channel SynthRefGainMultiplier
		pFPGA_Integrator 		= (volatile u32*)FPGA_COEF_INTEGRATOR_CH1;
		pFPGA_Proportinal 		= (volatile u32*)FPGA_COEF_PROPORTIONAL_CH1;
		pFPGA_FilterCoef 		= (volatile u32*)FPGA_COEF_FILTER_CH1;
		pFPGA_FilterGain 		= (volatile u32*)FPGA_GAIN_FILTER_CH1;
		pFPGA 					= (volatile u32*)DPRAM_MODE_SEL_CH1;		// FPGA uses same address

		for(int ChanIndex = 0; ChanIndex < 4; ChanIndex++)
		{
			UserFrequency[ChanIndex] = Frequency[ChanIndex];
			// Set the gain signals
			tempU32 = *pFPGA_SIG_GAIN_MULT_BIT;								// Get the signal gain multiplier for BIT mode
			*(volatile u32*)pFPGA_SIG_GAIN_MULT 	= tempU32;				// write it out to the channel signal gain multiplier
			tempU32 = *pFPGA_REF_GAIN_MULT_BIT;								// Get the reference gain multiplier for BIT mode
			*(volatile u32*)pFPGA_REF_GAIN_MULT 	= tempU32;				// write it out to the channel reference gain multiplier
			tempU32 = *pFPGA_REF_GAIN_MULT_BIT;								// Get the reference gain multiplier for BIT mode
			*(volatile u32*)pFPGA_SYNTH_GAIN_MULT 	= tempU32;				// write it out to the channel synthetic reference gain multiplier
			*pFPGA_Integrator 	= BIT_INTEGRATOR_VALUE;						//
			*pFPGA_Proportinal 	= BIT_PROPORTIONAL_VALUE;					//
			*pFPGA_FilterCoef 	= BIT_FILTER_COEF;							//
			*pFPGA_FilterGain 	= BIT_FILTER_GAIN;							//
			UserChanMode[ChanIndex] = *pFPGA;								// Save current user mode
			*pFPGA				= 0x0000;									// Resolver Mode
			// Increment pointer
			pFPGA_SIG_GAIN_MULT++;											// increment to the next channel
			pFPGA_REF_GAIN_MULT++;											// increment to the next channel
			pFPGA_SYNTH_GAIN_MULT++;										// increment to the next channel
			pFPGA_Integrator++;												// increment to the next channel
			pFPGA_Proportinal++;											// increment to the next channel
			pFPGA_FilterCoef++;												// increment to the next channel
			pFPGA_FilterGain++;												// increment to the next channel
			pFPGA += 0x14;													// increment to the next channel
		}
		InD0Tests = TRUE;
	}
	D0_BitFunctionIndex++;
}
//===============================================================================================================
void WaitFor_D0_FreqSettled(void)	// State 1
{
	if(D0BitFreqWaitTime < D0_BIT_FREQ_WAIT_TIME)							// wait for 1 seconds
	{
		D0BitFreqWaitTime++;
	}
	else
	{
		D0BitFreqWaitTime = 0;
		D0_BitFunctionIndex++;												// Increment to next function
	}

}
//===============================================================================================================
void Set_D0_BitAngle(void)	// State 2
{
	volatile u32    *pDPRAM;
	volatile u32    *pFPGA;
	u32				bit_angle_hex;

	pDPRAM 	= (volatile u32*)DPRAM_BIT_ANGLE;								// Pointer to User BIT Angle
	pFPGA 	= (volatile u32*)FPGA_ARM_BIT_ANGLE;							// Pointer to BIT angle in FPGA

	bit_angle_hex = *pDPRAM;												// Get Bit Angle from user
	*pFPGA = bit_angle_hex;													// Write out to hardware

	D0BitAngleExpected = bit_angle_hex * ANGLE_LSB32;

	D0_BitFunctionIndex++;
}
//===============================================================================================================
void WaitFor_D0_AngleSettled(void)	// State 3
{
	if(D0BitWaitTime < D0_BIT_CAL_WAIT_TIME)								// wait for 500ms
	{
		D0BitWaitTime++;
	}
	else
	{
		D0BitWaitTime = 0;
		D0_BitFunctionIndex++;												// Increment to next function
	}
}
//===============================================================================================================
void Get_D0_BitAngle(void)
{
	volatile u32    *pFPGA;
	double			tempDouble;
	u32				uHexAngle;
	s64				sHexAngle;

	if(AverageD0BitAngleIndex < D0_BIT_CAL_AVG_TIME)
	{
		for(int chanIndex = 0; chanIndex <=3; chanIndex++)
		{
			pFPGA 	  = (volatile u32*)DPRAM_ANGLE_CH1;										// Address of value read in
			pFPGA    += (chanIndex * 0x14);

			uHexAngle = *pFPGA;
			tempDouble = ((double)uHexAngle * ANGLE_LSB32);


			if( (D0BitAngleExpected > 359.9f) && (tempDouble < 0.1f) )
			{
				tempDouble += (double)360.0;
			}
			if( (D0BitAngleExpected < 0.1f) && (tempDouble > 359.9f) )
			{
				tempDouble -= (double)360.0;
			}

			D0BitAngleAccumDouble[chanIndex] += tempDouble;

			sHexAngle =  (s64)uHexAngle;
			D0BitAngleAccumSigned[chanIndex] += sHexAngle;
			//..................................
		}
		AverageD0BitAngleIndex++;
	}
	else
	{
		for(int chanIndex = 0; chanIndex <=3; chanIndex++)
		{
			if( (D0BitAngleExpected > 359.9f) || (D0BitAngleExpected < 0.1f) )
			{
				D0BitAngleActual[chanIndex]  = (D0BitAngleAccumDouble[chanIndex] / (double)D0_BIT_CAL_AVG_TIME);
			}
			else
			{
				D0BitAngleActual[chanIndex] = ((D0BitAngleAccumSigned[chanIndex] / (double)D0_BIT_CAL_AVG_TIME)  * ANGLE_LSB32);
			}
			D0BitAngleError[chanIndex]    = D0BitAngleActual[chanIndex] - D0BitAngleExpected;
			if(D0BitAngleError[chanIndex] > 350.0f)
				D0BitAngleError[chanIndex] -= 360.0f;
			if( (D0BitAngleError[chanIndex] > MaxD0BitError[chanIndex]) | (D0BitAngleError[chanIndex] < (-MaxD0BitError[chanIndex])) )
			{
				D0BitFailed[chanIndex] = TRUE;
			}
			else
				D0BitFailed[chanIndex] = FALSE;
			// Clear out accumulators
			D0BitAngleAccumSigned[chanIndex]  = 0;
			D0BitAngleAccumDouble[chanIndex]  = (double)0.0;
		}
		AverageD0BitAngleIndex 	= 0;
		D0_BitFunctionIndex++;																// Increment to next function
	}
}
//===============================================================================================================
void Update_D0_BitStatus(void)
{
	volatile u32    *pFPGA;
	u32				maskBitHi, maskBitLo;
	u32				tempStatus;

	// BIT Status is bit mapped
	pFPGA		= (volatile u32*)FPGA_BIT_STATUS;						// Get FPGA  address of status register

	tempStatus 	= 0x0000;												// Initialize value of BIT status register

	for(int chanIndex = 0; chanIndex < MAX_CHAN; chanIndex++)
	{
		// Create proper mask
		maskBitHi = 0x00000001 << chanIndex;							// create mask for failure
		maskBitLo = ~maskBitHi;											// create mask for pass
		if(D0BitFailed[chanIndex] == TRUE)								// If channel failed,
		{
			tempStatus |= maskBitHi;									// Set BIT status channel HIGH
			D0BitFailed[chanIndex] = FALSE;								// Reset flag for next time around
		}
		else
		{
			tempStatus &= maskBitLo;									// Set BIT status channel LOW
		}
	}
	*pFPGA = tempStatus;

	D0_BitFunctionIndex++;												// Increment to next function
}
//===============================================================================================================
void EndOf_D0_BITFunctions(void)
{
	volatile u32    *pFPGA;
	volatile u32    *pDPRAM;

	pDPRAM 	= (volatile u32*)DPRAM_BIT_MODE;
	pFPGA 	= (volatile u32*)DPRAM_MODE_SEL_CH1;		// FPGA uses same address

	if((*pDPRAM & 0x0001) == 0x0000)									//  Not in D0 Mode anymore
	{
		for(int ChanIndex = 0; ChanIndex < MAX_CHAN; ChanIndex++)
		{
			*pFPGA = UserChanMode[ChanIndex];							//	Restore current user mode
			 pFPGA += 0x14;												// increment to the next channel
		}
		D0_BitFunctionIndex = 0;
		LeftD0Tests = TRUE;
		InD0Tests   = FALSE;
	}
	else
		D0_BitFunctionIndex = 2;										// Check if we need to set a different angle
}
//===============================================================================================================
void Init_D3_BitMode(void)
{
    u32				tempU32;

	volatile u32    *pFPGA;
	volatile u32 	*pFPGA_SIG_GAIN_MULT,     *pFPGA_REF_GAIN_MULT,     *pFPGA_SYNTH_GAIN_MULT;
	volatile u32	*pFPGA_SIG_GAIN_MULT_BIT, *pFPGA_REF_GAIN_MULT_BIT;
	volatile u32    *pFPGA_Integrator, *pFPGA_Proportinal;

	if(InBITMode == 0)
	{
		InBITMode = 1;														// Let FPGA know we are in BIT mode
		pFPGA 	= (volatile u32*)FPGA_IN_BIT_MODE;							// This will allow the FPGA to choose the correct filter
		*pFPGA 	= InBITMode;												// and set the gain for the SIG & REF accordingly

		WaitTimeBeforeSigLossDetect = LOSS_DETECT_WAITTIME;
		WaitTimeBeforeRefLossDetect = LOSS_DETECT_WAITTIME;
		// Calculate pointer for channel
		pFPGA_SIG_GAIN_MULT_BIT = (volatile u32*)FPGA_SIG_GAIN_MULT_BIT;	// Point to SigGainMultiplier when in BIT mode
		pFPGA_REF_GAIN_MULT_BIT = (volatile u32*)FPGA_REF_GAIN_MULT_BIT;	// Point to RefGainMultiplier when in BIT mode
		pFPGA_SIG_GAIN_MULT 	= (volatile u32*)FPGA_SIG_GAIN_MULT_CH1;	// Point to start of array for the channel SigGainMultiplier
		pFPGA_REF_GAIN_MULT 	= (volatile u32*)FPGA_REF_GAIN_MULT_CH1;	// Point to start of array for the channel RefGainMultiplier
		pFPGA_SYNTH_GAIN_MULT 	= (volatile u32*)FPGA_SYNTH_GAIN_MULT_CH1;	// Point to start of array for the channel SynthRefGainMultiplier
		pFPGA_Integrator 		= (volatile u32*)FPGA_COEF_INTEGRATOR_CH1;
		pFPGA_Proportinal 		= (volatile u32*)FPGA_COEF_PROPORTIONAL_CH1;
		pFPGA 					= (volatile u32*)DPRAM_MODE_SEL_CH1;		// FPGA uses same address

		for(int ChanIndex = 0; ChanIndex < 4; ChanIndex++)
		{
			UserFrequency[ChanIndex] = Frequency[ChanIndex];
			// Set the gain signals
			tempU32 = *pFPGA_SIG_GAIN_MULT_BIT;								// Get the signal gain multiplier for BIT mode
			*(volatile u32*)pFPGA_SIG_GAIN_MULT 	= tempU32;				// write it out to the channel signal gain multiplier
			tempU32 = *pFPGA_REF_GAIN_MULT_BIT;								// Get the reference gain multiplier for BIT mode
			*(volatile u32*)pFPGA_REF_GAIN_MULT 	= tempU32;				// write it out to the channel reference gain multiplier
			tempU32 = *pFPGA_REF_GAIN_MULT_BIT;								// Get the reference gain multiplier for BIT mode
			*(volatile u32*)pFPGA_SYNTH_GAIN_MULT 	= tempU32;				// write it out to the channel synthetic reference gain multiplier
			*pFPGA_Integrator 	= BIT_INTEGRATOR_VALUE;
			*pFPGA_Proportinal 	= BIT_PROPORTIONAL_VALUE;
			UserChanMode[ChanIndex] = *pFPGA;		//	Save current user mode
			*pFPGA				= 0x0000;			//	Resolver
			// Increment pointer
			pFPGA_SIG_GAIN_MULT++;											// increment to the next channel
			pFPGA_REF_GAIN_MULT++;											// increment to the next channel
			pFPGA_SYNTH_GAIN_MULT++;										// increment to the next channel
			pFPGA_Integrator++;												// increment to the next channel
			pFPGA_Proportinal++;											// increment to the next channel
			pFPGA += 0x14;													// increment to the next channel
		}
		InD3Tests   = TRUE;
	}

	D3_BitFunctionIndex++;
}
//===============================================================================================================
void Set_D3_BitAngle(void)
{
	volatile u32    *pFPGA;
	u32				bit_angle_hex;

	pFPGA 	= (volatile u32*)FPGA_ARM_BIT_ANGLE;				// Get angle address

	if(D3BitAngleExpected == 355)								// Check if we are at the last angle
	{
		D3BitAngleExpected = 0.500;								// If so, reset back to first angle (To avoid 0.000/359.999 issue, set angle to 0.500)
	}
	else if(D3BitAngleExpected == 0.500)						// After the first angle,
	{
		D3BitAngleExpected = 5;									// we'll do every 5 degrees
	}
	else
	{
		D3BitAngleExpected += 5;								// otherwise increment to the next angle
	}

	bit_angle_hex = (u32)(D3BitAngleExpected / ANGLE_LSB32);	// Get hex value of angle
	*pFPGA = bit_angle_hex;										// Output the angle

	D3_BitFunctionIndex++;
}
//===============================================================================================================
void WaitFor_D3_FreqSettled(void)
{
	if(D3BitFreqWaitTime < D3_BIT_FREQ_WAIT_TIME)				// wait for 2 seconds
	{
		D3BitFreqWaitTime++;
	}
	else
	{
		D3BitFreqWaitTime = 0;
		D3_BitFunctionIndex++;									// Increment to next function
	}

}
//===============================================================================================================
void WaitFor_D3_AngleSettled(void)
{
	if(D3BitWaitTime < D3_BIT_CAL_WAIT_TIME)					// wait for 500ms
	{
		D3BitWaitTime++;
	}
	else
	{
		D3BitWaitTime = 0;
		D3_BitFunctionIndex++;									// Increment to next function
	}

}
//===============================================================================================================
void Get_D3_BitAngle(void)
{
	volatile u32    *pFPGA;
	u32				hexAngle;

	if(AverageD3BitAngleIndex < D3_BIT_CAL_AVG_TIME)
	{
		for(int chanIndex = 0; chanIndex <=3; chanIndex++)
		{
			pFPGA 	= (volatile u32*)DPRAM_ANGLE_CH1;									// Address of value read in
			pFPGA  += (chanIndex * 0x14);
			D3BitAngleAccumulated[chanIndex] += (*pFPGA);
		}
		AverageD3BitAngleIndex++;
	}
	else
	{
		for(int chanIndex = 0; chanIndex <=3; chanIndex++)
		{
			hexAngle  = (u32)(D3BitAngleAccumulated[chanIndex] / D3_BIT_CAL_AVG_TIME);		// Hex value of angle
			D3BitAngleActual[chanIndex]  = ( hexAngle * ANGLE_LSB32 );
			D3BitAngleError[chanIndex]    = D3BitAngleActual[chanIndex] - D3BitAngleExpected;
			if( (D3BitAngleError[chanIndex] > MaxD3BitError[chanIndex]) | (D3BitAngleError[chanIndex] < (-MaxD3BitError[chanIndex])) )
				D3BitFailed[chanIndex] = TRUE;
			D3BitAngleAccumulated[chanIndex]  = 0;
		}
		AverageD3BitAngleIndex = 0;

		if(D3BitAngleExpected == 355)
			D3_BitFunctionIndex++;																// Increment to next function
		else
			D3_BitFunctionIndex = 2;	//	Set_D3_BitAngle
	}
}
//===============================================================================================================
void Update_D3_BitStatus(void)
{
	volatile u32    *pDPRAM;
	volatile u32    *pFPGA;
	u32				maskBitHi, maskBitLo;
	u32				tempStatus;
	u32				bit_mode;

	// BIT Status is bit mapped
	pDPRAM		= (volatile u32*)DPRAM_BIT_STATUS_DYNAMIC;		// Get DPRAM address of status register
	pFPGA		= (volatile u32*)FPGA_BIT_STATUS;				// Get FPGA  address of status register

	tempStatus 	= 0x0000;										// Initialize value of BIT status register

	for(int chanIndex = 0; chanIndex < MAX_CHAN; chanIndex++)
	{
		// Create proper mask
		maskBitHi = 0x00000001 << chanIndex;					// create mask for failure
		maskBitLo = ~maskBitHi;									// create mask for pass
		if(D3BitFailed[chanIndex] == TRUE)						// If channel failed,
		{
			tempStatus |= maskBitHi;							// Set BIT status channel HIGH
			D3BitFailed[chanIndex] = FALSE;						// Reset flag for next time around
		}
		else
		{
			tempStatus &= maskBitLo;							// Set BIT status channel LOW
		}
	}
	*pFPGA = tempStatus;

	// Clear out D3 Bit to let User know we are finished.
	pDPRAM 	 = (volatile u32*)DPRAM_BIT_MODE;
	bit_mode = *pDPRAM;
	*pDPRAM  = (bit_mode & 0xFFF7);									// Reset D3 bit

	D3_BitFunctionIndex++;											// Increment to next function
}
//===============================================================================================================
void EndOf_D3_BITFunctions(void)
{
	volatile u32    *pFPGA;

	pFPGA 	= (volatile u32*)DPRAM_MODE_SEL_CH1;					// FPGA uses same address

	for(int ChanIndex = 0; ChanIndex < MAX_CHAN; ChanIndex++)
	{
		*pFPGA = UserChanMode[ChanIndex];							//	Restore current user mode
		 pFPGA += 0x14;												// increment to the next channel
	}

	D3_BitFunctionIndex = 0;
	LeftD3Tests = TRUE;
}
//===============================================================================================================
void WaitForDataReady(void)
{
	unsigned short	 timeout = 0;
	unsigned short	 sm_done = 0;
	static u32		 NumOfTimeouts = 0;

	timeout = 0;
	do
	{
		sm_done = Xil_In32(FPGA_SM_DONE);
		timeout++;
	}while((sm_done == 0) && (timeout < 1000));
	if(timeout >= 1000)
		NumOfTimeouts++;
	Xil_Out32(FPGA_CLR_DONE, 0x0001);			// Clear SM_Done
	Xil_Out32(FPGA_CLR_DONE, 0x0000);			// Release Clear
}
//===============================================================================================================


