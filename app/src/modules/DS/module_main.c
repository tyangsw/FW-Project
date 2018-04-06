/*
 * DSx module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Local functions */
static void init(void);
static void load_params(void);
static void Load_DigitalController(u16);
static void load_caldata(void);

// Main loop
static void	GetFixedModeScale(void);
#ifdef tempout
static void	CheckVrefForRatioMode(void);
#endif
// DPRAM Routines
static void CheckPowerEnable(void);
static void CheckSynRslMode(void);
static void GetCurrentMeas(void);
static void CheckChanStatusEnable(void);
static void CheckThresholds(void);
static void CheckVLLChange(void);
static void CheckFreqChange(void);
static void CheckMultispeed(void);
static void CalculateVelocity(void);
static void CheckDebug(void);
static void OffsetForWrap(void);
static void	CheckExpectedRef(void);
static void CheckTestGenFrequency(void);
static void	CheckWrapBandwidth(void);
static void Calculate_GainValues(void);
static void EndOfDpramFunctions(void);

// Bit Functions
static void D2Test(void);
static void D3Test(void);
static void EndOfBitFunctions(void);

// Background Calibration
static void CheckBackgroundCal(void);
static void InitBkgrndAngleCal(void);
static void AverageAngleError(void);
static void EndOfBkgrndAngleFunctions(void);

/* Global variables */
static bool mod_init = FALSE;
static u8 MaxChannels = 0;
static u16 PowerDelay[NUM_CHANNELS]={0};
static u16 BkgndChan = 0;
static u16 Bkgnd_Func_Index = 0;
static float SigGainMult[NUM_CHANNELS]={0};
static float RefGainMult[NUM_CHANNELS]={0};
static float SynthRefGainMult[NUM_CHANNELS]={0};



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DS Global Variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HOST_REGS 	*pHostRegs = (HOST_REGS *)HOST_REGS_BASE;
HPS_REGS  	*pHpsRegs  = (HPS_REGS  *)HPS_REGS_BASE;
CAL_REGS	*pCalRegs  = (CAL_REGS  *)MODULE_CAL_BASE;

static u32		Dpram_Func_Index	= 0;     // Holds index for function pointer

// For Bit
static bool 	D3Enabled = FALSE;
static u32		Bit_Func_Index		= 0;
static u32 		BitStatusWord   	= 0;

static bool NewRefVltg_flg[NUM_CHANNELS] = {FALSE};
static bool NewVoltage_flg[NUM_CHANNELS] = {FALSE};
static bool NewFrequency_flg[NUM_CHANNELS] = {FALSE};

// Background Angle calibration
static float BkgndAccum = 0;
static u16 BkgndSampleCount = 0;
static u16 BkgndIterationCount = 0;

static s32 DigitalControllerCoeff_DS8[40] = {
		1087240432,
		-1113651312,
		-2147483648,
		1065353216,
		0,
		0,
		0,
		0,
		1065353216,
		1066684513,
		-1103864397,
		-1074446024,
		1063956807,
		0,
		0,
		0,
		1065353216,
		1073716378,
		-1082181323,
		-1073798093,
		1065240713,
		0,
		0,
		0,
		1065353216,
		1073659337,
		-1082293624,
		-1073782829,
		1065278343,
		0,
		0,
		0,
		1065353216,
		-2147483648,
		-2147483648,
		0,
		0,
		0,
		0,
		0

};

static s32 DigitalControllerCoeff_DSE[40] = {
		1060392954,
		1052019484,
		-2147483648,
		1065353216,
		0,
		0,
		0,
		0,
		1065353216,
		1068897092,
		-1091171437,
		-1074572128,
		1063733698,
		0,
		0,
		0,
		1065353216,
		1073730349,
		-1082153382,
		-1073802279,
		1065232395,
		0,
		0,
		0,
		1065353216,
		1073600972,
		-1082410945,
		-1074515607,
		1063873181,
		0,
		0,
		0,
		1065353216,
		-2147483648,
		-2147483648,
		0,
		0,
		0,
		0,
		0
};

// EXTERNAL VARIABLES
//extern u32 FIXEDMODE_VLL_SCALE[2];	// [HV/LV]
//static u32 FIXEDMODE_VLL_SCALE[2] = {202533,798801};		// [HV/LV]
/**************************** Exported functions ****************************/

void module_init(void)
{
    /* Initialize module */
    init();
}


void (*DPRamfunctionPtr[])(void) =
{
	CheckPowerEnable,
	GetCurrentMeas,
	CheckChanStatusEnable,
	CheckThresholds,
// moving to main loop for now	CheckVrefForRatioMode,
	CheckVLLChange,
	CheckFreqChange,
	CheckMultispeed,
	CheckSynRslMode,
	CalculateVelocity,
	CheckDebug,
	OffsetForWrap,
	CheckExpectedRef,
	CheckTestGenFrequency,
	CheckWrapBandwidth,
	Calculate_GainValues,
	EndOfDpramFunctions
};

void (*BitFunctionPtr[])(void) =
{
	D2Test,
	D3Test,
    EndOfBitFunctions
};

void(*BkgrndAngleCorrPtr[])(void) =
{
	InitBkgrndAngleCal,
	AverageAngleError,
	EndOfBkgrndAngleFunctions
};

void module_main(void)
{
    if (mod_init)
    {
    	GetFixedModeScale();

#ifdef tempout
    	CheckVrefForRatioMode();
#endif

    	// Run normal operation routines
		DPRamfunctionPtr[Dpram_Func_Index]();

		// For background testing
		BitFunctionPtr[Bit_Func_Index]();

		CheckBackgroundCal();
    }
}

/*
 *  This function is to check the RMS measurement of the reference against the expected reference and
 *   create a scale factor for fixed mode to keep the reference voltage outputting at the expected voltage.
 */
static void GetFixedModeScale(void)
{
		union {
			 u32 half[2];
			 u64 full;
		 } SumOfSquares;

		static u32 prevExpVll[NUM_CHANNELS] = {0};
		static u8 ChanDelay[NUM_CHANNELS] = {0};
		s32 FixedModeScale = 0;

		for(u8 ch=0; ch<MaxChannels*2; ch=ch+2)
		{
			u32 tmpMode	 = pHostRegs->ds_OutputMode[ch];

			// Check if channel is powered off
//ra			if ( !(pCommon->ps_enable & (1<<(ch/2))) )
//ra			{
//ra				FixedModeScale = 0x02000000;  									// Default Small Ratio
//ra				pHpsRegs->Wr_FixedModeScale[ch/2] = FixedModeScale;				// Write default ratio
//ra				PowerDelay[ch/2] = 0;											// Initialize power on delay
//ra			}
//ra			else
//ra			{	// Do calculations for fixed mode
				u8 CheckRmsStored = pHpsRegs->Rd_RefRmsStrd[ch/2];
				if (CheckRmsStored & (1<<(ch/2)) )
					ChanDelay[ch/2]++;

				if (ChanDelay[ch/2] == 2)
				{
					ChanDelay[ch/2] = 0;

					if(PowerDelay[ch/2]==0)
					{
						u32 tmpExpVll = pHostRegs->ds_Voltage[ch/2];
						// Check if VLL setting has changed
						if (tmpExpVll != prevExpVll[ch/2])
						{
							prevExpVll[ch/2] = tmpExpVll;			// Update previous VLL reading

							// ExpVll/(2600) or (ExpVll/9000)
							float Ratio_flt = 0;
							if(pHpsRegs->HVLV_sel==0)				// 0:HV module, 1:LV module
								Ratio_flt = (float)tmpExpVll/9000;
							else
								Ratio_flt = (float)tmpExpVll/2600;
							Ratio_flt = Ratio_flt * 0x40000000;
							pHpsRegs->Wr_VLLScale[ch] = 0x40000000;//(u32)Ratio_flt;						// Write new scale
							usleep(20000);
						}
						else
						{	// If set voltage did not change

							if(tmpMode==0) // 0=Ratio Mode
							{
								// Ratio = ExpVll / ExpRef
								u32 tmpExpRef = pHostRegs->ExpectedReference[ch/2];
								float Ratio = ((float)tmpExpVll/(float)tmpExpRef);
								FixedModeScale = (u32)(0x40000000 * Ratio);

								// Check Limits
								if( FixedModeScale>0x02000000 && FixedModeScale<0x7FFFFFFF)
									pHpsRegs->Wr_FixedModeScale[ch/2] = FixedModeScale;

							}
							else
							{   // 1 = Fixed Mode

								if ( (pCommon->ps_enable & (1<<(ch/2))) )
								{
									float RMSVal;
									SumOfSquares.half[0] = pHpsRegs->Rd_RefSumOfSquares[ch+0];	// Get lower half of 56bit accumulator
									SumOfSquares.half[1] = pHpsRegs->Rd_RefSumOfSquares[ch+1];	// Get upper half of 56bit accumulator
									RMSVal = (float)SumOfSquares.full;							// Assign accumulator to float
									u32 NumOfSamples = pHpsRegs->Rd_PeriodCnt[ch/2];			// Get number of samples
									RMSVal = RMSVal / (float)NumOfSamples;						// Divide by number of samples
									RMSVal = sqrt(RMSVal);										// Take square root
									RMSVal = RMSVal * 0.0025749679528444999244302883404332; 	// Calculated this 0.00257 constant by taking the 11500 that was

									if( (pHpsRegs->ExtRefSel&(1<<ch)) == 1)						// See which reference circuit is being used (0:0>=X>=28V, 1:28V>X>=115V)
										RMSVal = RMSVal/4;										// Scale down by ~3.9 if using low voltage reference reading

									float Ratio = ((float)tmpExpVll/RMSVal);
									FixedModeScale = (u32)(0x40000000 * Ratio);

									// Check Limits
									if( FixedModeScale>0x02000000 && FixedModeScale<0x7FFFFFFF)
										pHpsRegs->Wr_FixedModeScale[ch/2] = FixedModeScale;
								}
							}
						}
						//ra					}
						//ra					else
						//ra						PowerDelay[ch/2]--;
				}
			}
		}

}

#ifdef tempout
static void CheckVrefForRatioMode(void)
{
//	static u32 prevRefVoltage[NUM_CHANNELS] = {0};
	static u32 prevExpRef[NUM_CHANNELS] = {0};
	static u32 prevMode[NUM_CHANNELS] = {0};
	static u32 RefRatio[NUM_CHANNELS] ={0};
	static u32 prevRefRatio[NUM_CHANNELS] ={0};
	static u32 prevExpVll[NUM_CHANNELS] ={0};


//	u32 tmpRefVlt = 0;
//	u32 tmpRefScaled = 0;
//	u64 NewRatio = 0;
	u32 tmpExpRef = 0;
	u32 tmpMode = 0;
	u32 tmpExpVll = 0;

	float Ratio_flt = 0;

	for (u8 ch = 0; ch < MaxChannels; ch++)
	{
		tmpExpRef = pHostRegs->ExpectedReference[ch];
//		tmpRefVlt = pHpsRegs->Rd_RefVoltage[ch];
		tmpMode	  = pHostRegs->ds_OutputMode[ch];
		tmpExpVll = pHostRegs->ds_Voltage[ch];

		// Check if fixed mode
		if (tmpMode==1) // 1=Fixed Mode
		{
/*			if(PowerDelay[ch]>0)
				RefRatio[ch] = 0;
			else
				RefRatio[ch] = RefRatio[ch] + (0x3216-RefRatio[ch])/4;
*/
			if( (abs(RefRatio[ch]-prevRefRatio[ch])>10) || (tmpMode!=prevMode[ch]) ||  (tmpExpVll!=prevExpVll[ch])  )
			{
				u8 modType = pHpsRegs->HVLV_sel;


				RefRatio[ch] = tmpExpVll * FIXEDMODE_VLL_SCALE[modType];	//202533; // Fixed mode
				RefRatio[ch] = RefRatio[ch] >> 14;
//				NewRatio = (u64)RefRatio[ch];


				prevMode[ch] = tmpMode;											// Update previous mode
				prevExpVll[ch] = tmpExpVll;										// Update previous VLL setting
				prevRefRatio[ch] = RefRatio[ch];								// Update previous ratio

				// ExpVll/(2600) or (ExpVll/9000)
				Ratio_flt = (float)tmpExpVll/2600;
				Ratio_flt = Ratio_flt * 0x40000000;
				pHpsRegs->Wr_VLLScale[ch] = (u32)Ratio_flt;						// Write new scale

			}
		}
		else
		{
			if ( (tmpExpRef!=prevExpRef[ch]) || (tmpMode!=prevMode[ch]) || (tmpExpVll!=prevExpVll[ch]) ) 		// Check for 10mV change
			{
				prevMode[ch] = tmpMode; 										// Update previous mode
				prevExpVll[ch] = tmpExpVll;										// Update previous VLL setting
//				u8 modType = pHpsRegs->HVLV_sel;
				if (pHostRegs->ds_OutputMode[ch] == 0)	//  0=Ratio Mode
				{
					NewRefVltg_flg[ch] = TRUE;									// Set flag for BIT
					prevExpRef[ch] = tmpExpRef;									// Update previous expected ref
//					prevRefVoltage[ch] = tmpRefVlt; 							// Update previous reading

/*					tmpExpVll = tmpExpVll << 14;								// Scale up for diving integers later
					RefRatio[ch] = tmpExpVll / pHostRegs->ExpectedReference[ch];// Measured/Expected (0x3216 = gain of 1)
					if (modType==0)
						NewRatio = HV_VLL_OUTPUT_SCALE*(u64)RefRatio[ch];			// Scale ratio
					else
						NewRatio = LV_VLL_OUTPUT_SCALE*(u64)RefRatio[ch];			// Scale ratio
					NewRatio = NewRatio >> 14;									// Normalize
*/
					// ExpVll/(2600) or (ExpVll/9000)
					Ratio_flt = (float)tmpExpVll/2600;
					Ratio_flt = Ratio_flt * 0x40000000;


				}
				else
				{
/*					RefRatio[ch] = tmpExpVll *  FIXEDMODE_VLL_SCALE[modType];	// Fixed ratio
					RefRatio[ch] = RefRatio[ch] >> 14;
					NewRatio = (u32)RefRatio[ch];
*/				}

				pHpsRegs->Wr_VLLScale[ch] = (u32)Ratio_flt;							// Write new scale
			}
		}
	}

}

#endif

// Module Functions
static void CheckPowerEnable(void)
{
	static u32 pwrEnable = 0;
	u32 tmpPwrEn = pCommon->ps_enable;

	if(pwrEnable != tmpPwrEn)
	{


		#if MODULE_DSJ
				u8 PwrChange = pwrEnable ^ tmpPwrEn; 							// Get bits that changed
				for(u32 ch=0 ; ch<MaxChannels ; ch++)
				{
					u8 currPwr = (PwrChange >> ch) & 0x01;						// Get specific channel that changed
					u8 prevEnable = (pwrEnable >> ch) & 0x01;					// Get channel's previous power enable

					if(currPwr==1)
					{
						if (prevEnable==0)										// See if power was previously off and wants to be turned on
						{
							pHpsRegs->Wr_ChanProgram = ch;						// Tell FPGA which channel we're programming
							pHpsRegs->Pwr_En = (pHpsRegs->Pwr_En | (1<<ch) );	// Turn on power for specific channel

							usleep(300000);										// @@@ fixme - not sure if this delay is needed
							program_cpld(CPLD_1);								// Program CPLD
						}
						else													// Power was on and wants to shut off
							pHpsRegs->Pwr_En = (pHpsRegs->Pwr_En & ~(1<<ch) );	// Clear power for specific channel
					}
				}
		#else
				pHpsRegs->Pwr_En = tmpPwrEn;									// Power on module
		#endif
		pwrEnable = tmpPwrEn; 													// Update previous variable
	}

	// Go to next function
	Dpram_Func_Index++;
}

/*
 * This will see if user changed the mode
 */
static void CheckSynRslMode(void)
{
	static u32 SynRslMode[NUM_CHANNELS] = {0};
	u32 tmpSynEn = 0;
	u8 WriteMode = FALSE;

	for(u32 ch=0 ; ch<MaxChannels ; ch++)
	{
		if (SynRslMode[ch] != pHostRegs->SynRsl_sel[ch])
		{
			WriteMode = TRUE;
			SynRslMode[ch] = pHostRegs->SynRsl_sel[ch];

			if(SynRslMode[ch] == 3)		   // 1=Enables calculations to be done for synchro mode
				tmpSynEn |= (1<<ch);
			else
				tmpSynEn &= ~(1<<ch);
		}
	}

	if (WriteMode==TRUE)
		pHpsRegs->SynchroEnable = tmpSynEn;

	// Go to next function
	Dpram_Func_Index++;
}

/*
 * This will take the square root of the FPGA current measurement to complete the RMS calculation
 */
static void GetCurrentMeas(void)
{
	#if MODULE_DS8
	{
		u32 CurrentRd = 0;
		double dbl_CurrentRd = 0;
		double SqrRootSin = 0;
		double SqrRootCos = 0;
		double SqrRootTotal = 0;
		u8 chan = 0;

		for (u16 i=0 ; i<(MaxChannels*2) ; i=i+2)
		{
			CurrentRd = pHpsRegs->RdCurrent[i];
			dbl_CurrentRd = (double)CurrentRd;
			SqrRootSin = sqrt(dbl_CurrentRd);
			SqrRootSin = SqrRootSin*CURRENT_AD_LSB_DS8;
			SqrRootSin = SqrRootSin*1000;
			pHostRegs->RmsCurrEachSide[i] = (u32)SqrRootSin; 	 // Write independent current

			CurrentRd = pHpsRegs->RdCurrent[i+1];
			dbl_CurrentRd = (double)CurrentRd;
			SqrRootCos = sqrt(dbl_CurrentRd);
			SqrRootCos = SqrRootCos*CURRENT_AD_LSB_DS8;
			SqrRootCos = SqrRootCos*1000;
			pHostRegs->RmsCurrEachSide[i+1] = (u32)SqrRootCos; // Write independent current

			// Write total current
			SqrRootTotal = SqrRootSin + SqrRootCos;
			pHostRegs->meas_Current[chan] = (u32)SqrRootTotal;
			chan++;

		}
	}
	#else //if MODULE_DSE or other  @@@ todo - Make LSB an ifdef instead.
	{
		u32 CurrentRd = 0;
		double dbl_CurrentRd = 0;
		double SqrRootSin = 0;
		double SqrRootCos = 0;
		double SqrRootTotal = 0;
		u8 chan = 0;

		for (u16 i=0; i<(MaxChannels*2); i=i+2)
		{
			CurrentRd = pHpsRegs->RdCurrent[i];
			dbl_CurrentRd = (double)CurrentRd;
			SqrRootSin = sqrt(dbl_CurrentRd);
			SqrRootSin = SqrRootSin*CURRENT_AD_LSB_DSE;
			SqrRootSin = SqrRootSin*1000;
			pHostRegs->RmsCurrEachSide[i] = (u32)SqrRootSin;// Write independent current

			CurrentRd = pHpsRegs->RdCurrent[i+1];
			dbl_CurrentRd = (double)CurrentRd;
			SqrRootCos = sqrt(dbl_CurrentRd);
			SqrRootCos = SqrRootCos*CURRENT_AD_LSB_DSE;
			SqrRootCos = SqrRootCos*1000;
			pHostRegs->RmsCurrEachSide[i+1] = (u32)SqrRootCos;// Write independent current

			// Write total current
			SqrRootTotal = SqrRootSin + SqrRootCos;
			pHostRegs->meas_Current[chan] = (u32)SqrRootTotal;
			chan++;

		}
	}

	#endif

	// Go to next function
	Dpram_Func_Index++;

}


/*
 * This will see if the user wants to enable/disable the status registers
 */
static void CheckChanStatusEnable(void)
{
	static u32 StatusEn = 0;
	if(pCommon->ch_status_en != StatusEn)
	{
		StatusEn = pCommon->ch_status_en;
		pHpsRegs->ChanStatusEnable = StatusEn;
	}

	// Go to next function
	Dpram_Func_Index++;
}


/*
 *  Check the threshold s that the user has written and apply them to FPGA if they have changed
 */
static void CheckThresholds(void)
{
	static u32 sigThresh[NUM_CHANNELS]  = {0};
	static u32 refThresh[NUM_CHANNELS]  = {0};
	static u32 currThresh[NUM_CHANNELS] = {0};

	for(u32 ch=0 ; ch<MaxChannels ; ch++)
	{
		if (sigThresh[ch] != pHostRegs->sig_Threshold[ch])
		{
			sigThresh[ch] = pHostRegs->sig_Threshold[ch];
			pHpsRegs->Wr_SigLossThreshold[ch] = sigThresh[ch];
		}

		if (refThresh[ch] != pHostRegs->ref_Threshold[ch])
		{
			refThresh[ch] = pHostRegs->ref_Threshold[ch];
			pHpsRegs->Wr_RefLossThreshold[ch] = refThresh[ch];
		}

		if (currThresh[ch] != pHostRegs->cur_Threshold[ch])
		{
			currThresh[ch] = pHostRegs->cur_Threshold[ch];

			// Scale current threshold by conversion factor
			u32 tmpScale = 0;
			#if MODULE_DS8
				tmpScale = 487915; // 7.445 * (2^16) based on lsb weight (7.445 per lsb => [(1/134.3 per bit) x 1000] )
			#elif MODULE_DSE
				tmpScale = 976129; // 14.894 * (2^16) based on lsb weight (7.445 per lsb => [(1/67.1 per bit) x 1000] )
			#else
				tmpScale = 976129; // 14.894 * (2^16) based on lsb weight (7.445 per lsb => [(1/67.1 per bit) x 1000] )
			#endif

			u32 tmpCurrVal = currThresh[ch] * tmpScale;
			tmpCurrVal = tmpCurrVal >> 16;
			pHpsRegs->Wr_CurrentThreshold[ch]=tmpCurrVal;
		}
	}
	// Go to next function
	Dpram_Func_Index++;
}


/*
 * Currently used to set flag if VLL setting changed
 */
static void CheckVLLChange(void)
{

	static u32 currentVLLVoltage[NUM_CHANNELS] = {0};

	for(u8 ch=0; ch<MaxChannels; ch++)
	{
		if(currentVLLVoltage[ch] != pHostRegs->ds_Voltage[ch])
		{
			currentVLLVoltage[ch] = pHostRegs->ds_Voltage[ch];		// Update variable
			pHpsRegs->Wr_DSVLL[ch]= currentVLLVoltage[ch]; 			// Write to module
			NewVoltage_flg[ch] = TRUE;								// set flag
		}
	}

	// Go to next function
	Dpram_Func_Index++;

}

/*
 *  Currently used to set flag if frequency changed
 */
static void CheckFreqChange(void)
{
	static u32 currentFrequency[NUM_CHANNELS] = {0};
	u32 FreqDiff = 0;

	for(u8 ch=0; ch<MaxChannels; ch++)
	{
		FreqDiff = abs(currentFrequency[ch]-pHpsRegs->Rd_Frequency[ch]);
		if(FreqDiff > 20) // Check for a 20Hz change in the frequency measurement
		{
			currentFrequency[ch]=pHpsRegs->Rd_Frequency[ch];
			NewFrequency_flg[ch] = TRUE;
		}
	}

	// Go to next function
	Dpram_Func_Index++;
}

/*
 *  This will check if the multispeed setting has changed and write it
 */
static void CheckMultispeed(void)
{
	static u32 currentMultispdRatio = {0};

	if (currentMultispdRatio != pHostRegs->DSRatioMode)
	{
		currentMultispdRatio =  pHostRegs->DSRatioMode;
		pHpsRegs->Wr_Multispeed = currentMultispdRatio;
	}

	// Go to next function
	Dpram_Func_Index++;
}


/*
 *  This will scale and display the velocity
 */
static void CalculateVelocity(void)
{
	s32 tmpVel_s32 = 0;
	float tmpVel_flt = 0;

	for(u8 ch=0; ch<MaxChannels; ch++)
	{
		tmpVel_s32 = pHpsRegs->Rd_RawVelocity[ch];
		tmpVel_flt = (float)tmpVel_s32 / VELOCITY_SCALE;
		pHostRegs->wrap_Velocity[ch] = (s32)tmpVel_flt;
	}

	// Go to next function
	Dpram_Func_Index++;

}

/*
 * Used for debugging module
 */
void CheckDebug(void)
{
	if (pHostRegs->reset_Fifo==1)
	{
		pHpsRegs->Rst_Fifo = 1;
		pHostRegs->reset_Fifo = 0;
		printf("Reset Fifo - Clear Display");
	}

	static u32 readDigCtrl = 0;
	if(pHostRegs->OpenDigCtrlLoop != readDigCtrl)
	{
		readDigCtrl = pHostRegs->OpenDigCtrlLoop;
		pHpsRegs->OpenDigCtrlLoop = readDigCtrl;
	}

	static u32 readTestGenEn = 0;
	if(pHostRegs->EnableTestGen != readTestGenEn)
	{
		readTestGenEn = pHostRegs->EnableTestGen;
		pHpsRegs->EnableTestGen = readTestGenEn;
	}


	int tempData = 0;
	if (pHostRegs->enable_Fifo1Capture == 1)
	{
		pHostRegs->enable_Fifo1Capture=0;

		for(u32 i=0; i<2048; i++ )
		{
			tempData = (int)(pHpsRegs->Rd_fifo1Data);
			printf("%d", tempData);
			if(i!=2047)
				printf("\n");

		}
	}

	if (pHostRegs->enable_Fifo2Capture == 1)
	{
		pHostRegs->enable_Fifo2Capture=0;

		for(u32 i=0; i<2048; i++ )
		{
			tempData = (int)(pHpsRegs->Rd_fifo2Data);
			printf("%d", tempData);
			if(i!=2047)
				printf("\n");
		}
	}

	static u32 readPWMgainCh1 = 0x0000;
	static u32 readPWMgainCh2 = 0x0000;
	static u32 readPWMgainCh3 = 0x0000;

	if(pHostRegs->PwmScaleCh1 != readPWMgainCh1)
	{
		readPWMgainCh1 = pHostRegs->PwmScaleCh1;
		pHpsRegs->PwmScaleCh1 = readPWMgainCh1;
	}
	if(pHostRegs->PwmScaleCh2 != readPWMgainCh2)
	{
		readPWMgainCh2 = pHostRegs->PwmScaleCh2;
		pHpsRegs->PwmScaleCh2 = readPWMgainCh2;
	}
	if(pHostRegs->PwmScaleCh3 != readPWMgainCh3)
	{
		readPWMgainCh3 = pHostRegs->PwmScaleCh3;
		pHpsRegs->PwmScaleCh3 = readPWMgainCh3;
	}


	////////////////////////////////////////////////
	static u32 readOpenPhaseLock = 0;
	if(pHostRegs->dbg_OpenPhaseLock != readOpenPhaseLock)
	{
		readOpenPhaseLock = pHostRegs->dbg_OpenPhaseLock;
		pHpsRegs->dbg_OpenPhaseLock = readOpenPhaseLock;
	}

	 ////////////////////////////////////////////////
	static u32 readPausePhaseCorr = 0;
	if(pHostRegs->dbg_PausePhaseCorr != readPausePhaseCorr)
	{
		readPausePhaseCorr = pHostRegs->dbg_PausePhaseCorr;
		pHpsRegs->dbg_PausePhaseCorr = readPausePhaseCorr;
	}

	////////////////////////////////////////////////
	static u32 readDbgChan = 0;
	if(pHostRegs->dbg_Chan != readDbgChan)
	{
		readDbgChan = pHostRegs->dbg_Chan;
		pHpsRegs->dbg_Chan = readDbgChan;
	}

	////////////////////////////////////////////////
	static u32 readArmDelAmt= 0;
	if(pHostRegs->wr_ArmDelAmt != readArmDelAmt)
	{
		readArmDelAmt = pHostRegs->wr_ArmDelAmt;
		pHpsRegs->wr_ArmDelAmt = readArmDelAmt;
	}

	// Go to next function
	Dpram_Func_Index++;
}

/*
 *  Will calculate the number of cycles needed to accumulate no more than 65535 samples.
 *  This will also take the accumulated offset and divide it by the number of samples actually taken.
 */
void OffsetForWrap(void)
{
	u32 Frequency = 0;
	u32 ScaledFreq= 0;
	u32 tmpNumCyc = 0;
	s32 FullOffset= 0;
	s32 Offset    = 0;
	u32 NumSamples= 0;
	static u32 NumCycles[NUM_CHANNELS] = {0};

	for(u8 ch=0; ch<MaxChannels; ch++)
	{
		// Only do if powered on
		if( (pHpsRegs->Pwr_En) & (1<<ch))
		{
			// Get measured frequency
			Frequency = pHpsRegs->Rd_Frequency[ch];

			// Freq * MAX_OFFSET_SAMPLES
			ScaledFreq = Frequency * MAX_OFFSET_SAMPLES;

			// Number of cycles = result/97656 //(97565 ~= 1/10.24us)
			tmpNumCyc = ScaledFreq/97656;

			// Write number of cycles if different by more than 4 cycles
			if( abs(tmpNumCyc-NumCycles[ch]>4) )
			{
				NumCycles[ch] = tmpNumCyc; 		// Update static variable
				pHpsRegs->WrMaxOffsCycles[ch]; // Write number of cycles to FPGA
			}

			// Now read offset
			FullOffset = pHpsRegs->RdMeasWrapOffs[ch];

			// The result coming back is already divided by 2^9 so must divide by (#samples/2^9)
			NumSamples = NumCycles[ch]*(97656/Frequency);
			NumSamples = NumSamples>>9;
			Offset = (s32)FullOffset/(s32)NumSamples;

			// Write calculated offset if it is within a certain limit
			if (abs(Offset)<0x1000)
				pHpsRegs->WrWrapOffsetCorr[ch] = Offset;
		}

	}
	// Go to next function
	Dpram_Func_Index++;
}

/*
 * 	Monitors the expected reference set by the user and will switch the relay if necessary.
 * 	0-28 is the low voltage input and 28-115 is the high voltage input.
 */
void CheckExpectedRef(void)
{
	u32 tmpExpRef = 0;
	static u32 prevExpRef[NUM_CHANNELS] = {0};
	for(u8 ch=0; ch<MaxChannels; ch++)
	{
		tmpExpRef = pHostRegs->ExpectedReference[ch];					// Read expected reference
		if(tmpExpRef != prevExpRef[ch])									// Compare to previous
		{
			prevExpRef[ch] = tmpExpRef;									// Update previous reading
			if (tmpExpRef <= 2800)										// If reference is less than or equal to 28volts than
				pHpsRegs->ExtRefSel = pHpsRegs->ExtRefSel |  (1<<ch);	// select low voltage reference reading.
			else
				pHpsRegs->ExtRefSel = pHpsRegs->ExtRefSel & ~(1<<ch);	// otherwise select the high voltage reference reading
		}
	}

	// Go to next function
	Dpram_Func_Index++;
}

/*
 *  See if test generator frequency changed and write new one if it did
 */
void CheckTestGenFrequency(void)
{
	static u32 TestGenSetting = 0;
	u32 tmpFreq=0;

	tmpFreq = pHostRegs->TestGenFreq;			// Read setting
	if(tmpFreq != TestGenSetting)
	{
		TestGenSetting = tmpFreq;				// Update variable
		tmpFreq = tmpFreq * TESTGEN_FREQ_SCALE; // Scale Frequency
		pHpsRegs->Wr_TestGenFreq = tmpFreq;		// Write Frequency
	}

	// Go to next function
	Dpram_Func_Index++;

}

/*
 * 	This function will check the measured frequency and change the bandwidth accordingly.
 */
void CheckWrapBandwidth(void)
{
	static u8 BandwidthSel[NUM_CHANNELS] = {0};
	static u8 prevBandwidthSel[NUM_CHANNELS] = {15,15,15}; // Making not 0 or 1 so it's forced to write the bandwidth at least once

	for(u8 ch=0; ch<MaxChannels; ch++)
	{
		// Get frequency
		u32 MeasFreq = pHpsRegs->Rd_Frequency[ch];

		// If below 200Hz then switch to low bandwidth
		if (MeasFreq<200)
			BandwidthSel[ch] = 0;
		else
		{ // else if above 220Hz then switch to high bandwidth
			if (MeasFreq>220)
				BandwidthSel[ch] = 1;
		}

		// See if bandwidth select has changed
		if(prevBandwidthSel[ch] != BandwidthSel[ch])
		{
			prevBandwidthSel[ch] = BandwidthSel[ch];

			if(BandwidthSel[ch]==0)	// Write low bandwidth
			{
				switch (ch)
				{
				case 0:
					Xil_Out32(FPGA_COEF_INTEGRATOR_CH1, 	0x391d4952);//    0.00015
					Xil_Out32(FPGA_COEF_PROPORTIONAL_CH1, 	0x3f400000);//    0.75
					Xil_Out32(FPGA_COEF_FILTER_CH1, 		0xbf7f57c5);//   -0.997433012
					Xil_Out32(FPGA_GAIN_FILTER_CH1, 		0x3aa8723e);//    0.001285143
					Xil_Out32(FPGA_MIN_INTEGRATOR_CH1,      0xc8d6a0e0);// -40960 (3599 DEG/SEC)
					Xil_Out32(FPGA_MAX_INTEGRATOR_CH1,      0x48d6a100);//  40960 (3599 DEG/SEC)
					break;
				case 1:
					Xil_Out32(FPGA_COEF_INTEGRATOR_CH2, 	0x391d4952);//    0.00006
					Xil_Out32(FPGA_COEF_PROPORTIONAL_CH2, 	0x3f400000);//    0.75
					Xil_Out32(FPGA_COEF_FILTER_CH2, 		0xbf7f57c5);//   -0.997433012
					Xil_Out32(FPGA_GAIN_FILTER_CH2, 		0x3aa8723e);//    0.001285143
					Xil_Out32(FPGA_MIN_INTEGRATOR_CH2,      0xc8d6a0e0);// -40960 (3599 DEG/SEC)
					Xil_Out32(FPGA_MAX_INTEGRATOR_CH2,      0x48d6a100);//  40960 (3599 DEG/SEC)
					break;
				case 2:
					Xil_Out32(FPGA_COEF_INTEGRATOR_CH3, 	0x391d4952);//    0.00006
					Xil_Out32(FPGA_COEF_PROPORTIONAL_CH3, 	0x3f400000);//    0.75
					Xil_Out32(FPGA_COEF_FILTER_CH3, 		0xbf7f57c5);//   -0.997433012
					Xil_Out32(FPGA_GAIN_FILTER_CH3, 		0x3aa8723e);//    0.001285143
					Xil_Out32(FPGA_MIN_INTEGRATOR_CH3,      0xc8d6a0e0);// -40960 (3599 DEG/SEC)
					Xil_Out32(FPGA_MAX_INTEGRATOR_CH3,      0x48d6a100);//  40960 (3599 DEG/SEC)
					break;
				}
			}
			else					// Write high bandwidth
			{
				switch (ch)
				{
				case 0:
					Xil_Out32(FPGA_COEF_INTEGRATOR_CH1, 	0x3bb43958);//    0.0055
					Xil_Out32(FPGA_COEF_PROPORTIONAL_CH1, 	0x40800000);//    4.0
					Xil_Out32(FPGA_COEF_FILTER_CH1, 		0xBF7D6437);//   -0.989810434
					Xil_Out32(FPGA_GAIN_FILTER_CH1, 		0x3BA7CCFF);//    0.005120873
					Xil_Out32(FPGA_MIN_INTEGRATOR_CH1,      0xc7abb380);//   -87912 (720 DEG/SEC)
					Xil_Out32(FPGA_MAX_INTEGRATOR_CH1,      0x47abb400);//    87912 (720 DEG/SEC)
					break;
				case 1:
					Xil_Out32(FPGA_COEF_INTEGRATOR_CH2, 	0x3bb43958);//    0.0055
					Xil_Out32(FPGA_COEF_PROPORTIONAL_CH2, 	0x40800000);//    4.0
					Xil_Out32(FPGA_COEF_FILTER_CH2, 		0xBF7D6437);//   -0.989810434
					Xil_Out32(FPGA_GAIN_FILTER_CH2, 		0x3BA7CCFF);//    0.005120873
					Xil_Out32(FPGA_MIN_INTEGRATOR_CH2,      0xc7abb380);//   -87912 (720 DEG/SEC)
					Xil_Out32(FPGA_MAX_INTEGRATOR_CH2,      0x47abb400);//    87912 (720 DEG/SEC)
					break;
				case 2:
					Xil_Out32(FPGA_COEF_INTEGRATOR_CH3, 	0x3bb43958);//    0.0055
					Xil_Out32(FPGA_COEF_PROPORTIONAL_CH3, 	0x40800000);//    4.0
					Xil_Out32(FPGA_COEF_FILTER_CH3, 		0xBF7D6437);//   -0.989810434
					Xil_Out32(FPGA_GAIN_FILTER_CH3, 		0x3BA7CCFF);//    0.005120873
					Xil_Out32(FPGA_MIN_INTEGRATOR_CH3,      0xc7abb380);//   -87912 (720 DEG/SEC)
					Xil_Out32(FPGA_MAX_INTEGRATOR_CH3,      0x47abb400);//    87912 (720 DEG/SEC)
					break;
				}
			}
		}
	}

	// Go to next function
	Dpram_Func_Index++;

}



void Calculate_GainValues(void)
{
	volatile u32 		*pFPGA_SIG_GAIN_MULT, *pFPGA_REF_GAIN_MULT, *pFPGA_SYNTH_GAIN_MULT;

	float Sig_RMS[NUM_CHANNELS] = {0};
	float Ref_RMS[NUM_CHANNELS] = {0};
	float RefGainMult[NUM_CHANNELS] = {0};

	for(u8 ch=0; ch<MaxChannels; ch++)
	{
		u32 tmpSigVll = pHpsRegs->Rd_MeasVll[ch];
		Sig_RMS[ch] = ((float)tmpSigVll)/100;

		// Calculate pointer for channel
		pFPGA_SIG_GAIN_MULT 	= (volatile u32*)FPGA_SIG_GAIN_MULT_CH1;
		pFPGA_REF_GAIN_MULT 	= (volatile u32*)FPGA_REF_GAIN_MULT_CH1;
		pFPGA_SYNTH_GAIN_MULT 	= (volatile u32*)FPGA_SYNTH_GAIN_MULT_CH1;

		pFPGA_SIG_GAIN_MULT 	+= (ch * 0x01);
		pFPGA_REF_GAIN_MULT 	+= (ch * 0x01);
		pFPGA_SYNTH_GAIN_MULT 	+= (ch * 0x01);

		// Calculate Multipliers
		if(Sig_RMS[ch] > 0.100)
			SigGainMult[ch] 		= MAX_SIG_HI_VLL  / Sig_RMS[ch];
		else
			SigGainMult[ch] 		= 1.0000;
		// Stop multiplier from being to big. MAX_SIG_MULT is 10% greater(arbitrary number) than minimum rms voltage allowed
		if(SigGainMult[ch] > MAX_SIG_MULT)
			SigGainMult[ch] = MAX_SIG_MULT;


		// Check expected reference to see what the Max reference voltage should be (either 0-28Vref or 28-115Vref)

		float MaxRefVrms = 0;
		float MaxRefMult = 0;
		if(pHpsRegs->ExtRefSel & (1<<ch))	// 0:HV, 1:LV
		{
			MaxRefVrms = 28;
			MaxRefMult = 30.8;
		}
		else
		{
			MaxRefVrms = 115;
			MaxRefMult = 126.5;
		}

		// Read Ref RMS
		u32 tmpRefRms = pHpsRegs->Rd_RefVoltage[ch];
		Ref_RMS[ch] = ((float)tmpRefRms)/100;

		if(Ref_RMS[ch] > 0.100)
			RefGainMult[ch] 		= MaxRefVrms / Ref_RMS[ch]; //RA need to switch based on expected reference MAX_REF_HI_VRMS / Ref_RMS[ch];
		else
			RefGainMult[ch] 		= 1.0000;
		// Stop multiplier from being to big. MAX_REF_MULT is 10% greater(arbitrary number) than minimum rms voltage allowed
		if(RefGainMult[ch] > MaxRefMult) 	//ra needs to be variable based on expected ref. MAX_REF_MULT)
			RefGainMult[ch] = MaxRefMult;	//ra needs to be variable based on expected ref. MAX_REF_MULT;

		SynthRefGainMult[ch] = SigGainMult[ch]  * 25.0;

		// Output Multipliers to FPGA
		*(volatile float*)pFPGA_SIG_GAIN_MULT 		= SigGainMult[ch];
		*(volatile float*)pFPGA_REF_GAIN_MULT 		= RefGainMult[ch];
		*(volatile float*)pFPGA_SYNTH_GAIN_MULT 	= SynthRefGainMult[ch];

	}

	// Go to next function
	Dpram_Func_Index++;
}
//===============================================================




/*
 *  Used to point the function pointer back to beginning
 */
void EndOfDpramFunctions(void)
{
    Dpram_Func_Index=0;
}


// ###########################################################################
// 	BIT
// ###########################################################################
/*
 * D2 Test - This will compare the set angle against the measured angle
 */
void D2Test(void)
{
	bool NewBitAng_flg = FALSE;
	bool ChanRotating_flg = FALSE;

	static u32 AccumulatedError = 0;
	static u32 SampleCount = 0;
	static u8 BITChan = 0;
	static u32 CurrentBitAngle[NUM_CHANNELS] = {0};
	static u32 BitDelayCounter[NUM_CHANNELS] = {0};

	// Skip over this D2 test if D3 Test is enabled or D2 not enabled
	u32 TestEnable = pCommon->test_enable;
	if ((TestEnable & 0x8) > 0 || (TestEnable & 0x4) == 0)
	{
		// Reset average & sample count
		AccumulatedError = 0;
		SampleCount = 0;

		if ((TestEnable & 0x8) == 0 && (TestEnable & 0x4) == 0) // If D3 and D2 disabled
		{
			BitStatusWord = 0;							// Clear bit status
			pHpsRegs->Wr_BitStatus = BitStatusWord;		// Write to bit status register
		}

		Bit_Func_Index++;								// Go to next function
		return;

	}

	// Write 0x55 to heart beat register if D2 test is enabled
	pCommon->test_verify = 0x55;

	// See if D/S Angle has changed (also check ratio for 2-chan module)
	u32 CompareAngle = 0;
/*	u32 tmpRatio  = pHostRegs->DSRatioMode;
	if( (tmpRatio != 1) && (BITChan==1) ) 					// This is checking if the second channel is in ratio mode
	{
		u32 tmpCh1Ang = pHostRegs->ds_Angle[0];				// Get channel 1 angle
		CompareAngle = tmpCh1Ang*tmpRatio;					// Scale it by ratio
	}
	else
		CompareAngle = pHostRegs->ds_Angle[BITChan]; 		// Otherwise read set angle
*/
	CompareAngle = pHpsRegs->Rd_ExpectedAngle[BITChan]; 	// Read set angle

	if(CurrentBitAngle[BITChan] != CompareAngle)			// If angle has changed
	{
		CurrentBitAngle[BITChan] = CompareAngle;			// then update the registers
		NewBitAng_flg = TRUE;								// and raise the flag
	}

	// Check if rotating
	if(pHpsRegs->Rd_RotationStatus & (0x1 << BITChan))
		ChanRotating_flg = TRUE;

	// Check if powered off or on
	u8 PoweredON_flg = 0;
	PoweredON_flg = (pHpsRegs->Pwr_En>>BITChan) & 0x1;

	// Create flag for just powering up so there's a delay in checking BIT
	bool CreatePowerOnDelay = FALSE;
	static bool prevPoweredON_flg[NUM_CHANNELS] = {FALSE};
	if(prevPoweredON_flg[BITChan] != PoweredON_flg)					// Check against previous flag
	{
		if(PoweredON_flg==1 && prevPoweredON_flg[BITChan]==0)
			CreatePowerOnDelay=TRUE;								// Create power up delay
		prevPoweredON_flg[BITChan] = PoweredON_flg;					// Update previous flag
	}

	// Check flags to see if we need to reset the BIT check
//	if(PoweredON_flg==0 || CreatePowerOnDelay==TRUE || NewVoltage_flg[BITChan]==TRUE || NewRefVltg_flg[BITChan]==TRUE  || NewFrequency_flg[BITChan]==TRUE || NewBitAng_flg==TRUE || ChanRotating_flg==TRUE)
	if(CreatePowerOnDelay==TRUE || NewVoltage_flg[BITChan]==TRUE || NewRefVltg_flg[BITChan]==TRUE  || NewFrequency_flg[BITChan]==TRUE || NewBitAng_flg==TRUE || ChanRotating_flg==TRUE)
	{
		// Reset average & sample count
		AccumulatedError = 0;
		SampleCount = 0;

		// Clear flags that were set in other functions
		CreatePowerOnDelay = FALSE;
		NewVoltage_flg[BITChan]= FALSE;
		NewFrequency_flg[BITChan]=FALSE;
		NewRefVltg_flg[BITChan]=FALSE;

		if(ChanRotating_flg==TRUE)
		{
			BitStatusWord &= ~(1<<BITChan);				// Clear bit status
			pHpsRegs->Wr_BitStatus = BitStatusWord;		// Write to bit status register
		}

/*		if (PoweredON_flg==0)
		{
			BitStatusWord = BitStatusWord & pHpsRegs->Pwr_En; // Clear bit for powered off channel
			pHpsRegs->Wr_BitStatus = BitStatusWord;		// Write to bit status register
		}
*/
		//usleep(500000); // Wait 500ms for output to settle
		BitDelayCounter[BITChan] = 0;

		// Increment channel and function if necessary
		if(BITChan<MaxChannels-1)
			BITChan++;										// Increment bit channel
		else
			BITChan=0;										// Reset Bit channel

		Bit_Func_Index++;									// Go to next function
		return;

	}

	// Check delay counter
	if (BitDelayCounter[BITChan] < BIT_DELAY_TIME)
	{
		BitDelayCounter[BITChan]++;
		return;
	}

	// Get difference between set angle and measured then accumulate it
	u32 AngDiff = 0;
	AngDiff = abs((s32)pHpsRegs->Rd_WrapAngle[BITChan]-(s32)CurrentBitAngle[BITChan]);
	AngDiff = AngDiff >> 16; 								// Use only upper 16 bits for error accumulation
	AccumulatedError += AngDiff;							// Accumulate

	// If reached max number of accumulations than get average otherwise increment counter
	if(SampleCount < (NUM_D2_SAMPLES))
		SampleCount++;
	else
	{
		// Reset sample count
		SampleCount = 0;

		// Check if average is above threshold
		u32 AveragedError = 0;
		AveragedError = AccumulatedError >> NUM_D2_BITS;
		AccumulatedError = 0;

		// Set BIT if it is
		if (AveragedError > MAX_BIT_ERROR)					// Set BIT status if failed
			BitStatusWord |= (1<<BITChan);
		else												// Clear BIT status if passed
			BitStatusWord &= ~(1<<BITChan);

//		BitStatusWord = BitStatusWord & pHpsRegs->Pwr_En;	// Mask Bit Status with power on register
		pHpsRegs->Wr_BitStatus = BitStatusWord;				// Write bit status

		// Increment channel and function if necessary
		if(BITChan<MaxChannels-1)
			BITChan++;
		else
		{
			BITChan=0;										// Reset Bit channel
			Bit_Func_Index++;								// Go to next function
		}
	}


}


/*
 * D3 Test - This will cycle through 24 angles and check the error in the measured result
 */
void D3Test(void)
{
	bool FinishedD3 = FALSE;
	static u32 D3ErrorAcc[NUM_CHANNELS] = {0};
	static u32 D3AccCnt = 0;
	static u32 D3TestIndex = 0;
	u32 D3TestArray[24] = {0x00B60B60, 0x0AAAAAAA, 0x15555555, 0x1FFFFFFF, 0x2AAAAAAA, 0x35555555, 0x3FFFFFFF, 0x4AAAAAAA, 0x55555555, 0x5FFFFFFF, 0x6AAAAAAA, 0x75555555,
						   0x7FFFFFFF, 0x8AAAAAAA, 0x95555555, 0x9FFFFFFF, 0xAAAAAAAA, 0xB5555555, 0xBFFFFFFF, 0xCAAAAAAA, 0xD5555555, 0xDFFFFFFF, 0xEAAAAAAA, 0xF5555555};

	u32 TestEnable = pCommon->test_enable;
	if((TestEnable&0x8)==0)	// If disabled
	{
		pHpsRegs->D3TestEn 	 = 0;
		pHpsRegs->D3TestWord = 0;
		FinishedD3 = TRUE;
		D3Enabled = FALSE;

		Bit_Func_Index++;	// Go to next function
		return;
	}

	// Initialize Loop
	if(D3Enabled == FALSE)
	{
		// Reset BitWord for D3 test
		BitStatusWord = 0;
		pHpsRegs->Wr_BitStatus = BitStatusWord;

		D3TestIndex = 0;													// Initialize index
		pHpsRegs->D3TestWord = D3TestArray[D3TestIndex];					// Write first word

		for(u32 ch=0; ch<MaxChannels; ch++)
			D3ErrorAcc[ch] = 0;											// Reset Accumulator
		D3AccCnt = 0;													// Reset accumulator counter

		pHpsRegs->D3TestEn = 1;											// Write select line down to FPGA
		D3Enabled = TRUE;												// Set flag to indicate
		usleep(700000); 												// (now 500 ms) Waiting 50 ms in case the module has a slow bandwidth set
		FinishedD3 = FALSE;

	}

	for (u8 ch=0; ch<MaxChannels; ch++)
	{
		// Read wraps and compare to expected angle
		u32 WrapReading = pHpsRegs->Rd_WrapAngle[ch];
		u32 multispeedratio = pHostRegs->DSRatioMode;
		u32 CompareAng = 0;

		// Check for multispeed mode and multiply angle if necessary
		if(ch==1 && multispeedratio!=1)
			CompareAng = D3TestArray[D3TestIndex] * multispeedratio;
		else
			CompareAng = D3TestArray[D3TestIndex];

		u32 tmpError = abs(WrapReading - CompareAng);				// Get error
		tmpError = tmpError >> 16;									// Only using upper 16 bits of angle error
		D3ErrorAcc[ch] += tmpError;									// Accumulate error
	}

	// Check Accumulator counter
	if( (D3AccCnt) < (NUM_D3_SAMPLES) )
		D3AccCnt++;	// Increment Accumulator counter after all channels were checked
	else
	{
		D3AccCnt = 0;												// Reset accumulator counter
		s32 D3DiffAvg = 0;											// Will be average error

		for(u8 ch=0; ch<MaxChannels; ch++)							// Loop through channels and get average error
		{
			D3DiffAvg = D3ErrorAcc[ch] >> NUM_D3_BITS;
			D3ErrorAcc[ch] = 0;

			// Check average to see if it's too large and set bit if it is
			if(D3DiffAvg>MAX_BIT_ERROR)								// Set BIT status if failed
				BitStatusWord |= (1<<ch);
		}

		// Was this the last index?
		if (D3TestIndex == 23)
			FinishedD3 = TRUE;										// Finished D3
		else														// Not done yet
		{
			D3TestIndex++;											// Go to next test word
			pHpsRegs->D3TestWord = D3TestArray[D3TestIndex]; 		// Write word to module
			usleep(700000); 										// Set delay
		}

		if (FinishedD3 == TRUE)
		{
			// Mask Bit Status with power on register
//			BitStatusWord = BitStatusWord & pHpsRegs->Pwr_En;

			// Write to bit status (write 1 to bit position if error)
			pHpsRegs->Wr_BitStatus =  BitStatusWord;				// Write to bit status register

			// Clean up
			pHpsRegs->D3TestEn 	 = 0;								// Clear Select line
			pHpsRegs->D3TestWord = 0;								// 0 out test word
			D3Enabled = FALSE;; 									// Clear flag
			pCommon->test_enable = (TestEnable & ~0x8); 			// Clear D3 test enable bit

		}
	}

	Bit_Func_Index++;// Go to next function

}

void EndOfBitFunctions(void)
{
	  Bit_Func_Index=0;
}


// ###########################################################################
// 	BACKGROUND ANGLE CALIBRATION
// ###########################################################################

/*
 *  This will see if we want to cal the background angle correction routines
 */
void CheckBackgroundCal(void)
{
	// Check if background cal disabled
	if( (pHostRegs->En_BkgrndCal & 0x0000FFFF) != 0x0000AA55)
		return;

	// Check if channel powered off
	if( !(pHpsRegs->Pwr_En & (1<<BkgndChan)) )
	{
		// Increment channel
		if (BkgndChan < MaxChannels - 1)
			BkgndChan++;
		else
			BkgndChan = 0;

		// Reset function pointer to 0
		Bkgnd_Func_Index = 0;

		return;
	}

	// If not then call background angle calibration routine
	BkgrndAngleCorrPtr[Bkgnd_Func_Index]();

}


/*
 *  This will initialize the background angle correction
 */
static void InitBkgrndAngleCal(void)
{
	BkgndSampleCount = 0; 										// Reset number of samples
	BkgndAccum = 0; 											// Reset accumulator
	Bkgnd_Func_Index++;											// Go to next function
}

/*
 *  This will average the error between the set angle and the actual output
 */
static void AverageAngleError(void)
{
	bool AngleTooClose = FALSE;
	static u32 prevCorrection[NUM_CHANNELS] = {0};
	static u32 prevAngle[NUM_CHANNELS] = {0};
	static u32 DelayCounter[NUM_CHANNELS] = {0};
	static XTime time;
	static bool  time_to_update = TRUE;

	s32 tmpWrap 	= pHpsRegs->Rd_WrapAngle[BkgndChan]; 				// Read Wrap angle
	s32 tmpExpAng   = pHpsRegs->Rd_ExpectedAngle[BkgndChan]; 			// Read expected angle
	float tmpRatio 	= (float)((u32)tmpExpAng)/(float)((u32)tmpWrap);	// Get ratio

	// See if angle changed by a large amount
	if(abs((u32)tmpExpAng-prevAngle[BkgndChan]) > 0x038E38E3) 			// If angle changed by more than 5 degrees
	{
		BkgndIterationCount = 0;										// Reset iteration count
		BkgndSampleCount = 0;											// Reset sample count
		Bkgnd_Func_Index++;												// Go to next function
		prevAngle[BkgndChan] = (u32)tmpExpAng;							// Update previous angle
		DelayCounter[BkgndChan] = 100000;								// Create a delay
		return;															// Abort function
	}

	prevAngle[BkgndChan] = (u32)tmpExpAng;								// Update previous angle

	// Check delay counter
	if(DelayCounter[BkgndChan]>0)										// If not zero then keep delaying
	{
		DelayCounter[BkgndChan]--;										// Decrement counter
		return;															// Abort function
	}

	if (BkgndIterationCount<MAX_BKGND_ANG_SAMPLES)			 			// If number of samples is less than max
	{
		u32 tmpDiff = abs(0-(u32)tmpExpAng);
		if(pHostRegs->SynRsl_sel == 0)	// RESOLVER
		{
			if( (tmpDiff<0x038E38E3) ||									// Between 355 and 5
			((u32)tmpExpAng>0x3C71C71C && (u32)tmpExpAng<0x438E38E3) ||	// Between  85 and 95
			((u32)tmpExpAng>0x7C71C71C && (u32)tmpExpAng<0x8AAAAAAA) ||	// Between 175 and 195
			((u32)tmpExpAng>0xBC71C71C && (u32)tmpExpAng<0xC38E38E3) )	// Between 265 and 275
			AngleTooClose = TRUE;
		}
		else
		{								// SYNCHRO
			if( (tmpDiff<0x038E38E3) ||									// Between 355 and 5
			((u32)tmpExpAng>0x51C71C71 && (u32)tmpExpAng<0x58E38E38) ||	// Between 115 and 125
			((u32)tmpExpAng>0x7C71C71C && (u32)tmpExpAng<0x8AAAAAAA) ||	// Between 175 and 195
			((u32)tmpExpAng>0xD1C71C71 && (u32)tmpExpAng<0xD8E38E38) )	// Between 295 and 305
			AngleTooClose = TRUE;
		}

		if(AngleTooClose == FALSE)										// Check if angle is at least 5 degrees away from a zero crossing
		{
			BkgndAccum = BkgndAccum + tmpRatio; 						// Accumulate Ratio
			BkgndSampleCount++;											// Increment sample count
		}
		BkgndIterationCount++;											// Increase iteration count
	}
	else
	{																	// else reached max sample count
		if(BkgndSampleCount>256)										// Need at least this many samples
		{
			if(time_to_update==TRUE)
			{
				time_to_update = FALSE;										// Reset boolean
				time = get_timer(0);										// Reset timer

				BkgndAccum = (BkgndAccum/BkgndSampleCount); 				// This holds the averaged ratio of the error
				if ((pHostRegs->En_BkgrndCal & 0xFFFF0000) == 0xAA550000)	// See if writing correction is enabled
				{

					u32 tmpCorr = 0;
					tmpCorr = pCalRegs->ds_angle[BkgndChan];				// Get current correction
	/*				tmpCorr = (u32)((float)tmpCorr * BkgndAccum);			// Scale current correction by averaged ratio

					if(tmpCorr>0x3A000000 && tmpCorr<0x45000000)			// Make sure correction is within limits
					{
						if(abs(prevCorrection[BkgndChan]-tmpCorr)>MIN_BKGND_ANG_CORR)	// Make sure it's a big enough correction
							pCalRegs->ds_angle[BkgndChan] = tmpCorr;				// Write new angle calibration value
					}
	*/

					// Want to divide ratio down by 8 so need to use the following formula:
					// SmallerRatio = [1-(1-Ratio)/8]
					float SmallerRatio = (1-BkgndAccum)/8;
					SmallerRatio = 1 - SmallerRatio;
					tmpCorr = (u32)((float)tmpCorr * SmallerRatio);			// Scale current correction by averaged ratio

					if(tmpCorr>0x3A000000 && tmpCorr<0x45000000)			// Make sure correction is within limits
					{
						if(abs(prevCorrection[BkgndChan]-tmpCorr)>MIN_BKGND_ANG_CORR)	// Make sure it's a big enough correction
							pCalRegs->ds_angle[BkgndChan] = tmpCorr;					// Write new angle calibration value
					}
				}
			}

			/* Time to make background correction? (every 1 s) */
			if (get_timer(time) > 1000)
				time_to_update = TRUE;
		}
		BkgndIterationCount = 0;										// Reset iteration count
		BkgndSampleCount = 0;											// Reset sample count

		// Go to next function
		Bkgnd_Func_Index++;
	}

}

/*
 *  Cleanup Background angle correction
 */
static void EndOfBkgrndAngleFunctions(void)
{
	// Increment channel
	if (BkgndChan < MaxChannels - 1)
		BkgndChan++;
	else
		BkgndChan = 0;

	// Reset function pointer to 0
	Bkgnd_Func_Index = 0;

}

/****************************** Local functions *****************************/

static void init(void)
{
    /* Load configuration parameters */
    load_params();

    /* Load calibration data */
    load_caldata();

     /*
     * Insert module specific initialization code here...
     */

    // Init PWM gain to 1 (normalized to 0x4000) THIS IS NOW USED FOR THE PWM DEAD TIME NOT THE PWM GAINS
    pHpsRegs->PwmScaleCh1 = 0x0000;
    pHpsRegs->PwmScaleCh2 = 0x0000;
    pHpsRegs->PwmScaleCh3 = 0x4000;
    pHostRegs->PwmScaleCh1 = 0x0000;
    pHostRegs->PwmScaleCh2 = 0x0000;
    pHostRegs->PwmScaleCh3 = 0x0000;

    // Init Arm Del Amount for phase shifting
    pHostRegs->wr_ArmDelAmt = 0x1;//0x400;
    pHpsRegs->wr_ArmDelAmt = 0x1;//0x400;

    // ========= ADDING JERRY'S S/D INIT CODE WITH MY CUSTOMIZED COEFFICIENTS ===========
    // Coefficients (These values equate to 40Hz bandwidth)(Filter at 80Hz cutoff)
    	Xil_Out32(FPGA_COEF_INTEGRATOR_CH1, 	0x3c23d70a);//    9.99999977648258209228515625E-3
    	Xil_Out32(FPGA_COEF_PROPORTIONAL_CH1, 	0x40f00000);//    7.5
    	Xil_Out32(FPGA_COEF_FILTER_CH1, 		0xBF7D6437);//   -0.989810434
    	Xil_Out32(FPGA_GAIN_FILTER_CH1, 		0x3BA7CCFF);//    0.005120873

    	Xil_Out32(FPGA_COEF_INTEGRATOR_CH2, 	0x3c23d70a);//    9.99999977648258209228515625E-3
    	Xil_Out32(FPGA_COEF_PROPORTIONAL_CH2, 	0x40f00000);//    7.5
    	Xil_Out32(FPGA_COEF_FILTER_CH2, 		0xBF7D6437);//   -0.989810434
    	Xil_Out32(FPGA_GAIN_FILTER_CH2, 		0x3BA7CCFF);//    0.005120873

    	Xil_Out32(FPGA_COEF_INTEGRATOR_CH3, 	0x3c23d70a);//    9.99999977648258209228515625E-3
    	Xil_Out32(FPGA_COEF_PROPORTIONAL_CH3, 	0x40f00000);//    7.5
    	Xil_Out32(FPGA_COEF_FILTER_CH3, 		0xBF7D6437);//   -0.989810434
    	Xil_Out32(FPGA_GAIN_FILTER_CH3, 		0x3BA7CCFF);//    0.005120873

    	Xil_Out32(FPGA_COEF_INTEGRATOR_CH4, 	0x3c23d70a);//    9.99999977648258209228515625E-3
    	Xil_Out32(FPGA_COEF_PROPORTIONAL_CH4, 	0x40f00000);//    7.5
    	Xil_Out32(FPGA_COEF_FILTER_CH4, 		0xBF7D6437);//   -0.989810434
    	Xil_Out32(FPGA_GAIN_FILTER_CH4, 		0x3BA7CCFF);//    0.005120873

    	Xil_Out32(FPGA_BW_COEF_INTEGRATOR_BIT, 	0x3c23d70a);//    9.99999977648258209228515625E-3
    	Xil_Out32(FPGA_BW_COEF_PROPORTIONAL_BIT,0x40f00000);//    7.5
    	Xil_Out32(FPGA_BW_COEF_FILTER_BIT, 		0xBF7D6437);//   -0.989810434
    	Xil_Out32(FPGA_BW_GAIN_FILTER_BIT, 		0x3BA7CCFF);//    0.005120873

/*    	Xil_Out32(DPRAM_LVDT_SCALE_CH1,			0x10000000);	//	Initial gain of "1"
    	Xil_Out32(DPRAM_LVDT_SCALE_CH2,			0x10000000);	//	Initial gain of "1"
    	Xil_Out32(DPRAM_LVDT_SCALE_CH3,			0x10000000);	//	Initial gain of "1"
    	Xil_Out32(DPRAM_LVDT_SCALE_CH4,			0x10000000);	//	Initial gain of "1"
*/
    	// Sig & Ref Gains (MOD_4CH_SD_#1)
    	Xil_Out32(FPGA_SIG_GAIN_MULT_CH1, 0x3F89D89E);			//  Gain of 1.07 (Assuming 26 VLL)
    	Xil_Out32(FPGA_SIG_GAIN_MULT_CH2, 0x3F89D89E);			//  Gain of 1.07 (Assuming 26 VLL)
    	Xil_Out32(FPGA_SIG_GAIN_MULT_CH3, 0x3F89D89E);			//  Gain of 1.07 (Assuming 26 VLL)
    	Xil_Out32(FPGA_SIG_GAIN_MULT_CH4, 0x3F89D89E);			//  Gain of 1.07 (Assuming 26 VLL)
    	SigGainMult[0] = 0x3F89D89E;
    	SigGainMult[1] = 0x3F89D89E;
    	SigGainMult[2] = 0x3F89D89E;
    	SigGainMult[3] = 0x3F89D89E;

    	Xil_Out32(FPGA_REF_GAIN_MULT_CH1, 0x3F89D89E);			//	Gain of 1.07 (Assuming 26 V ref in)
    	Xil_Out32(FPGA_REF_GAIN_MULT_CH2, 0x3F89D89E);			//	Gain of 1.07 (Assuming 26 V ref in)
    	Xil_Out32(FPGA_REF_GAIN_MULT_CH3, 0x3F89D89E);			//	Gain of 1.07 (Assuming 26 V ref in)
    	Xil_Out32(FPGA_REF_GAIN_MULT_CH4, 0x3F89D89E);			//	Gain of 1.07 (Assuming 26 V ref in)
    	RefGainMult[0] = 0x3F89D89E;
    	RefGainMult[1] = 0x3F89D89E;
    	RefGainMult[2] = 0x3F89D89E;
    	RefGainMult[3] = 0x3F89D89E;

    	Xil_Out32(FPGA_SYNTH_GAIN_MULT_CH1, 0x41D76276);		//	Gain of (1.07 * 25) (Assuming 26 VLL)
    	Xil_Out32(FPGA_SYNTH_GAIN_MULT_CH2, 0x41D76276);		//	Gain of (1.07 * 25) (Assuming 26 VLL)
    	Xil_Out32(FPGA_SYNTH_GAIN_MULT_CH3, 0x41D76276);		//	Gain of (1.07 * 25) (Assuming 26 VLL)
    	Xil_Out32(FPGA_SYNTH_GAIN_MULT_CH4, 0x41D76276);		//	Gain of (1.07 * 25) (Assuming 26 VLL)
    	SynthRefGainMult[0] = 0x41D76276;
    	SynthRefGainMult[1] = 0x41D76276;
    	SynthRefGainMult[2] = 0x41D76276;
    	SynthRefGainMult[3] = 0x41D76276;

//    	Xil_Out32(FPGA_DAC_CONFIG_MODE, 0x00000000);			//	Set DAC to receive data from SINE/COSINE
//    	Xil_Out32(FPGA_DAC_CONFIG_WORD, 0x00000000);			//	default
//    	Xil_Out32(FPGA_BIT_FREQ_CH1, BIT_DAC_FREQ);				//	default to 7.283Khz(HV) -or- 15.935Khz(LV)
//    	Xil_Out32(FPGA_BIT_FREQ_CH2, BIT_DAC_FREQ);				//	default to 7.283Khz(HV) -or- 15.935Khz(LV)
//    	Xil_Out32(FPGA_BIT_FREQ_CH3, BIT_DAC_FREQ);				//	default to 7.283Khz(HV) -or- 15.935Khz(LV)
//    	Xil_Out32(FPGA_BIT_FREQ_CH4, BIT_DAC_FREQ);				//	default to 7.283Khz(HV) -or- 15.935Khz(LV)

//      	Xil_Out32(FPGA_BIT_DAC_GAIN_MULTIPLIER, BIT_DAC_SCALE);	//	Gain of FullScale(HV) or 1/8 Scale(LV)
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
    // =========================== END JERRY'S S/D INIT CODE ===========================

    /* Module is now initialized */
    mod_init = TRUE;
}


static void load_params(void)
{
    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */

    }

    // Send Mux switches down based on module type
    u32 tmpSynRslSel	= 0;
    u32 tmpExpRef 		= 0;
    u32 tmpThreshRef 	= 0;
    u32 tmpThreshVLL 	= 0;
//    u32 tmpCurLim 		= 0;
    u16 tmpModuleType	= 0;
    u32 tmpVllScale		= 0;
    u32 tmpDSVLL		= 0;
    u32 tmpHVLV			= 0;

	#if MODULE_DS8
    	MaxChannels		= 1;		// Holds the number of maximum channels available for this module
    	tmpSynRslSel 	= 3;    	// 0=RSL, 1=4-wire, 2=2-wire, 3=SYN
    	tmpExpRef		= 0x2CEC;	// 115 volts
    	tmpThreshRef 	= 0x1F72; 	// 11500*80%
    	tmpThreshVLL 	= 0x1C20; 	//  9000*80%
//    	tmpCurLim		= 200; 		// The A/D is 134.3uA/bit so 200mA ~ 1489
    	tmpModuleType   = 0;		// DS8 module
    	tmpVllScale		= 0x40000000;// High voltage scale (original 0x0001D208 - Not sure how this was derived)
    								//  Low voltage scale (original 0x0005D9F7 - Not sure how this was derived)
    	tmpDSVLL 		= 9000;		// 90 volts for HV module
    	tmpHVLV			= 0;		// High voltage output

    #elif MODULE_DSE
    	MaxChannels		= 2;		// Holds the number of maximum channels available for this module
    	tmpSynRslSel	= 3;    	// 0=RSL, 1=4-wire, 2=2-wire, 3=SYN
    	tmpExpRef		= 0x2CEC;	// 115 volts
    	tmpThreshRef 	= 0x1F72; 	// 11500*80%
    	tmpThreshVLL 	= 0x1C20; 	//  9000*80%
//        tmpCurLim		= 75; 		// 75 mA limit. The A/D is 134.3uA/bit.
        tmpModuleType   = 1;		// DSE module
    	tmpVllScale		= 0x40000000;// High voltage scale (original 0x0001D208 - Not sure how this was derived)
    								//  Low voltage scale (original 0x0005D9F7 - Not sure how this was derived
    	tmpDSVLL		= 9000;		// 90 volts for HV module
    	tmpHVLV			= 0;		// High voltage output

	#elif MODULE_DSJ
    	MaxChannels		= 3;		// Holds the number of maximum channels available for this module
    	tmpSynRslSel	= 3;    	// 0=RSL, 1=4-wire, 2=2-wire, 3=SYN
    	tmpExpRef		= 0x0A28;	// 26 volts
    	tmpThreshRef 	= 0x0820; 	// 26*80%
    	tmpThreshVLL 	= 0x0820; 	// 26*80%
//        tmpCurLim		= 75; 		// 75 mA limit. The A/D is 134.3uA/bit.
        tmpModuleType   = 3;		// DSJ module
    	tmpVllScale		= 0x40000000;// High voltage scale (original 0x0001D208 - Not sure how this was derived)
    								//  Low voltage scale (original 0x0005D9F7 - Not sure how this was derived
    	tmpDSVLL		= 2600;		// 26 volts for LV module
    	tmpHVLV			= 1;		// Low voltage output


	#endif

    Load_DigitalController(tmpModuleType);
    pHpsRegs->ModuleType = tmpModuleType;

	for(u8 ch=0; ch<MaxChannels; ch++)
	{
		pHostRegs->SynRsl_sel[ch] 		= tmpSynRslSel;
		pHostRegs->ExpectedReference[ch]= tmpExpRef;
		pHostRegs->ref_Threshold[ch]	= tmpThreshRef;
		pHostRegs->sig_Threshold[ch]	= tmpThreshVLL;
// Comes from Cal file now		pHostRegs->cur_Threshold[ch] 	= tmpCurLim;
		pHpsRegs->Wr_VLLScale[ch]		= tmpVllScale;
		pHostRegs->ds_Voltage[ch]		= tmpDSVLL;

		pCommon->ch_status_en |= (1 << ch); // Enable status registers




		// @@@ fixme - temporarily initilializing to gain of "1"
		pHpsRegs->Wr_RefScale[ch] = 0x40000000;
	}

	pHostRegs->DSRatioMode = 1;		// Default ratio of 1 for multispeed
	pCommon->test_enable = 0x4;		// Enable D2 test
	pHpsRegs->HVLV_sel = tmpHVLV;	// Set type of module
	pHostRegs->TestGenFreq = 400;	// Initialize test generator to be 400 Hz
}

/*
 * Load digital controller coefficients
 */
static void Load_DigitalController(u16 ModuleType)
{
	pHpsRegs->Wr_DigCtrlEnable = 1;

	for(u32 tmpAddr=0; tmpAddr<40; tmpAddr++)
	{
		pHpsRegs->Wr_DigCtrlAddr = tmpAddr;
		if(ModuleType==0)
			pHpsRegs->Wr_DigCtrlData = DigitalControllerCoeff_DS8[tmpAddr];
		else
			pHpsRegs->Wr_DigCtrlData = DigitalControllerCoeff_DSE[tmpAddr];

	}

	pHpsRegs->Wr_DigCtrlEnable = 0;

}


static void load_caldata(void)
{
    if (!(pCommon->mod_ready & MOD_READY_CAL_LOADED))	// If cal file isn't loaded
    {
        /* Use default values */
		for (u32 ch=0; ch<NUM_CHANNELS; ch++)
		{
			pCalRegs->ds_voltage[ch] 			= 0x40000000; // Write 4 Voltage Cals (0x00000000)
			pCalRegs->ds_angle[ch] 				= 0x40000000; // Write 4 Angle Cals (0x00000010)
			pCalRegs->ds_offset[ch][0] 			= 0x00000000; // Write 4 Offset Cals (0x00000020)
			pCalRegs->ds_offset[ch][1] 			= 0x00000000; // Write 4 Offset Cals (0x00000030)
			pCalRegs->ds_phseCorr[ch] 			= 0x00000000; // Write 4 Angle Cals (0x00000040)
			pCalRegs->ds_WrapCorr[ch] 			= 0x40000000; // Write 4 Wrap Cals (0x00000050)
			pCalRegs->ds_VllMeasCorr[ch] 		= 0x4000; 	  // Write 4 Vll Cals (0x00000060)
			pCalRegs->ds_RefMeasCorr[ch] 		= 0x4000; 	  // Write 4 Ref Cals (0x00000070)
			pCalRegs->ds_PwmGain[ch]			= 0x30003000; // Write Pwm Gain Cals(0x000000A0)
			pCalRegs->ds_PwmPhase[ch]			= 0x00FE00FE; // Write Pwm Phase respect to output (0x000000B0)
			pCalRegs->ds_voltage_FixedMode[ch] 	= 0x40000000; // Write 4 Voltage Cals (0x00000110)

			pHostRegs->cur_Threshold[ch]		= 200;		  // Write Current threshold for each channel

		}

		pCalRegs->ds_MeasCurOffset[0]	= 0xFE0;
		//for (u32 ch=0; ch<6; ch++)
		for (u32 ind=0; ind<6; ind++)
		{
			pCalRegs->ds_MeasCurOffset[ind] = 0x0; 			// Write 6 Offset Corrections 			(0x00000080)
			pCalRegs->ds_PwmOffset[ind] 	= 0x00000000; 	// Write 6 PWM offsets 		  			(0x000000C0)
			pCalRegs->ds_MeasCurGain[ind]	= 0x400;		// Write 6 Current Gain Corrections 	(0x000000E0)
		}

    }
    else	// If cal file is loaded, then get current limit and write to host register
    {
		for (u32 ch = 0; ch < NUM_CHANNELS; ch++)
			pHostRegs->cur_Threshold[ch] = pCalRegs->CurrLimSetting[ch]; // Write DS current limit from cal register (0x00000100)
	}


}



