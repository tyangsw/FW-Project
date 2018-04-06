/*
 * DA1 module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Local functions */
static void init(void);
static void load_params(void);
static void load_caldata(void);

/* Global variables */
static bool mod_init = FALSE;

// Module Specific Functions
static void ReadExtPllParams(void);
static void NullPLLVadc (void);
static void CheckPowerEnable(void);
static void CheckRangeAndMode(void);
static void CheckUserSampleRate(void);
static bool NewPLLParameters(u32 chanRate);
static void CheckExternalPLL(void);
static void CheckCalMode(void);
static void EndOfDpramFunctions(void);

// Bit Functions
static void D2Test(void);
static void D3Test(void);
static void EndOfBitFunctions(void);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DA Global Variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HOST_REGS 	*pHostRegs = (HOST_REGS *)HOST_REGS_BASE;
HPS_REGS  	*pHpsRegs  = (HPS_REGS *)HPS_REGS_BASE;
CAL_REGS	*pCalRegs  = (CAL_REGS *)MODULE_CAL_BASE;


static u32		Dpram_Func_Index	= 0;     // Holds index for function pointer
static u32		UserSampleRate		= 0;
static double	ExtPll_RFREQflt 	= 0;
static double	ExtPll_fxtal		= 0;
static u8 		NullPllDone 		= 0;
static bool 	LoadZeroCal 		= 0;

// For Bit
static bool 	D3Enabled = FALSE;
static u32		Bit_Func_Index		= 0;
static u32		BITChan				= 0;
static u32 		BitWord   			= 0;
bool 			NewMode_flg 		= FALSE;
bool 			NewRange_flg 		= FALSE;
bool			NewSampRate_flg 	= FALSE;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static u16 		ExtPll_HSDV			= 0;
static u16 		ExtPll_N1 			= 0;
static union {
		s32 half[2];
		s64 full;
	  } ExtPll_RFREQ;

static u32 		ExtPll_Fout	 		= 10000000; //10MHz Startup Frequency of Si599 Oscillator
static u32 		PllMeasTime 		= 0;
//static u32 		PLLVadcLockVal 		= 0;
static s32 		PLLDacWord 			= 0x8000;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


/**************************** Exported functions ****************************/

void module_init(void)
{
    /* Initialize module */
    init();
}


/*
 *  This will hold the list of DPRAM functions
 */
void (*DPRamfunctionPtr[])(void) =
{
	CheckPowerEnable,
	CheckRangeAndMode,
    CheckUserSampleRate,
    CheckExternalPLL,
    CheckCalMode,
    EndOfDpramFunctions
};

void (*BitFunctionPtr[])(void) =
{
	D2Test,
	D3Test,
    EndOfBitFunctions
};


void module_main(void)
{
    if (mod_init)
    {
    	if(!NullPllDone)
		{
			NullPLLVadc(); 					// Call Null routine
			return;					   	 	// Kick out of functional loop
		}

    	// Run normal operation routines
    	DPRamfunctionPtr[Dpram_Func_Index]();

    	BitFunctionPtr[Bit_Func_Index]();

        /*
         * Insert module specific functional code here...
         */
    }
}

/****************************** Local functions *****************************/

static void init(void)
{
	// Power on module
	pCommon->ps_enable = 1;
	pHpsRegs->PowerEn = 1;
	usleep(500000);

	/* Load configuration parameters */
    load_params();

    /* Load calibration data */
    load_caldata();

    //  1 second measurement of external PLL
    pHpsRegs->PllFreqMeasTime = 0x07735940;

    // Get Initial Readings from external PLL
    ReadExtPllParams();

    // Extra wait for module to turn on to get PLL Frequency readings
    usleep(100000);

    // Switch Transformer clock to be our stable clock
    pHpsRegs->UseOurTxCLK = 1;

    // Force Span to update
    pHpsRegs->ForceSpanUpdate = 1;

    /* Module is now initialized */
    mod_init = TRUE;
}


static void load_params(void)
{
    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {


    }

    /* Use default values */
    pHostRegs->DASampRate = 400000;

    // Enable D2
    pCommon->test_enable = 0x4;

}


static void load_caldata(void)
{
    if ( !(pCommon->mod_ready & MOD_READY_CAL_LOADED)  || (LoadZeroCal==1) )
    {
    	LoadZeroCal=0;

        /* Use default values */
    	for(int range=0 ; range<NUM_RANGES; range++)
    	{
			for(int chan=0; chan<16 ; chan++)
			{
				pCalRegs->dacCal_VOffs_uni[range][chan] = 0x0000;
				pCalRegs->dacCal_VGain_uni[range][chan] = 0x4000;
				pCalRegs->dacCal_VOffs_bip[range][chan] = 0x0000;
				pCalRegs->dacCal_VGain_bip[range][chan] = 0x4000;

				pCalRegs->dacCal_IOffs_uni[range][chan] = 0x0000;
				pCalRegs->dacCal_IGain_uni[range][chan] = 0x4000;
				pCalRegs->dacCal_IOffs_bip[range][chan] = 0x0000;
				pCalRegs->dacCal_IGain_bip[range][chan] = 0x4000;

				pCalRegs->adCal_Offs_uni[range][chan]  = 0x0000;
				pCalRegs->adCal_Gain_uni[range][chan]  = 0x4000;
				pCalRegs->adCal_Offs_bip[range][chan]  = 0x0000;
				pCalRegs->adCal_Gain_bip[range][chan]  = 0x4000;

				pCalRegs->adCurrCal_Offs[chan] = 0x0000;	// Writes twice since there is no range index
				pCalRegs->adCurrCal_Gain[chan] = 0x4000;	// Writes twice since there is no range index
			}
    	}
    }
}

//#####################################################################################################
static void ReadExtPllParams(void)
{

	// First write nominal DAC Data for PLL frequency shifting (0V=0x0000, 1.25V=0x8000, 2.5V=0xFFFF)
	PLLDacWord = 0x8000;
	pHpsRegs->plldac_data = PLLDacWord; // Nominal MidScale = 1.25V
	pHpsRegs->plldac_start = 1;

	// Start Read sequence by writing a '1'
	pHpsRegs->starti2c_rdwr = 1;

	// Wait at least 210 us
	usleep(250);

	u32 hsdvn1 = 0;
	hsdvn1  = pHpsRegs->plli2c_rdwr_hsdvn1;

	// Get RFREQ
	ExtPll_RFREQ.half[0] = pHpsRegs->plli2c_rdwr_rfreqLO;
	ExtPll_RFREQ.half[1] = pHpsRegs->plli2c_rdwr_rfreqHI;
	ExtPll_RFREQflt = ExtPll_RFREQ.full;
	ExtPll_RFREQflt = ExtPll_RFREQflt / 268435456; // RFREQflt/2^28;

	// Get N1 and HSDV and unormalize them
	ExtPll_N1 =   (u16)(  hsdvn1 & 0x0000003F);		// Extract Value
	ExtPll_HSDV = (u16)( (hsdvn1 & 0x00000380) >> 7);// Extract Value
	ExtPll_HSDV = ExtPll_HSDV +4;						// un-Normalize to get actual value
	ExtPll_N1   = ExtPll_N1   +1;						// un-Normalize to get actual value

	// Calculate the current fxtal of the Si599 external PLL
	ExtPll_fxtal = (double)(ExtPll_HSDV *ExtPll_N1);
	ExtPll_fxtal = ExtPll_fxtal * (double)ExtPll_Fout;
	ExtPll_fxtal = ExtPll_fxtal/ExtPll_RFREQflt;

}

// This will check if the power is enabled by the user and set it in the module accordingly
static void CheckPowerEnable(void)
{
	static u32 ModPowerEn = 0;
	u32 tmpPwrEn = pCommon->ps_enable;

	if (tmpPwrEn != ModPowerEn)
	{
		if(tmpPwrEn==0)
		{
			pHpsRegs->PowerEn = 0;
			ModPowerEn = 0;
		}
		else
		{
			ModPowerEn = 1;
			pHpsRegs->PowerEn = 0x1;

			// Extra wait for module to turn on
			usleep(100000);

			// Force Span to update
			pHpsRegs->ForceSpanUpdate = 1;
		}
	}

	Dpram_Func_Index++;
}


static void CheckRangeAndMode(void)
{
	static u32 CurrentMode;
	static u32 CurrentRange[NUM_CHANNELS] = {0};

	u32 tmpMode = pHostRegs->VIMode;
	if(tmpMode != CurrentMode)
	{
		CurrentMode = tmpMode;
		NewMode_flg = TRUE;
	}

	u32 tmpRange = 0;
	NewRange_flg = FALSE;
	for(u32 i=0 ; i<(NUM_CHANNELS/2); i++)
	{
		tmpRange = pHostRegs->PolAndRange[i];

		if(tmpRange != CurrentRange[i])
		{
			CurrentRange[i]= tmpRange;
			NewRange_flg = TRUE;
		}

	}

	Dpram_Func_Index++;
}

static void CheckUserSampleRate(void)
{
    uint32_t tmpUserSampleRate, difference;

    // See if user sampling rate has changed
    tmpUserSampleRate = pHostRegs->DASampRate;

    // Make sure Sample Rate per channel is not above 400kHz
    if(tmpUserSampleRate>400000)
    {
    	tmpUserSampleRate=400000;	// Write variable with min Sample Rate per Channel
    	pHostRegs->DASampRate = tmpUserSampleRate; // Update User's register
    }

    // Check for difference in sampling rate
    difference = abs(tmpUserSampleRate-UserSampleRate);

    if(difference>0)
    {
    	NewSampRate_flg = TRUE;

    	// Update sampling rate variable
        UserSampleRate = tmpUserSampleRate;

       // Configure PLL with new sampling rate
        bool valid;
        valid = NewPLLParameters(UserSampleRate*2);
        if(valid == FALSE)
        {
            puts("!!!!!!! Unsuccessful configuration!!!!!!!");
            printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            Dpram_Func_Index++;
            return;
        }
        else
        {
            puts("\nNew Sampling rate configured Successfully");
            printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            Dpram_Func_Index++;
            return;
        }
    }

    Dpram_Func_Index++;

}

static bool NewPLLParameters(uint32_t chanRate)
{
    bool valid = 0;
    double pllRate;
    u32 hsdvn1 = 0;

    // PLL is 100 times faster than D/A Sampling Rate
    pllRate = chanRate * 100;

    // Get new PLL Parameters
	double FdcoNew = 0;
	u16 HSDIVnew = 11;
	u16 N1new = 1; // Initialize N1 for first check

	FdcoNew = pllRate * HSDIVnew * N1new;		// Calculate Fdco
	if( (FdcoNew>4.9E9) && (FdcoNew<5.6E9) )	// See if it's between 4.9GHz and 5.6GHz
	 valid = 1;									// If it is, then we're done
	else										// If not then we need to start loop through other combinations
	{
		for(HSDIVnew=11; HSDIVnew>=9; HSDIVnew-=2)
		{	for(N1new=2; N1new<=128; N1new+=2)
			{	FdcoNew = pllRate * HSDIVnew * N1new;
				 if( (FdcoNew>4.85E9) && (FdcoNew<5.67E9) )
				 {	valid = 1;
					 break;
				 }
			}

			if( valid == 1 )
				break;
		}
	 }

    if (valid == TRUE)
    {
    	// Update Variable
    	ExtPll_Fout = pllRate;

    	ExtPll_RFREQflt = FdcoNew/ExtPll_fxtal;

		// Multiply RFREQ by 2^28
		ExtPll_RFREQflt = ExtPll_RFREQflt*268435456;

		// Discard fractional portion and write all values to FPGA
		ExtPll_RFREQ.full = (int64_t)(ExtPll_RFREQflt);
		ExtPll_HSDV = HSDIVnew - 4;		// Normalize
		ExtPll_N1   = N1new - 1;			// Normalize
		hsdvn1 = (ExtPll_HSDV << 7) | ExtPll_N1;  // Format for writing

		// Write values to Si599
		pHpsRegs->plli2c_rdwr_hsdvn1 = hsdvn1;
		pHpsRegs->plli2c_rdwr_rfreqLO=ExtPll_RFREQ.half[0];
		pHpsRegs->plli2c_rdwr_rfreqHI=ExtPll_RFREQ.half[1];

		// Start write sequence and see if frequency changed
		pHpsRegs->starti2c_rdwr = 0x2;
		usleep(300);
		printf("\nWrote New PLL Frequency: %9.0f Hz \n", pllRate);

    }

    return valid;
}


/*
 * This function is used to adjust the D/A that drives the external PLL.  The D/A voltage
 *  makes small adjustments to the PLL output frequency.  When the user is not using an external
 *  synchronization clock, they will specify a specific sampling frequency so we want to be able to
 *  set the external PLL frequency as exact as we can.  To do this, we will iteratively adjust the
 *  D/A voltage to get the desired frequency we are setting the PLL to (+/-2Hz).
 *
 *  For now this is only done at power up.
 */
static void NullPLLVadc (void)
{
	static float AvgPllFreq = 0;
	static u16   AvgPllCnt  = 0;
	s32 tmpDiff=0;

	//printf("\nSTARTING PLL NULL ROUTINE\n");

	if (AvgPllCnt == 4)						// Average the measured frequency
	{

		AvgPllFreq = AvgPllFreq/4;			// Get average PLL Frequency

		printf("PLL Average: %9.0f Hz \n", AvgPllFreq);

		// Check difference from set frequency
		tmpDiff = ExtPll_Fout - (u32)AvgPllFreq;

		// Check to see if PLL is within 5 Hz of setting
		if(abs(tmpDiff)<=5)
		{
			NullPllDone = 1;						  // If it is, then we're done
			//PLLVadcLockVal = 1;						  // Update Lock Variable
			//pHostRegs->PllVadcLock = PLLVadcLockVal;  // Update external Register
			pHpsRegs->starti2c_rdwr = 0x1004; 		  // Lock PLL VADC
			usleep(100000); 						  // wait while writing lock over i2c

			printf("\nFINISHED NULLING PLL FREQUENCY and LOCKED VADC\n");
		}
		else
		{
			if(abs(tmpDiff)<250) // make sure the error isn't way off as in the case of first starting up
			{
				// Scale difference
				tmpDiff = tmpDiff*10;  // (1/0.069 = 14.49 but scaling by less) PLL DAC word is about 69mHz per DAC bit

				// Adjust PLL DAC word
				PLLDacWord += tmpDiff;

				// Write to DAC
				pHpsRegs->plldac_data =PLLDacWord;
				pHpsRegs->plldac_start = 1;
			}

		}

		AvgPllCnt  = 0;						// Reinitialize count
		AvgPllFreq = 0;						// Reinitialize average
	}
	else
	{
		AvgPllCnt++;						// Increment loop counter
		sleep(1);							// wait for 1 second
		AvgPllFreq += (pHpsRegs->PllFreqMeas*2);// Add to accumulated average

	}
}


/*
 * This is used to check the averaging time for the external PLL measurement.
 *  It will also read in the External PLL frequency measurement and write it out.
 */
void CheckExternalPLL(void)
{

	u32 tmpTime;
	tmpTime = 0;//pHostRegs->PllFreqMeasTime;

	// See if averaging time has changed
	if (PllMeasTime != tmpTime)
	{
		PllMeasTime = tmpTime;					 // Update variable

		if(PllMeasTime==0)
			pHpsRegs->PllFreqMeasTime = 0x07735940; //  1 second measurement
		else
			pHpsRegs->PllFreqMeasTime = 0x4A817C80; // 10 second measurement
	}

	// Update Frequency reading
	pHostRegs->PllFreqMeas = pHpsRegs->PllFreqMeas;

	// Go to next function
	Dpram_Func_Index++;

}

/*
 *  Used to see if we want to reset to zero cal
 */
void CheckCalMode(void)
{
	if(pHostRegs->ResetCalData == 0xAA55)
	{
		LoadZeroCal = 1;
		load_caldata();
		pHostRegs->ResetCalData = 0;
	}

	// Go to next function
	Dpram_Func_Index++;

}


/*
 *  Used to point the function pointer back to beginning
 */
void EndOfDpramFunctions(void)
{
    Dpram_Func_Index=0;
}

//############################################################################
void D2Test(void)
{
	s16 tmpDiff = 0;
	static s32 D2DiffAvg = 0;
	static s32 D2DiffAcc = 0;
	static u32 D2AccCnt  = 0;
	static u32 D2ErrCnt  = 0;
	static u32 BitDelayCntr = 0;
	static u32 DACommand[12] = {0};
	bool NewDaCmd_flg = FALSE;

	// See if D/A command has changed
	if(DACommand[BITChan] != pHostRegs->SetDAData[BITChan])
	{
		DACommand[BITChan] = pHostRegs->SetDAData[BITChan];
		NewDaCmd_flg = TRUE;
	}

	// See if range, mode, sample rate changed
	if(NewMode_flg || NewRange_flg || NewSampRate_flg || NewDaCmd_flg)
	{
		NewMode_flg = FALSE;								// Reset flag
		NewRange_flg = FALSE;								// Reset flag
		NewSampRate_flg = FALSE;							// Reset flag
		NewDaCmd_flg = FALSE;								// Reset flag
		BitDelayCntr = BIT_DELAY;							// Reset bit delay
		D2DiffAcc = 0; 										// Reset accumulator
		D2AccCnt  = 0; 	 									// Reset loop counter
		D2ErrCnt  = 0;										// Reset error count

		// Increment channel and function if necessary
		if(BITChan<NUM_CHANNELS-1)
			BITChan++;										// Increment bit channel
		else
			BITChan=0;										// Reset Bit channel

		Bit_Func_Index++;									// Go to next function
		return;
	}

	// Skip over this D2 test if D3 Test is enabled
	// Also start averaging over.
	u32 checkEn = pCommon->test_enable;
	if( (checkEn&0x8)>0 || (checkEn&0x4)==0)
	{
		D2DiffAcc = 0; 										// Reset accumulator
		D2AccCnt  = 0; 	 									// Reset loop counter
		BITChan	  = 0;										// Reset Bit channel
		D2ErrCnt  = 0;										// Reset error count
		BitDelayCntr = BIT_DELAY;							// Reset bit delay

		Bit_Func_Index++;									// Go to next function

		if ( (checkEn&0x8)==0 && (checkEn&0x4)==0 ) 		// If D3 and D2 disabled
		{
			BitWord = 0;									// Clear bit status
			pHpsRegs->BitStatus =  BitWord;					// Write to bit status register
		}

		return;
	}

	// Check delay counter
	if (BitDelayCntr!=0)
	{	BitDelayCntr--;
		return;
	}

	// Read Data
	u32 tmpData = pHpsRegs->BitData[BITChan];						// Read data from module
	//s16 tmpDacData = (s16)(tmpData & 0x0000FFFF);					// No longer getting post calibrated DAC word
	s16 tmpDacData = (s16)(pHostRegs->SetDAData[BITChan]);			// Get D/A setting
	s16 tmpAdData  = (s16)((tmpData & 0xFFFF0000)>>16);				// Get A/D Data
	u32 Range = (pHostRegs->PolAndRange[BITChan>>1]) & 0x03;		// Get range setting
	//tmpAdData = tmpAdData << Range;									// Scale A/D reading up by range
	s32 tmpAdData_s32 = ((s32)tmpAdData) << Range;

	s16 Polarity = (pHostRegs->PolAndRange[BITChan>>1]) & 0x10;		// Get polarity
	if(Polarity) // If signed
	{
		if( (pHostRegs->VIMode & (1 << BITChan)) > 0 )				// Check if in current mode
			tmpAdData_s32 =  (tmpAdData_s32*30)/25;					// Scale A/D reading up since Wrap is +/-30mA and output is +/-25mA
		else
			tmpAdData_s32 = (tmpAdData_s32*13)/10;					// Scale A/D reading up since Wrap is +/-13V and output is +/-10V

		tmpDiff	= abs((s32)tmpDacData-tmpAdData_s32);				// Get difference in measurement of signed data
	}
	else	    // else unsigned
	{
		s32 tmpAdData_s = 0;
		tmpAdData_s = (u32)tmpAdData << 1;							// Since wrap is always bipolar, need to scale reading up by 2 for unipolar
		tmpAdData_s= (tmpAdData_s*13)/10;	    					// Scale A/D reading up since Wrap is +/-13V and output is +/-10V
		if(tmpAdData_s>0xFFFF)										// See if unsigned data has overflowed
			tmpAdData = 0xFFFF;										// If it has then saturate it
		else
			tmpAdData = (s16)tmpAdData_s;							// Else, take the actual data

		tmpDiff = abs((s16)((u16)tmpDacData - (u16)tmpAdData)); 	// Get difference in measurement of unsigned data
	}

	// Accumulate difference
	if( (D2AccCnt) < (D2SAMPLECNT) )
	{
		D2DiffAcc += tmpDiff;										// Accumulate
		D2AccCnt++;													// Increment counter
	}
	else
	{
		D2AccCnt=0;													// Reset D2 Counter
		D2DiffAvg = D2DiffAcc >> D2SAMPLEBITS;						// Get Average Error
		D2DiffAcc = 0;												// Reset accumulator

		// Check average to see if it's too large and set/clr bit as necessary
		if(D2DiffAvg > MAX_D2_ERR)									// Set BIT status if failed
			D2ErrCnt++;

		if(D2ErrCnt>0)												// Set BIT status if failed
			BitWord |= (1<<BITChan);
		else														// Clear BIT status if passed
			BitWord &= ~(1<<BITChan);

		pHpsRegs->BitStatus =  BitWord;								// Write to bit status register

		D2ErrCnt = 0;												// Reset D2 Error count

		// Write 0x55 to heart beat register
		pCommon->test_verify = 0x55;

		// Increment channel and function if necessary
		if(BITChan<NUM_CHANNELS-1)
			BITChan++;
		else
		{
			BITChan=0;	// Reset Bit channel
			Bit_Func_Index++;// Go to next function
		}
	}
}

void D3Test(void)
{
	bool FinishedD3 = FALSE;
	static u16 D3TesIndex = 0;
	u32 D3TestArray[17] = {0x8000, 0x9000, 0xA000, 0xB000, 0xC000, 0xD000, 0xE000, 0xF000, 0x0000,
						 0x1000, 0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x7000, 0x7fff};
	static s32 D3DiffAcc[NUM_CHANNELS] = {0};
	static u32 D3AccCnt = 0;
	//u32 pIndx = 0;

	// See if D3 Test is enabled
	// If not then make sure to clear D3 select line!
	u32 checkEn = pCommon->test_enable;
	if((checkEn&0x8)==0)
	{
		pHpsRegs->D3Select 	 = 0;
		for(uint i=0;i<NUM_CHANNELS;i++)
			pHpsRegs->D3TestWord[i] = 0;
		Bit_Func_Index++;	// Go to next function
		FinishedD3 = TRUE;
		D3Enabled = FALSE;
		return;
	}

	u32 Polarity;
	u32 Range;

	// Initialize Loop
	if(D3Enabled == FALSE)
	{
		// Reset BitWord for D3 test
		BitWord = 0;
		pHpsRegs->BitStatus = BitWord;

		D3TesIndex = 0;													// Initialize index
		for(u32 cIndx=0;cIndx<NUM_CHANNELS;cIndx++)						// Initialize DAC word
		{
			// Get polarity
			Polarity = (pHostRegs->PolAndRange[cIndx>>1]) & 0x10;

			if(Polarity==0)												// If unsigned then flip MSB
				pHpsRegs->D3TestWord[cIndx]   = D3TestArray[D3TesIndex] ^ 0x8000;
			else
				pHpsRegs->D3TestWord[cIndx] = D3TestArray[D3TesIndex];	// Write first DAC word to all channels

			D3DiffAcc[cIndx]   = 0;										// Reset Accumulator
		}
		pHpsRegs->D3Select = 1;											// Write select line down to FPGA
		D3Enabled = TRUE;												// Set flag to indicate
		usleep(700000); 												// (now 500 ms) Waiting 50 ms in case the module has a slow bandwidth set
		FinishedD3 = FALSE;
	}
	else
	{
		u32 tmpData;
		s16 tmpDacData, tmpAdData, tmpDiff;

		// Read A/Ds and get difference
		for(u32 cIndx=0; cIndx<NUM_CHANNELS; cIndx++)
		{
			// Read Data
			tmpData 	= pHpsRegs->BitData[cIndx];						// Read data from module
			//tmpDacData 	= (s16)(tmpData & 0x0000FFFF);				// No longer getting post calibrated DAC word
			tmpDacData  = (s16)pHpsRegs->D3TestWord[cIndx];				// Get D/A setting
			tmpAdData  	= (s16)((tmpData & 0xFFFF0000)>>16);			// Get A/D Data
			Range	 	= (pHostRegs->PolAndRange[cIndx>>1]) & 0x03;	// Get range setting
			tmpAdData 	= tmpAdData << Range;							// Scale A/D reading up by range
			s32 tmpAdData_s32 = ((s32)tmpAdData) << Range;

			Polarity = (pHostRegs->PolAndRange[cIndx>>1]) & 0x10;		// Get polarity
			if(Polarity) // If signed
			{
				if( (pHostRegs->VIMode & (1 << cIndx)) > 0 )			// Check if in current mode
					tmpAdData_s32 =  (tmpAdData_s32*30)/25;				// Scale A/D reading up since Wrap is +/-30mA and output is +/-25mA
				else
					tmpAdData_s32 = (tmpAdData_s32*13)/10;				// Scale A/D reading up since Wrap is +/-13V and output is +/-10V

				tmpDiff	= abs((s32)tmpDacData-tmpAdData_s32);			// Get difference in measurement of signed data
			}
			else	    // else unsigned
			{
				s32 tmpAdData_s = 0;
				tmpAdData_s = (u32)tmpAdData << 1;						// Since wrap is always bipolar, need to scale reading up by 2 for unipolar
				tmpAdData_s= (tmpAdData_s*13)/10;	    				// Scale A/D reading up since Wrap is +/-13V and output is +/-10V
				if(tmpAdData_s>0xFFFF)									// See if unsigned data has overflowed
					tmpAdData = 0xFFFF;									// If it has then saturate it
				else
					tmpAdData = (s16)tmpAdData_s;						// Else, take the actual data

				tmpDiff = abs((s16)((u16)tmpDacData - (u16)tmpAdData)); // Get difference in measurement of unsigned data
			}

			// Accumulate difference
			D3DiffAcc[cIndx] += tmpDiff;

		}

		// Check Accumulator counter
		if( (D3AccCnt) < (D3SAMPLECNT) )
			D3AccCnt++;	// Increment Accumulator counter after all channels were checked
		else
		{
			D3AccCnt = 0;												// Reset accumulator counter
			s32 D3DiffAvg = 0;
			//pIndx=0;

			// Get averages of differences
			for(u32 cIndx=0; cIndx<NUM_CHANNELS; cIndx++)
			{
				D3DiffAvg  = D3DiffAcc[cIndx]   >> D3SAMPLEBITS;		// Get Average
				D3DiffAcc[cIndx]   = 0;									// Reset Accumulator

				// Check average to see if it's too large and set/clr bit as necessary
				if(D3DiffAvg>MAX_D2_ERR)								// Set BIT status if failed
					BitWord |= (1<<cIndx);

			}

			// Was this the last index?
			if (D3TesIndex == 16)
				FinishedD3 = TRUE;	// Finished D3
			else
			{
				// Go to next index
				D3TesIndex++;
				//pIndx=0;
				for(u32 cIndx=0; cIndx<NUM_CHANNELS; cIndx++)
				{
					// Get polarity
					Polarity = (pHostRegs->PolAndRange[cIndx>>1]) & 0x10;

					// If unsigned then flip MSB
					if(Polarity==0)
						pHpsRegs->D3TestWord[cIndx]   = D3TestArray[D3TesIndex] ^ 0x8000;
					else
						pHpsRegs->D3TestWord[cIndx]   = D3TestArray[D3TesIndex];
				}

				// Set sleep time
				usleep(700000); // (now 500ms) - Waiting 50 ms in case the module has a slow bandwidth set

			}
		}

	}

	if (FinishedD3 == TRUE)
	{
		// Write to bit status (write 1 to bit position if error)
		pHpsRegs->BitStatus =  BitWord;				// Write to bit status register

		// Clean up
		pHpsRegs->D3Select 	 = 0;					// Clear Select line
		for(u32 cIndx=0; cIndx<NUM_CHANNELS; cIndx++)
			pHpsRegs->D3TestWord[cIndx] = 0;
		D3Enabled = FALSE;; 						// Clear flag
		pCommon->test_enable = (checkEn & ~0x8); 	// Clear D3 test enable bit

		// Go to next function
		BITChan=0;									// Reset Bit channel
		Bit_Func_Index++;							// Go to next function
	}

}

void EndOfBitFunctions(void)
{
	  Bit_Func_Index=0;
}

