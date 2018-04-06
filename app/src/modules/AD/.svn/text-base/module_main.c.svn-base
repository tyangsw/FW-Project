/*
 * ADx module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Local functions
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void init(void);
static void load_params(void);
static void load_caldata(void);
static void init_fifo(void);
void init_dac(void);
static void ReadExtPllParams(void);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Module Functions
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static uint8_t  GetActChanCnt(void);
static bool     NewPLLParameters(uint32_t chanRate, uint32_t ActChanCnt);
//static bool     calcPllParameters(double adRate, double Fvco_D, uint32_t N_var);

static void		NullPLLVadc(void);
static void     CheckUsrCtrls(void);
static void     PowerResume(void);
static void     CheckUserSampleRate(void);
static void     CheckTestMode(void);
static void     CheckFilterCutoff(void);
static void     RecalculateFilterCoeff(uint32_t chan, uint32_t BreakFreq);
static void     CheckIIRUsage(void);
static void     CheckUserRange(void);
static void		SetSigRange(u32, u32);
static void     CheckFifoSize(void);
static void		CheckVImode(void);
static void		CheckExternalPLL(void);
static void     EndOfDpramFunctions(void);
static bool     D3_running  = FALSE;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Calibration Functions
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static  void AverageSamples(void);
//static uint32_t       Cal_Func_Index=0;       // Holds index for function pointer


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Background Calibration and BIT Functions
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern void BkgndCalWithBit(void);
extern void GetNextActiveChan(void);
extern void ConnectDACtoBit(void);
extern void BkgndCalBITOffs(void);
extern void SetDacVoltage(void);
extern void BkgndCalBITGain(void);
extern void ConnectChanToBIT(void);
extern void ConnectDACtoSig(void);
extern void BkgndCalSigOffs(void);
//extern void SetDacVoltage(void);
extern void BkgndCalSigGain(void);
extern void BkgndCalCleanup(void);
extern void AverageForD2(void);
extern void EndOfBkgrndCalFunctions(void);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AD Global Variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HOST_REGS 	*pHostRegs = (HOST_REGS *)HOST_REGS_BASE;
HPS_REGS  	*pHpsRegs  = (HPS_REGS *)HPS_REGS_BASE;
CAL_REGS	*pCalRegs  = (CAL_REGS *)MODULE_CAL_BASE;

u32 check_sel_array[NUM_CHANNELS]= {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3};
u32 check_en_array[NUM_CHANNELS] = {0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1};
u32 TestMode;
u32 RangeNormalizer;
u32	allow_currMode;
u8  MAXRANGE = 5;
bool VI_Mode_Changed_flg = FALSE;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AD Local Variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static bool 		mod_init = FALSE;
//static uint32_t 	MinAdSampRate = 62500; //  !!!! If changing this, we may need to adjust the Sampling Rate function below
//static uint32_t 	MaxAdSampRate = 125000;//100000; //  !!!! The hardware is also dividing the PLL clock by two because Si599 did not go as low as 8MHz!!!
static u32 	MinAdSampRate = 200000; // Unable to go this fast without cross coupling channels so lowering limits
static u32 	MaxAdSampRate = 400000; // Unable to go this fast without cross coupling channels so lowering limits


static uint32_t     powerEnableVar = 1;
static uint32_t     Dpram_Chan=0;           // Holds current channel for functions
static uint32_t     Dpram_Func_Index=0;     // Holds index for function pointer
static uint32_t     UserSampleRate=0;
static uint8_t      ActChanCnt=0;

static uint32_t     decimatorsCnt;
static uint32_t     ActiveBreakFreq[NUM_CHANNELS] = {0};
static bool         NewBreakFreq_flg[NUM_CHANNELS]= {0};
static bool         NewSampRate_flg = FALSE;
static bool         D0_enabled = FALSE;
static uint32_t     CurrentRange[NUM_CHANNELS] = {0};
static uint32_t     actChanVar;
static uint32_t		ChanMode = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static u16 ADpll_HSDV = 0;
static u16 ADpll_N1 = 0;
static union {
		int32_t half[2];
		int64_t full;
	  } ADpll_RFREQ;

static double ADpll_RFREQflt = 0;
static double ADpll_fxtal= 0;
static uint32_t ADpll_Fout = 10000000; //10MHz Startup Frequency of Si599 Oscillator
static u32 PllMeasTime = 0;
static u32 PLLVadcLockVal = 0;
static s32 PLLDacWord = 0x8000;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static u8 NullPllDone = 0;
static u32 AdMultiplierArray[2][NUM_RANGES]; //[Voltage/Current][RANGE]

// Range multiplexor selects
static u32 ADRANGE_SEL[6] = {0x4, 0x5, 0x0, 0x1, 0x2, 0x3}; // (10V, 5V, 2.5V, 1.25V, 0.625V, 0.3V) ranges


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*
 *  This will hold the list of DPRAM functions
 */
void (*DPRamfunctionPtr[])(void) =
{
    CheckUsrCtrls,
    CheckUserSampleRate,
    CheckTestMode,
    CheckFilterCutoff,
    CheckIIRUsage,
    CheckUserRange,
    CheckFifoSize,
    CheckVImode,
    CheckExternalPLL,
    EndOfDpramFunctions
};

/*
 *  This will hold the list of Calibration functions
 */
void (*CalfunctionPtr[])(void) =
{
    AverageSamples
};


/**************************** Exported functions ****************************/

void module_init(void)
{
    /* Initialize module */
    init();
}


void module_main(void)
{
    if (mod_init)
    {
    	if(!NullPllDone)
    	{
    		NullPLLVadc(); 					// Call Null routine
    		return;					   	 	// Kick out of functional loop
    	}

		if(pHostRegs->cal_en==0x0000AA55) 	// Check Calibration is enabled
		{
			CalfunctionPtr[0]();  	      	// If it is then call calibration routines
			return;							// Kick out of functional loop
		}

		// Run normal operation routines
		DPRamfunctionPtr[Dpram_Func_Index]();
		if(1)
			BkgndCalWithBit();

		if(pHpsRegs->PSRstTriggered==1)
		{
			pHostRegs->TXRstStatus = 0xACEDBEEF;
			printf("\n No Response From Top Board Resetting Tx !!!  \n");
			pHpsRegs->ClearPSRstStatus = 1;
			usleep(100000);
			PowerResume();
		}

    }
}

/****************************** Local functions *****************************/

static void init(void)
{
    /* Load configuration parameters */
    load_params();

    /* Load calibration data */
    load_caldata();

    /* Give top board time to power on */
	usleep(100000);      // Give module time to power on
	usleep(100000);      // Give module time to power on
	usleep(100000);      // Give module time to power on

    /* Initialize FIFO */
    init_fifo();

    /* Initialize DAC */
    init_dac();

    // Write module current limit
    pHpsRegs->currentLimit = 19660; //(19660)=30mA in Range 2      32432;	// ~ 0.026Amps max.  Couldn't go any higher because A/D reading was saturating at about 0x7EC0=32448

    // Default the external PLL to measure for 1 second
    pHpsRegs->PllFreqMeasTime = 0x07735940; //  1 second measurement

    // Get Initial Readings from external PLL
    ReadExtPllParams();

    // Force the range to be written
    pHpsRegs->forceUpdateADRange=1;

    // Extra wait for module to turn on to get PLL Frequency readings
    usleep(100000);

    /* Module is now initialized */
    mod_init = TRUE;
}


static void load_params(void)
{
    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */

#if MODULE_AD4
        // Choose normalizer
        pHpsRegs->Vrange_normalizer = RangeNormalizer = 0;
        // Allow current mode
        pHpsRegs->allow_currMode = allow_currMode = 1;
        AdMultiplierArray[0][0] = 0;
        AdMultiplierArray[0][1] = 0;
        AdMultiplierArray[0][2] = 0;
        AdMultiplierArray[0][3] = 0;
        AdMultiplierArray[1][0] = 1;
		AdMultiplierArray[1][1] = 1;
		AdMultiplierArray[1][2] = 1;
		AdMultiplierArray[1][3] = 1;



#elif MODULE_AD5
        // Choose normalizer
        pHpsRegs->Vrange_normalizer = RangeNormalizer = 0;
        // Block current mode
        pHpsRegs->allow_currMode = allow_currMode =  0;
        AdMultiplierArray[0][0] = 1;
        AdMultiplierArray[0][1] = 1;
        AdMultiplierArray[0][2] = 1;
        AdMultiplierArray[0][3] = 1;
        AdMultiplierArray[1][0] = 0;
		AdMultiplierArray[1][1] = 0;
		AdMultiplierArray[1][2] = 0;
		AdMultiplierArray[1][3] = 0;


#elif MODULE_AD6
        // Choose normalizer
        pHpsRegs->Vrange_normalizer = RangeNormalizer = 0;
        // Block current mode
        pHpsRegs->allow_currMode = allow_currMode =  0;
        AdMultiplierArray[0][0] = 1;
		AdMultiplierArray[0][1] = 1;
		AdMultiplierArray[0][2] = 1;
		AdMultiplierArray[0][3] = 1;
        AdMultiplierArray[1][0] = 0;
		AdMultiplierArray[1][1] = 0;
		AdMultiplierArray[1][2] = 0;
		AdMultiplierArray[1][3] = 0;


#endif

        // Set Current Mode Normalizer
        pHpsRegs->Irange_normalizer = ImodeNormalizer;

        // Set to voltage or current mode
        pHostRegs->iv_mode = 0x0000;            // 0=Voltage, 1=Current

        // Set all channels to Active
        pHostRegs->activeChan = 0xFFFF;         // bitmapped (0=inactive, 1=active)
        pHpsRegs->activeChan = 0xFFFF;          // Write active channels to FPGA

        // Set default polarity and range
        for(uint32_t i=0; i<NUM_CHANNELS; i++)
        {
            pHostRegs->ad_polrange[i] = 0x10;   					 // 2's compliment, range 0 (b[1..0] = range, b[4] = polarity)
            pHpsRegs->ad_range[i].reg = (0x10 | ADRANGE_SEL[0]);       // Also write it to FPGA
            pHpsRegs->adMultiplier[i] = AdMultiplierArray[0][RangeNormalizer];
        }

        // Set sampling rate to 10kHz
        pHostRegs->sampleRatePerChan = 10000;   // 1 Hz resolution

        // Default to power on
		pCommon->ps_enable = 1;
		pHpsRegs->powerEnable = 1;
		powerEnableVar = 1;

        // Turn on D2 test
        pCommon->test_enable = D2_EN_BIT;

        // Write default threshold detect information
        for(u32 i=0; i<NUM_CHANNELS; i++)
		{
			pHostRegs->ThresholdDetect1[i]  =  0x7332;   					// 90% of bipolar full scale
			pHostRegs->ThresholdDetCtrl1[i] = 0x00000;   					// Rising edge, no hysteresis
			pHostRegs->ThresholdDetect2[i]  =  0x8CCE;   					// -90% of bipolar full scale
			pHostRegs->ThresholdDetCtrl2[i] = 0x10000;   					// Falling edge, no hysteresis

		}

    }
}


static void load_caldata(void)
{
    if (!(pCommon->mod_ready & MOD_READY_CAL_LOADED))
    {
        /* Use default values */

    	// Write 64 offsets (0x00000000)
    	for (uint32_t ch=0; ch<NUM_CHANNELS; ch++)
    	{

    		for(uint32_t r=0; r<NUM_RANGES; r++)
    		{

    			//tmpRob = 1;
    			pCalRegs->ad_offset[ch][r] = 0x00000000;
    		}
    	}

    	// Write 64 Gains (0x00005B00)
    	for (uint32_t ch=0; ch<NUM_CHANNELS; ch++)
		{
			for(uint32_t r=0; r<NUM_RANGES; r++)
			{
				pCalRegs->ad_gain[ch][r] = 0x00004000;
			}
		}

    	// Write 64 offsets (0x00000000)
    	for (uint32_t ch=0; ch<NUM_CHANNELS; ch++)
		{
			for(uint32_t r=0; r<NUM_RANGES; r++)
			{
				pCalRegs->bit_offset[ch][r] = 0x00000000;
			}
		}

    	// Write 64 Gains (0x00005B00)
    	for (uint32_t ch=0; ch<NUM_CHANNELS; ch++)
		{
			for(uint32_t r=0; r<NUM_RANGES; r++)
			{
				pCalRegs->bit_gainCal[ch][r] = 0x00004000;
			}
		}

    	 // Write 64 offsets (0x00000000)
		for (uint32_t ch=0; ch<NUM_CHANNELS; ch++)
		{
			for(uint32_t r=0; r<NUM_RANGES; r++)
			{
				pCalRegs->Imode_offset[ch][r] = 0x00000000;
			}
		}

		// Write 64 Gains (0x00005B00)
		for (uint32_t ch=0; ch<NUM_CHANNELS; ch++)
		{
			for(uint32_t r=0; r<NUM_RANGES; r++)
			{
				pCalRegs->Imode_gain[ch][r] = 0x00004000;
			}
		}

   	    // Write 64 offsets (0x00000000)
		for (uint32_t ch=0; ch<NUM_CHANNELS; ch++)
		{
			for(uint32_t r=0; r<NUM_RANGES; r++)
			{
				pCalRegs->Imode_bit_offset[ch][r] = 0x00000000;
			}
		}

		// Write 64 Gains (0x00005B00)
		for (uint32_t ch=0; ch<NUM_CHANNELS; ch++)
		{
			for(uint32_t r=0; r<NUM_RANGES; r++)
			{
				pCalRegs->Imode_bit_gainCal[ch][r] = 0x00004000;
			}
		}

    	// Write DAC cal for BIT
    	for (u32 i=0; i<NUM_RANGES; i++)
    	{
    	 pCalRegs->bit_DacCal[0][i] = 0x00000000; // Offset Range 0-4
    	 pCalRegs->bit_DacCal[1][i] = 0x00004000; //   Gain Range 0-4
    	}

    	// Write DAC cal for SIG
    	for (u32 i=0; i<NUM_CHANNELS; i++)
		{
		 pCalRegs->sig_DacCal[0][i] = 0x00000000; // Offset channels 0-15
		 pCalRegs->sig_DacCal[1][i] = 0x00004000; //   Gain channels 0-15
		}
    }
}


static void init_fifo(void)
{
    int i;

    for(i=0; i<NUM_CHANNELS; i++)
    {
        // Write starting address for FIFOs
        pHpsRegs->fifo_strtAddy[i] = 0x1 + i;

        // Write max size to 2Meg for each channel
        pHpsRegs->fifo_fullCnt[i]=0x0FFFFF;

        // Write max user max size for each channel
        pHostRegs->fifoBufferSize[i]=0x0FFFFF;
    }

    // Reset and enable FIFO controller
    pHpsRegs->fifo_init.reg=2; // reset
    pHpsRegs->fifo_init.reg=0; // clear reset
    pHpsRegs->fifo_init.reg=5; // enable

    puts("\nFIFO Initialized\n");
}


void init_dac(void)
{
    // Put DAC into configuration mode
    //mw.l 0xFF300004 0x00000003
    pHpsRegs->dac_rstn=0x3;
    usleep(500); // Delay 500us so the reset is cleared and the DAC register is written

    // Put DAC into Normal Mode
    //mw.l 0xFF300004 0x00000001
    pHpsRegs->dac_rstn=0x1;
    puts("DAC Initialized\n");
}


static void ReadExtPllParams(void)
{

	// Start Read sequence by writing a '1'
	pHpsRegs->starti2c_rdwr = 1;

	// Wait at least 210 us
	usleep(250);

	u32 hsdvn1 = 0;
	hsdvn1  = pHpsRegs->plli2c_rdwr_hsdvn1;

	// Get RFREQ
	ADpll_RFREQ.half[0] = pHpsRegs->plli2c_rdwr_rfreqLO;
	ADpll_RFREQ.half[1] = pHpsRegs->plli2c_rdwr_rfreqHI;
	ADpll_RFREQflt = ADpll_RFREQ.full;
	ADpll_RFREQflt = ADpll_RFREQflt / 268435456; // RFREQflt/2^28;

	// Get N1 and HSDV and unormalize them
	ADpll_N1 =   (u16)(  hsdvn1 & 0x0000003F);		// Extract Value
	ADpll_HSDV = (u16)( (hsdvn1 & 0x00000380) >> 7);// Extract Value
	ADpll_HSDV = ADpll_HSDV +4;						// un-Normalize to get actual value
	ADpll_N1   = ADpll_N1   +1;						// un-Normalize to get actual value

	// Calculate the current fxtal of the Si599 external PLL
	ADpll_fxtal = (double)(ADpll_HSDV *ADpll_N1);
	ADpll_fxtal = ADpll_fxtal * (double)ADpll_Fout;
	ADpll_fxtal = ADpll_fxtal/ADpll_RFREQflt;

	// Write nominal DAC Data for PLL frequency shifting (0V=0x0000, 1.25V=0x8000, 2.5V=0xFFFF)
	PLLDacWord = 0x8000;
	pHpsRegs->plldac_data = PLLDacWord; // Nominal MidScale = 1.25V
	pHpsRegs->plldac_start = 1;

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
		tmpDiff = ADpll_Fout - (u32)AvgPllFreq;

		// Check to see if PLL is within 2 Hz of setting
		if(abs(tmpDiff)<=5)
		{
			NullPllDone = 1;						  // If it is, then we're done
			PLLVadcLockVal = 1;						  // Update Lock Variable
			pHostRegs->PllVadcLock = PLLVadcLockVal;  // Update external Register
			pHpsRegs->starti2c_rdwr = 0x1004; 		  // Lock PLL VADC
			usleep(300); 							  // wait while writing lock over i2c

			// Force the range to be written
			usleep(100000);
			pHpsRegs->forceUpdateADRange=1;

			printf("\nFINISHED NULLING PLL FREQUENCY and LOCKED VADC\n");

			//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    // Configure PLL with new sampling rate
			bool valid;
			valid = NewPLLParameters(pHostRegs->sampleRatePerChan, 8);
			if(valid == FALSE)
			{
			   puts("!!!!!!! Unsuccessful configuration!!!!!!!");
			   printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			}
			else
			{
			   NewSampRate_flg = TRUE; // Set flag to calculate a new break frequency for the post IIR filter
			   puts("\nNew Sampling rate configured Successfully");
			   printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			}

			// Update variables so the same frequency does not get written a second time when in main loop
			ActChanCnt = 8;
			UserSampleRate = pHostRegs->sampleRatePerChan;
			//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

			// Switch Transformer clock to be our stable clock
			pHpsRegs->UseOurTxCLK = 1;
			pHostRegs->UseOurTxCLK=1;


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
		AvgPllFreq += pHpsRegs->PllFreqMeas;// Add to accumulated average

	}
}

static void CheckUsrCtrls(void)
{

    union
    {   uint32_t reg;
        struct {
                uint8_t    BankA      :  8; // Bit  0-7
                uint8_t    BankB      :  8; // Bit  8-15
                uint16_t   _reserved  : 16; // Bits 16-31
                } banks;
    } tmpActChan;

    uint32_t tmpVal;

    // Check power enable control
    tmpVal = pCommon->ps_enable;
    if(tmpVal != powerEnableVar)
    {
        powerEnableVar = tmpVal;                // update global variable
        pHpsRegs->powerEnable = tmpVal;         // Write to FPGA
        if(powerEnableVar>0)                    // Check if power was being turned back on
            PowerResume();
    }

    tmpActChan.reg = pHostRegs->activeChan;
    if(tmpActChan.reg != actChanVar)
    {
        actChanVar = tmpActChan.reg;                    // update global variable
        pHpsRegs->activeChan = tmpActChan.reg;          // Write to FPGA

        // Write first channel of each bank and issue a reset to the TB_ChanSelect
        //  This is so the active channels between banks are synchronized when going into the FIFO
        bool ChanFound = FALSE;
        int16_t FirstchkIndx = 0;
        uint16_t tstVal = 0;
        while( (!ChanFound) && (FirstchkIndx<8) )
        {
        	 tstVal = tmpActChan.banks.BankA & (1<<FirstchkIndx);
        	 if(tstVal>0)
        		 ChanFound = TRUE;
        	 else
        		 FirstchkIndx++;
        }

        // This is to find the last active channel which will also help with re-synchronizing the FIFOs
        ChanFound = FALSE;
        int16_t LastchkIndx = 15;
        uint16_t chkIndx = 0;
        tstVal = 0;
        while( (!ChanFound) && (LastchkIndx>=0) )
        {
        	 tstVal = tmpActChan.banks.BankB & (128>>chkIndx);
        	 if(tstVal>0)
        		 ChanFound = TRUE;
        	 else
        	 {
        		 LastchkIndx--;
        		 chkIndx++;
        	 }
        }

        if(ChanFound)
        {
        	// Write new first channel so FIFO can line up banks when storing
        	pHpsRegs->firstBankChan = FirstchkIndx;
        	pHpsRegs->lastBankChan = LastchkIndx;

        	// Reset TB_ChanSelect
        	pHpsRegs->bankSync = 1;
        	pHpsRegs->bankSync = 0;
        }

    }

    Dpram_Func_Index++;
}

// This function is when the power is re-enabled
static void PowerResume(void)
{
	usleep(100000);      // Give module time to power on
	usleep(100000);      // Give module time to power on
	usleep(100000);      // Give module time to power on
	init_dac();          // Re-initialize the DAC
	ReadExtPllParams();	 // Get initial PLL parameters again
	pHpsRegs->forceUpdateADRange = 1; // Force the CPLD range to write again
}

static void CheckUserSampleRate(void)
{
    uint32_t tmpUserSampleRate, difference;
    uint32_t read_ActChanCnt=0;

    // See if user sampling rate has changed
    tmpUserSampleRate = pHostRegs->sampleRatePerChan;

    // Make sure Sample Rate per channel is not below 1kHz limit
    if(tmpUserSampleRate<MIN_SAMPRATE)
    {
    	tmpUserSampleRate=MIN_SAMPRATE;					  // Write variable with min Sample Rate per Channel
    	pHostRegs->sampleRatePerChan = tmpUserSampleRate; // Update User's register
    }

    // Check for difference in sampling rate
    difference = abs(tmpUserSampleRate-UserSampleRate);

    // Get number of active channels
    read_ActChanCnt = GetActChanCnt();

    // Check if we are over the maximum A/D Sampling Rate
    uint32_t calcSampRate = read_ActChanCnt*tmpUserSampleRate;
    if( calcSampRate > MaxAdSampRate)
    {
    	tmpUserSampleRate = (MaxAdSampRate/read_ActChanCnt) ;				// Set new sampling rate
    	pHostRegs->sampleRatePerChan = (MaxAdSampRate/read_ActChanCnt);		// Write back maximum rate to user register
    }

    if(difference>0 || (read_ActChanCnt!=ActChanCnt) )
    {
    	// Update Active Channel Count
    	ActChanCnt = read_ActChanCnt;

        // Update sampling rate variable
        UserSampleRate = tmpUserSampleRate;

        // Get number of active channels
        if (ActChanCnt == 0)
        {
            puts("No channels Active so exiting");
            Dpram_Func_Index++;
            return;
        }

        // Configure PLL with new sampling rate
        bool valid;
        valid = NewPLLParameters(UserSampleRate, ActChanCnt);
        if(valid == FALSE)
        {
            puts("!!!!!!! Unsuccessful configuration!!!!!!!");
            printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            Dpram_Func_Index++;
            return;
        }
        else
        {
            NewSampRate_flg = TRUE; // Set flag to calculate a new break frequency for the post IIR filter
            puts("\nNew Sampling rate configured Successfully");
            printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            Dpram_Func_Index++;
            return;
        }
    }

    Dpram_Func_Index++;

}


/*
 * Get the amount of active channels that will be used
 * Count requires that the total is the same between each bank
 * i.e. if the active channel word is 0x0307 then three channels are
 * active in bank A and two channels are active in bank B.  Therefore the total number
 * of active channels will be the greater of the two banks which is 3 channels.
 */
static uint8_t GetActChanCnt(void)
{
    uint8_t     bitcnt[16]={0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
    uint8_t     tmpCnt1,tmpCnt2,tmpCnt3,tmpCnt4;
    uint16_t 	tmpActChanCnt;

    // Get the amount of active channels that will be used
    //  Count requires that the total is the same between each bank
    union
    {   uint32_t reg;
        struct {
                uint8_t    lowerA     :  4; // Bit  0-3
                uint8_t    lowerB     :  4; // Bit  4-7
                uint8_t    lowerC     :  4; // Bit  8-11
                uint8_t    lowerD     :  4; // Bit  12-15
                uint16_t   _reserved  : 16; // Bits 16-31
                } bits;
    } myActiveChan;


    // Get the active channels
    myActiveChan.reg = pHostRegs->activeChan;               // Read active channels register
    tmpCnt1 = bitcnt[myActiveChan.bits.lowerA];             // Get first nibble count
    tmpCnt2 = tmpCnt1 + bitcnt[myActiveChan.bits.lowerB];   // Get second nibble count and add it to first
    tmpCnt3 = bitcnt[myActiveChan.bits.lowerC];             // Get third nibble count
    tmpCnt4 = tmpCnt3 + bitcnt[myActiveChan.bits.lowerD];   // Get fourth nibble count and add it to third

    if(tmpCnt2>=tmpCnt4)                                    // Compare Bank A and Bank B active channel counts
        tmpActChanCnt = tmpCnt2;                               //  and use whichever has the larger amount
    else
        tmpActChanCnt = tmpCnt4;

    return tmpActChanCnt;
}


static bool NewPLLParameters(uint32_t chanRate, uint32_t ActChanCnt)
{
    bool valid = 0;
    //double Fvco_D;
    double pllRate;
    double adRate;
    //uint32_t N_var;
    u32 hsdvn1 = 0;
    u8 DisDec = 0;

    // Calculate the A/D sampling rate based on the number of channels and rate per channel
    adRate = chanRate * ActChanCnt; // Channel rate x number of most active channels

    // Get number of decimation stages needed
    if (adRate<MinAdSampRate)
    {	DisDec=0;
        decimatorsCnt = ceil(log2(MinAdSampRate/adRate))-1;
    }
    else
    {	DisDec=1;
        decimatorsCnt = 0;
    }
    pHpsRegs->disableDecimation= DisDec;


static u32 arrMaxPolyCnt[8]   = {2, 4, 8, 16, 32, 64, 128, 256};
static u32 arrPolyBlkSize[8]  = {63, 63, 63, 63, 63, 63, 31, 15};
static u32 arrPolyCoeffLen[8] = {61, 60, 52, 45, 36, 46, 29, 15};
static u32 arrPolyMacLen[8]   = {65, 64, 56, 49, 40, 50, 33, 19};

	u32 tmpVal;
	// Get values for FPGA Decimation
	tmpVal = arrMaxPolyCnt[decimatorsCnt];
	pHpsRegs->MaxPolyCnt = tmpVal;// (0x0000004C)

	tmpVal = arrPolyBlkSize[decimatorsCnt];
	pHpsRegs->PolyBlkSize = tmpVal;	// (0x00000050)

	tmpVal = arrPolyCoeffLen[decimatorsCnt];
	pHpsRegs->PolyCoeffLen = tmpVal;// (0x00000054)

	tmpVal = arrPolyMacLen[decimatorsCnt];
	pHpsRegs->PolyMacLen = tmpVal;// (0x00000058)

    // Display and write data
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
	if(DisDec==0)
		printf("\nDecimation " "\x1b[32m" "ENABLED" "\x1b[0m");
	else
		printf("\nDecimation " "\x1b[31m"  "DISABLED" "\x1b[0m");

    printf("\nDecimation of : %ld \n", arrMaxPolyCnt[decimatorsCnt]);
    pHpsRegs->decimator_sel=decimatorsCnt;

    // Display amount of decimation to user
    if(DisDec==1)
    	pHostRegs->decimator_cnt = 0xDEAD;
    else
    	pHostRegs->decimator_cnt = arrMaxPolyCnt[decimatorsCnt];

    // Calculate PLL Rate
    if(DisDec==0)
    	pllRate = (adRate*128*exp2(decimatorsCnt+1)); //
    else
    	pllRate = (adRate*128*exp2(decimatorsCnt)); //

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
    	ADpll_Fout = pllRate;

    	// RFREQnew = fdcoNew/fxtal
		ADpll_RFREQflt = FdcoNew/ADpll_fxtal;

		// Multiply RFREQ by 2^28
		ADpll_RFREQflt = ADpll_RFREQflt*268435456;

		// Discard fractional portion and write all values to FPGA
		ADpll_RFREQ.full = (int64_t)(ADpll_RFREQflt);
		ADpll_HSDV = HSDIVnew - 4;		// Normalize
		ADpll_N1   = N1new - 1;			// Normalize
		hsdvn1 = (ADpll_HSDV << 7) | ADpll_N1;  // Format for writing

		// Write values to Si599
		pHpsRegs->plli2c_rdwr_hsdvn1 = hsdvn1;
		pHpsRegs->plli2c_rdwr_rfreqLO=ADpll_RFREQ.half[0];
		pHpsRegs->plli2c_rdwr_rfreqHI=ADpll_RFREQ.half[1];

		// Start write sequence and see if frequency changed
		pHpsRegs->starti2c_rdwr = 0x2;
		usleep(300);
		printf("\nWrote New PLL Frequency: %9.0f Hz \n", pllRate);
		printf("\n        A/D Sampling at: %9.0f Hz \n", pllRate/128);


		if (DisDec==0)  // If decimation enabled
			printf("\nSample Rate Per Channel: %9.0f Hz \n", ((pllRate/128)/ActChanCnt)/arrMaxPolyCnt[decimatorsCnt]);
		else			// else no decimation
			printf("\nSample Rate Per Channel: %9.0f Hz \n", (pllRate/128)/ActChanCnt);

	    // Write actual A/D Rate to register for displaying
   	    pHostRegs->ad_rate = pllRate/128;

    }

    return valid;
}

#ifdef RobsOldCode
static bool calcPllParameters(double pllRate, double Fvco_D, uint32_t N_var)
{
    bool valid = TRUE;
    double Fref = 125000000;
    double M_orig, Fvco, K_var, Residual;

    M_orig = round((Fvco_D*N_var)/Fref);            // Calculate nearest M value to start with
    Fvco = Fref*(M_orig/N_var);                     // Actual Fvco based on Integer M value
    C_var = ceil(Fvco/pllRate);                     // Get ratio for C scale

    K_var = (pllRate*C_var/Fref)*N_var-M_orig;      // Get floating point K value
    K_uint = (uint32_t)(floor(K_var*(exp2(32))));   // Get integer K value
    if (K_uint == 0)                                // Test K. It will be 0 if pllRate is a multiple of Fin
        K_uint=1;                                   //  If this is true we need to set it to 1

    M_orig = (M_orig + K_var );                     // Get full M value

    Fvco = Fref*(M_orig/N_var);                     // Vco frequency
    Fout = Fvco/C_var;                              // Actual Output frequency
    Residual = pllRate-Fout;                        //  error between desired and actual output frequency

    // Check to make sure the output frequency is within tolerance
    if (Residual>0.5)
        valid=FALSE;

    // Test to make sure each scale is within range
    M_var = (uint32_t)(M_orig);
    if(M_var>255 || C_var>255 || N_var>255)
        valid=FALSE;

    return valid;
}
#endif

#ifdef randolina
static void CheckTestMode(void)
{
    static uint32_t D3_ExpectedVoltage[MAX_D3_VOLTAGES] =   {0xFFFF8100,0xFFFF8CCE,0xFFFF999B,0xFFFFA668,0xFFFFB334,0xFFFFC001,0xFFFFCCCE,0xFFFFD99A,0xFFFFE667,0xFFFFF334,
                                                  0x00000000,0x00000CCC,0x00001999,0x00002666,0x00003332,0x00003FFF,0x00004CCC,0x00005998,0x00006665,0x00007332,0x00007F00};

    // Get test enable variable and write to global variable
    TestMode = pCommon->test_enable;

    // Put register into temporary variable for manipulating later
    u32 tmpTestMode;
    tmpTestMode = TestMode;

    // See if D0 or D3 test is active
    if(TestMode&D3_EN_BIT) // If D3 test enabled
    {

        static uint32_t D3_index    = 0;
        static uint32_t D3_timer    = 0;
        static uint32_t D3_Status   = 0;

        if(D3_running == FALSE)
        {
            // Init variables
            D3_index = 0;
            D3_running = TRUE;
            D3_timer = 4096;
            D3_Status = 0;

            // Set expected voltage to initialize
            pHpsRegs->dac_data=(D3_ExpectedVoltage[D3_index]>>RangeNormalizer);

            // Place DAC onto BIT signal
            pHpsRegs->bit_sel=0x2;

            // Now place BIT signal into A/D Channels (this will connect DAC to those channels)
            pHpsRegs->connect_dac=0xFFFF;

            // Write Range and Gain from User
            // b5:Enable, b4:Polarity, b[3..0]:Range
            pHpsRegs->bit_gain = (0x0030 | RangeNormalizer);  // write the enable bit (b5), 2's compliment enable bit (b4), Fix range to 1 (b3..0)
            pHpsRegs->DacRange = 0;

            // Set Cal channel upper BIT to indicate that it's connected to the DAC
            pHpsRegs->bit_chan = 0x0010;                                    // This will point to DAC cal data

        }
        else
        {
            if(D3_timer == 0)
            {

                // Init Error Array
                uint32_t D3_Error[NUM_CHANNELS] = {0};

                // Check error for EACH channel
                for(uint32_t i=0; i<NUM_CHANNELS; i++)
                {
                    D3_Error[i]  =abs(pHpsRegs->ad_reading[i] - D3_ExpectedVoltage[D3_index]);
                    if(D3_Error[i] > D3_ERROR_LIMIT)
                        D3_Status |= (1<<i);                                // Set bit to indicate a failure on current channel;
                }

                // Increment index for the next time around and reset timer
                D3_index++;

                if(D3_index>=MAX_D3_VOLTAGES)                               // Check if we are done going through voltages
                {
                    // Write BIT status
                    pHpsRegs->bit_status = D3_Status;

                    // Disconnect DAC
                    pHpsRegs->connect_dac=0x0000;

                    // Reset values
                    pHpsRegs->bit_chan = 0x0000;

                    // Reset DAC connections and variables
                    D3_running = FALSE;

                    // Clear register to indicate D3 test is no longer running
                    tmpTestMode &= ~(D3_EN_BIT);
                    pCommon->test_enable = tmpTestMode;
                }
                else
                {
                    pHpsRegs->dac_data=(D3_ExpectedVoltage[D3_index]>>RangeNormalizer);        // Set the expected voltage
                    D3_timer = 4096;                                        // Reset Timer
                }
            }
            else
                D3_timer--;
        }

    }
    else
    {
        // Clear D3 flag
    	if(D3_running == TRUE)
    	{
			// Disconnect DAC
			pHpsRegs->connect_dac=0x0000;

			// Reset values
			pHpsRegs->bit_chan = 0x0000;

    		D3_running = FALSE;
    	}


        if(TestMode&D0_EN_BIT) // If D0 test enabled user can write DAC value which is connected to A/D input
        {
            // Place DAC onto BIT signal
            pHpsRegs->bit_sel=0x2;

            // Now place BIT signal into A/D Channels (this will connect DAC to those channels)
            pHpsRegs->connect_dac=0xFFFF;

            // Write Range and Gain from User
            // b5:Enable, b4:Polarity, b[3..0]:Range
            uint32_t tmpRange = pCommon->test_range;   // Get Polarity from user (range is no longer used)
            tmpRange = tmpRange & 0x00000010;           // Just pull out the polarity bit
            tmpRange = tmpRange | 0x00000020;           // Fix range to 0 (b3..0) and write the enable bit (b5)
            pHpsRegs->bit_gain = (tmpRange|RangeNormalizer);              // Write range setting
            pHpsRegs->DacRange = tmpRange;

            // Set Cal channel upper BIT to indicate that it's connected to the DAC
            pHpsRegs->bit_chan = 0x0010;                // This will point to DAC cal data

            // Write DAC value from User
            pHpsRegs->dac_data=( (pCommon->test_value)>>RangeNormalizer );

            // Set flag to indicate D0 is active
            D0_enabled = TRUE;

        }
        else
        {
            if (D0_enabled == TRUE)
            {
                // Disconnect dac and gnd connections to re-enabled normal mode
                pHpsRegs->connect_dac=0x0000;
                pHpsRegs->connect_gnd=0x0000;

                // Reset values
                pHpsRegs->bit_chan = 0x0000;             // Reset Cal channel
                pHpsRegs->bit_gain = (0x10 | RangeNormalizer); // Reset bit gain and disable b[5] to read A/D properly
                pHpsRegs->DacRange = 0;

                // Clear flag to indicate D0 is no longer active
                D0_enabled = FALSE;
            }
        }
    }
    // Increment function pointer
    Dpram_Func_Index++;
}
#endif

static void CheckTestMode(void)
{
    static uint32_t D3_ExpectedVoltage[MAX_D3_VOLTAGES] =   {0xFFFF8100,0xFFFF8CCE,0xFFFF999B,0xFFFFA668,0xFFFFB334,0xFFFFC001,0xFFFFCCCE,0xFFFFD99A,0xFFFFE667,0xFFFFF334,
                                                  0x00000000,0x00000CCC,0x00001999,0x00002666,0x00003332,0x00003FFF,0x00004CCC,0x00005998,0x00006665,0x00007332,0x00007F00};

    // Get test enable variable and write to global variable
    TestMode = pCommon->test_enable;

    // Put register into temporary variable for manipulating later
    u32 tmpTestMode;
    tmpTestMode = TestMode;

    // See if D0 or D3 test is active
    if(TestMode&D3_EN_BIT) // If D3 test enabled
    {

        static uint32_t D3_index    = 0;
        static uint32_t D3_timer    = 0;
        static uint32_t D3_Status   = 0;

        if(D3_running == FALSE)
        {
            // Init variables
            D3_index = 0;
            D3_running = TRUE;
            D3_timer = 4096;
            D3_Status = 0;

            // Set range of all channels
            for(u16 tmpchan=0; tmpchan<NUM_CHANNELS; tmpchan++)
            	SetSigRange(tmpchan, 0x10);

            // Set expected voltage to initialize
            pHpsRegs->dac_data=(D3_ExpectedVoltage[D3_index]);//>>1); // Divide by 2 when DAC signal is connected to signal

            // Place DAC onto BIT signal
            pHpsRegs->bit_sel=0x2;

            // Now place BIT signal into A/D Channels (this will connect DAC to those channels)
            pHpsRegs->connect_dac=0xFFFF;

            // Write Range and Gain from User
            // b5:Enable, b4:Polarity, b[3..0]:Range
            pHpsRegs->bit_gain = 0x0034;  // write the enable bit (b5), 2's compliment enable bit (b4), Fix range to 1 (b3..0)
            pHpsRegs->DacRange = 0;

            // Set Cal channel upper BIT to indicate that it's connected to the DAC
            pHpsRegs->bit_chan = 0x0010;                                    // This will point to DAC cal data

        }
        else
        {
            if(D3_timer == 0)
            {

                // Init Error Array
                uint32_t D3_Error[NUM_CHANNELS] = {0};

                // Check error for EACH channel
                for(uint32_t i=0; i<NUM_CHANNELS; i++)
                {
                    D3_Error[i]  =abs(pHpsRegs->ad_reading[i] - D3_ExpectedVoltage[D3_index]);
                    if(D3_Error[i] > D3_ERROR_LIMIT)
                        D3_Status |= (1<<i);                                // Set bit to indicate a failure on current channel;
                }

                // Increment index for the next time around and reset timer
                D3_index++;

                if(D3_index>=MAX_D3_VOLTAGES)                               // Check if we are done going through voltages
                {
                    // Write BIT status
                    pHpsRegs->bit_status = D3_Status;

                    // Disconnect DAC
                    pHpsRegs->connect_dac=0x0000;

                    // Reset values
                    pHpsRegs->bit_chan = 0x0000;

                    // Set range back to user's range
					for(u16 tmpchan=0; tmpchan<NUM_CHANNELS; tmpchan++)
						SetSigRange(tmpchan, CurrentRange[tmpchan]);

                    // Reset DAC connections and variables
                    D3_running = FALSE;

                    // Clear register to indicate D3 test is no longer running
                    tmpTestMode &= ~(D3_EN_BIT);
                    pCommon->test_enable = tmpTestMode;
                }
                else
                {	// Set the expected voltage
                    pHpsRegs->dac_data=(D3_ExpectedVoltage[D3_index]);//>>1); // Divide by 2 when DAC signal is connected to signal
                    D3_timer = 4096;                                      // Reset Timer
                }
            }
            else
                D3_timer--;
        }

    }
    else
    {
        // Clear D3 flag
    	if(D3_running == TRUE)
    	{
    		// Set range back to user's range
    		for(u16 tmpchan=0; tmpchan<NUM_CHANNELS; tmpchan++)
    			SetSigRange(tmpchan, CurrentRange[tmpchan]);

			// Disconnect DAC
			pHpsRegs->connect_dac=0x0000;

			// Reset values
			pHpsRegs->bit_chan = 0x0000;

    		D3_running = FALSE;
    	}


        if(TestMode&D0_EN_BIT) // If D0 test enabled user can write DAC value which is connected to A/D input
        {
            // Place DAC onto BIT signal
//ra no longer needed after respin           pHpsRegs->bit_sel=0x2;

            // Now place BIT signal into A/D Channels (this will connect DAC to those channels)
            pHpsRegs->connect_dac=0xFFFF;

            // Write Range and Gain from User
            // b5:Enable, b4:Polarity, b[3..0]:Range
            uint32_t tmpRange = pCommon->test_range;   // Get Polarity from user (range is no longer used)
            tmpRange = tmpRange & 0x00000010;           // Just pull out the polarity bit
            tmpRange = tmpRange | 0x00000024;           // Fix range to 0 (b3..0) and write the enable bit (b5)
            pHpsRegs->bit_gain = (tmpRange);              // Write range setting
            pHpsRegs->DacRange = 0;

            // Need to set Signal A/D range
            for(u16 tmpchan=0; tmpchan<NUM_CHANNELS; tmpchan++)
            	SetSigRange(tmpchan, 0x10);

            // Set Cal channel upper BIT to indicate that it's connected to the DAC
//ra no longer needed after respin pHpsRegs->bit_chan = 0x0010;                // This will point to DAC cal data

            // Write DAC value from User
            u32 tmpTestVal = pCommon->test_value;
            if(tmpTestVal&0x8000) // Since test value is signed, sign extend sign bit if MSb is a '1'
            	tmpTestVal |= 0xFFFF0000;

            pHpsRegs->dac_data=(( tmpTestVal));//>>1 )); // Divide by 2 when DAC signal is connected to signal

            // Set flag to indicate D0 is active
            D0_enabled = TRUE;

        }
        else
        {
            if (D0_enabled == TRUE)
            {
            	// Need to reset Signal A/D range to user's set range
            	for(u16 tmpchan=0; tmpchan<NUM_CHANNELS; tmpchan++)
            		SetSigRange(tmpchan, CurrentRange[tmpchan]);

                // Disconnect dac and gnd connections to re-enabled normal mode
                pHpsRegs->connect_dac=0x0000;
                pHpsRegs->connect_gnd=0x0000;

                // Reset values
                pHpsRegs->bit_chan = 0x0000;             // Reset Cal channel
                pHpsRegs->bit_gain = (0x10 | ADRANGE_SEL[RangeNormalizer]); // Reset bit gain and disable b[5] to read A/D properly
                pHpsRegs->DacRange = 0;

                // Clear flag to indicate D0 is no longer active
                D0_enabled = FALSE;
            }
        }
    }
    // Increment function pointer
    Dpram_Func_Index++;
}



/*
 *  This will check the break frequency for the post IIR filter.
 *  If it changes, than it will set a flag that will be used to
 *  recalculate the filter coefficients.  The flag may also be set
 *  when the sampling rate gets regenerated since this would require
 *  different coefficients to keep the same cutoff.
 */
void CheckFilterCutoff(void)
{
    static uint32_t filterEnableCTRL = 0;   // Initialize filter as disabled
    static uint32_t ChannelCount = 0;
    static uint32_t FilterMode[NUM_CHANNELS] = {0};
    static bool     CycleThroughChans = FALSE;
           uint32_t readBreakFreq;

    //If mode changed from voltage mode to current mode then set a 1kHz cutoff only if sample rate is above 2.5kHz
    uint32_t TempCheck = (ChanMode >> Dpram_Chan) & 1;
    if(FilterMode[Dpram_Chan] != TempCheck)							// If VI mode changed
    {
    	FilterMode[Dpram_Chan] = TempCheck;							// Update variable
    	if(TempCheck==1)											// If user just switched into current mode then
    	{															// put a 1kHz IIR filter cutoff, but only if
    		if(UserSampleRate > 2500)								// the sample rate is above 2.5kHz
    		{
    			pHostRegs->breakFreq[Dpram_Chan] = 1000;			// Set frequency
    			pHostRegs->fifoBufferCtrl[Dpram_Chan].bits.data_type = 1;// Set bit to enable storing IIR filter
    		}
    	}
    	else														// If use just switched into voltage mode, remove IIR filter
    	{
    		pHostRegs->breakFreq[Dpram_Chan] = 0;					// Set IIR filter frequency to 0 to disable
    		pHostRegs->fifoBufferCtrl[Dpram_Chan].bits.data_type = 0;	// Clear bit to disable storing IIR filter
    	}
    }

    // Check break frequency that user set
    readBreakFreq = pHostRegs->breakFreq[Dpram_Chan];

    // Compare against original value and set flag if it has changed
    if (ActiveBreakFreq[Dpram_Chan]!=readBreakFreq)
    {
        ActiveBreakFreq[Dpram_Chan] = readBreakFreq;                // Update break frequency

        if(readBreakFreq == 0)                                      // If break frequency is zero than disable filter
            filterEnableCTRL = filterEnableCTRL & ~(1<<Dpram_Chan); // clear bit to disable
        else
        {
            filterEnableCTRL = filterEnableCTRL | (1<<Dpram_Chan);  // set bit to enable
            NewBreakFreq_flg[Dpram_Chan] = TRUE;                    // Set flag to recalculate
        }

        pHpsRegs->filter_enable = filterEnableCTRL;                 // Write enable bit
    }

    // Check if sampling rate has changed
    if (NewSampRate_flg==TRUE)                                      // This will detect a change even if it happens in the middle of
    {                                                               //  cycling through channels below
        NewSampRate_flg = FALSE;                                    // Clear flag to acknowledge change
        CycleThroughChans = TRUE;                                   // set flag to cycle through channels below
        ChannelCount = 0;                                           // Reset channel count so it can go through all channels
    }

    // If flag is set, recalculate coefficients (flag may be set by sample rate reconfiguring)
    if(NewBreakFreq_flg[Dpram_Chan]==TRUE || CycleThroughChans==TRUE)
    {
        RecalculateFilterCoeff(Dpram_Chan, readBreakFreq);          // Recalculate coefficients and write them to FPGA
        NewBreakFreq_flg[Dpram_Chan]=FALSE;                         // Reset flag

        if(CycleThroughChans==TRUE)                                 // If sample rate has changed than we need to recompute all channels
        {
            ChannelCount++;                                         // Increment channel counter
            if(ChannelCount==NUM_CHANNELS-1)                        // Check to see if we went through all the channels
                CycleThroughChans=FALSE;                            // Clear flag
        }
    }

    // Check channel number
    if (Dpram_Chan==(NUM_CHANNELS-1))
    {
        Dpram_Chan = 0;
        Dpram_Func_Index++;
    }
    else
        Dpram_Chan = Dpram_Chan + 1;
}


/*
 *  This will calculate the 5 coefficients for the two pole IIR LPF.
 *  It will use the current sampling frequency as well as the users cutoff
 *  frequency.
 */
void RecalculateFilterCoeff(uint32_t chan, uint32_t BreakFreq)
{
    /*
     *         (b0)z^2 + (b1)z + (b2)
     * H(z) = -------------------------
     *         (a0)z^2 + (a1)z + (a2)
     *
     * wp = =TAN(2*PI*Fc/(2*Fs)) = TAN(PI*(Fc/Fs))
     * b0 = wp^2
     * b1 = 2*wp^2
     * b2 = wp^2 = b0
     * a0 = 1 + sqrt(2)*wp + wp^2
     * a1 = 2*(wp^2)-2
     * a2 = 1 - sqrt(2)*wp + wp^2
     *
     * Then normalize everyone by a0 to make a0=1
     */
    float wp = tan(PI*((float)BreakFreq)/((float)UserSampleRate)); // Get normalized frequency
    float b0 = wp*wp;                                   // get pre-normalized b0
    float a0 = 1 + SQRT2*wp + wp*wp;                    // Calculate a0 for normalizing
    b0 = b0/a0;                                         // Normalize (this is also b2)
    float b1 = 2*b0;                                    // Calculate b1 from normalized b0
    float a1 = -(2*wp*wp-2)/a0;                         // Calculate and normalize
    float a2 = -(1-SQRT2*wp+wp*wp)/a0;                  // Calculate and normalize

    // Write floating point coefficients to FPGA
    pHpsRegs->filter_coeff[chan].b2 = b0;               // b0=b2
    pHpsRegs->filter_coeff[chan].b1 = b1;               //
    pHpsRegs->filter_coeff[chan].b0 = b0;               //
    pHpsRegs->filter_coeff[chan].a2 = a2;               //
    pHpsRegs->filter_coeff[chan].a1 = a1;               //

    return;

}

/*
 *  Check filter enable in fifo controls
 */
void CheckIIRUsage(void)
{
	static u32 tmpFifoBuffCtrl[NUM_CHANNELS] = {0};
	u32 tmpRead = 0;

	// Get Register value
	tmpRead = pHostRegs->fifoBufferCtrl[Dpram_Chan].bits.data_type;

	if(tmpRead != tmpFifoBuffCtrl[Dpram_Chan])
	{
		tmpFifoBuffCtrl[Dpram_Chan] = tmpRead;
		u32 tmpReg = pHpsRegs->TurnOnIIR;

		if(tmpRead == 1)
			pHpsRegs->TurnOnIIR = tmpReg |  (1 << Dpram_Chan); 	// Set bit
		else
			pHpsRegs->TurnOnIIR = tmpReg & !(1 << Dpram_Chan); // Clear bit

	}


    // Check channel number
    if (Dpram_Chan==(NUM_CHANNELS-1))
    {
        Dpram_Chan = 0;
        Dpram_Func_Index++;
    }
    else
        Dpram_Chan = Dpram_Chan + 1;


}


/*
 *  This will check the users setting for the A/D range,
 *  change them depending on the module and then write them
 *  back to the FPGA.
 */
void CheckUserRange(void)
{

	// First make sure both D0 and D3 are not running
	u32 tmpTestMode;
	u32 tmpRangeHpsChk;

	tmpTestMode = pCommon->test_enable;
	if( !( (tmpTestMode&D0_EN_BIT) | (tmpTestMode&D3_EN_BIT) ) )
	{
		u32 tmpRangeUser, tmpRangeHps, tmpUnNormRange;
		tmpRangeUser = pHostRegs->ad_polrange[Dpram_Chan];
		tmpUnNormRange = pHostRegs->ad_polrange[Dpram_Chan];

		if(allow_currMode)
		{
			if( (pHostRegs->iv_mode) & (1<<Dpram_Chan) )
				tmpRangeUser = tmpRangeUser + ImodeNormalizer;
			else
				tmpRangeUser = tmpRangeUser + RangeNormalizer;
		}
		else
			tmpRangeUser = tmpRangeUser + RangeNormalizer;

		if ((tmpRangeUser & RANGE_MASK)>MAXRANGE)
			tmpRangeUser = MAXRANGE;


		tmpRangeHpsChk = (tmpRangeUser & POLARITY_MASK) | ADRANGE_SEL[tmpRangeUser & RANGE_MASK]; // This is looking up the hardware range and ORing in the polarity bit
		tmpRangeHps  = pHpsRegs->ad_range[Dpram_Chan].reg;

		// See if Range has changed and if it's different than what is written in the FPGA
		if( (CurrentRange[Dpram_Chan] != tmpRangeUser) || (tmpRangeHps != tmpRangeHpsChk) )
		{
			// Update CurrentRange
			CurrentRange[Dpram_Chan] = tmpRangeUser;

			// Mask out range bits
			tmpUnNormRange = tmpUnNormRange & RANGE_MASK;

			// Call function to set the signal range
			SetSigRange(Dpram_Chan, tmpUnNormRange);
		}
	}

    // Check channel number
    if (Dpram_Chan==(NUM_CHANNELS-1))
    {
        Dpram_Chan = 0;
        Dpram_Func_Index++;
    }
    else
        Dpram_Chan = Dpram_Chan + 1;

}

void SetSigRange(u32 Channel, u32 PolAndRange)
{
    u32 tmpRange;

    // Read Setting for current channel
    tmpRange = PolAndRange & RANGE_MASK;		// Mask to get range only

    // Saturate Range setting to Range 2
    //  There are 4 physical ranges (0, 1, 2, 3).
    //  We start at physical range 1 so the hardware uses the full range of the
    //  A/D while keeping a large common range on the input.
    //  This limit is because we are starting the module at "Physical Range 1"
    //  which is the "User's Range 0".  Therefore the "User Range" is 0, 1, and 2 only.
    //

	// Normalize Range
	if(allow_currMode)
	{
		if( (pHostRegs->iv_mode) & (1<<Channel) )
		{
			pHpsRegs->adMultiplier[Channel] = AdMultiplierArray[1][tmpRange];
			tmpRange = tmpRange + ImodeNormalizer;
		}

		else
		{
			pHpsRegs->adMultiplier[Channel] = AdMultiplierArray[0][tmpRange];
			tmpRange = tmpRange + RangeNormalizer;
		}
	}
	else
	{
		pHpsRegs->adMultiplier[Channel] = AdMultiplierArray[0][tmpRange];
		tmpRange = tmpRange + RangeNormalizer;
	}

	if (tmpRange>MAXRANGE)
		tmpRange = MAXRANGE;


	// Get Polarity
	uint32_t tmpPol = CurrentRange[Channel] & POLARITY_MASK;

	// Write value to FPGA
	pHpsRegs->ad_range[Channel].reg = (tmpPol | ADRANGE_SEL[tmpRange]);


}

/*
 *  This will check the users desired FIFO size and write it to
 *   the FPGA.  This was functionality was moved to the processor
 *   so that we can initialize the default size.
 */
void CheckFifoSize(void)
{
    static uint32_t FifoSize[NUM_CHANNELS] = {0};
    uint32_t tmpFifoSize;

    // Read setting from DPRAM
    tmpFifoSize = pHostRegs->fifoBufferSize[Dpram_Chan];

    // See if size has changed
    if(FifoSize[Dpram_Chan] != tmpFifoSize)
    {
        // Update static variable
        FifoSize[Dpram_Chan] = tmpFifoSize;

        // Write new value back to FPGA
        pHpsRegs->fifoBufferSize[Dpram_Chan] = tmpFifoSize;
    }

    // Check channel number
    if (Dpram_Chan==(NUM_CHANNELS-1))
    {
        Dpram_Chan = 0;
        Dpram_Func_Index++;
    }
    else
        Dpram_Chan = Dpram_Chan + 1;
}


/*
 *  This is used to change the range normalizer if channel is changed to current mode.
 *   Module needs to be an AD4 in order to do current measurements
 */
void CheckVImode(void)
{

	// See if mode has changed
	uint32_t tempMode = pHostRegs->iv_mode;
	if(tempMode != ChanMode)
	{
		// Update global variable
		ChanMode = tempMode;

		// Set flag for background cal
		VI_Mode_Changed_flg = TRUE;

		for(uint32_t i=0; i<NUM_CHANNELS; i++)
		{
			// Check range for channel
			u32 tmpRange = pHostRegs->ad_polrange[i];

			// Mask out range bits
			tmpRange = tmpRange & RANGE_MASK;

			// Normalize Range
			if(allow_currMode)
			{
				if( (pHostRegs->iv_mode) & (1<<i) )
				{
					pHpsRegs->adMultiplier[i] = AdMultiplierArray[1][tmpRange];
					tmpRange = tmpRange + ImodeNormalizer;
				}
				else
				{
					pHpsRegs->adMultiplier[i] = AdMultiplierArray[0][tmpRange];
					tmpRange = tmpRange + RangeNormalizer;
				}
			}
			else
			{
				pHpsRegs->adMultiplier[i] = AdMultiplierArray[0][tmpRange];
				tmpRange = tmpRange + RangeNormalizer;
			}

			if (tmpRange>MAXRANGE)
				tmpRange = MAXRANGE;

			// Write new range
			uint32_t tmpPol = CurrentRange[i] & POLARITY_MASK;

			// Write value to FPGA
			pHpsRegs->ad_range[i].reg = (tmpPol | ADRANGE_SEL[tmpRange]);

		}
	}

	// Go to next function
    Dpram_Func_Index++;

}

/*
 * This is used to check the averaging time for the external PLL measurement.
 *  It will also read in the External PLL frequency measurement and write it out.
 */
void CheckExternalPLL(void)
{

	u32 tmpTime;
	tmpTime = pHostRegs->PllFreqMeasTime;

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

	// Check VADC Lock
	u32 tmpLock;
	tmpLock = pHostRegs->PllVadcLock;

	if(tmpLock != PLLVadcLockVal)
	{
		PLLVadcLockVal = tmpLock;
		if(tmpLock==0)
			pHpsRegs->starti2c_rdwr = 0x0004; // UnLock
		else
			pHpsRegs->starti2c_rdwr = 0x1004; // Lock

		usleep(300);
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


/*********************************************************************************/
void AverageSamples(void)
{
    uint32_t mode, calChan, rangeToCheck, rangeNormalized;
    bool CalOffset = FALSE;
    bool CalGain = FALSE;
    bool CalDacOffset = FALSE;
    bool CalDacGain = FALSE;
    bool CalSigDacOff = FALSE;
    bool CalSigDacGain = FALSE;

    // Disable A/D Sig and Bit latch synchronization
    pHpsRegs->EnSyncAdLatch = 0x00;							//b(0):Enable synchronization, b(4):Start state machine

    // Check averaging enable register
    mode = pHostRegs->cal_mode;

    // Get channel number
    calChan = pHostRegs->cal_chan;

    // Connect Bit A/D to channel
    pHpsRegs->check_en =  check_en_array[calChan];      // Enable proper mux
    pHpsRegs->check_sel= check_sel_array[calChan];      // Select mux channel

    // Read gain for current channel
    rangeToCheck = pHostRegs->ad_polrange[calChan];     // Read register
    rangeToCheck = rangeToCheck & 0x00000003;           // mask out range

    bool CurrentModeChan = FALSE;
    if(allow_currMode)
    {
    	if( (pHostRegs->iv_mode) & (1<<calChan) )
    	{
    		CurrentModeChan = TRUE;
    		pHpsRegs->adMultiplier[calChan] = AdMultiplierArray[1][rangeToCheck];
    		rangeNormalized = rangeToCheck + ImodeNormalizer;
    	}
		else
		{
			pHpsRegs->adMultiplier[calChan] = AdMultiplierArray[0][rangeToCheck];
			rangeNormalized = rangeToCheck + RangeNormalizer;
		}
    }
    else
    {
    	pHpsRegs->adMultiplier[calChan] = AdMultiplierArray[0][rangeToCheck];
    	rangeNormalized = rangeToCheck + RangeNormalizer;
    }

    if (rangeNormalized>MAXRANGE)
    	rangeNormalized = MAXRANGE;


    // Get Polarity
    //uint32_t tmpPol = CurrentRange[calChan] & 0x10;
    uint32_t tmpPol = 0x10;

    for(u16 tmpind = 0; tmpind<16; tmpind++)
    	pHpsRegs->ad_range[tmpind].reg = (ADRANGE_SEL[rangeNormalized] | tmpPol);  // Gain for SIG A/D

    // Only calibrate one mode at a time
    switch(mode)
    {
        case 1 :                    // Cal signal and Bit offset
        case 5 :					// Cal current mode signal offset
            CalOffset = TRUE;
            break;
        case 2 :                    // Cal signal and Bit gain
        case 6 :					// Cal current mode gain
            CalGain = TRUE;
            break;
        case 3 :                    // Cal DAC offset
            CalDacOffset = TRUE;
            break;
        case 4 :                    // Cal DAC gain
            CalDacGain = TRUE;
            break;
        case 7 :
        	CalSigDacOff = TRUE;
        	break;
        case 8 :
        	CalSigDacGain = TRUE;
        	break;
        default:
            break;
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// SIG/BIT Voltage/Current offset or GAIN
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if( (CalOffset==TRUE) || (CalGain==TRUE) )
    {
    	// Make sure the readings are from the user A/D (this was causing a problem when going into Current Calibration)
    	pHpsRegs->connect_dac=0x0000;

		// Select Bit mux
		if(calChan<8)
			pHpsRegs->bit_sel = 0;                          // Connect (channel 1-8)
		else
			pHpsRegs->bit_sel = 1;                          // Connect (channel 9-16)

		// Write Cal channel for BIT
		pHpsRegs->bit_chan = calChan;

		// Set Gain (set by PGA range)
		pHpsRegs->bit_gain = ADRANGE_SEL[rangeNormalized];               // Gain for BIT A/D
		pHpsRegs->DacRange = rangeToCheck;

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// SIG/BIT Voltage/Current offset
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if( CalOffset==TRUE )
		{
			// Check if in voltage or current mode
			if(!CurrentModeChan)
			{
				pCalRegs->ad_offset[calChan][rangeToCheck]  = pHostRegs->sig_cal_val;
				pCalRegs->bit_offset[calChan][rangeToCheck] = pHostRegs->bit_cal_val;
			}
			else
			{
				pCalRegs->Imode_offset[calChan][rangeToCheck] = pHostRegs->sig_cal_val;
				pCalRegs->Imode_bit_offset[calChan][rangeToCheck] = pHostRegs->bit_cal_val;
			}
		}

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// SIG/BIT Voltage/Current Gain
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if( CalGain == TRUE )
		{
			// Check if in voltage or current mode
			if(!CurrentModeChan)
			{
				pCalRegs->ad_gain[calChan][rangeToCheck]  = pHostRegs->sig_cal_val;
				pCalRegs->bit_gainCal[calChan][rangeToCheck] = pHostRegs->bit_cal_val;
			}
			else
			{
				pCalRegs->Imode_gain[calChan][rangeToCheck]  = pHostRegs->sig_cal_val;
				pCalRegs->Imode_bit_gainCal[calChan][rangeToCheck] = pHostRegs->bit_cal_val;
			}
		}
    }

    if( (CalDacOffset==TRUE) || (CalDacGain==TRUE) )
    {

    	// Place DAC onto BIT signal
    	pHpsRegs->bit_sel=0x2;

    	// Make sure BIT channel is updated so it uses the correct multiplier gain in the FPGA
    	pHpsRegs->bit_chan = calChan;

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Write CAL Value
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Write Range and Gain from User
		// b5:Enable, b4:Polarity, b[3..0]:Range
		u32 tmpRange;
		u32 Range = pCommon->test_range; 		 // Get Polarity from user (range is no longer used)
		tmpRange = Range & 0x00000003;           // Just pull out the user's range

		if( CalDacOffset == TRUE)
		{
			pCalRegs->bit_DacCal[0][tmpRange] = pHostRegs->bit_cal_val;  // Write to offset cal value
		}
		else
		{
			pCalRegs->bit_DacCal[1][tmpRange] = pHostRegs->bit_cal_val;  // Write to offset cal value
		}

		//tmpRange = Range | 0x00000030;           // Fix range to 0 (b3..0) and write the enable bit (b5) - Now fixing to bipolar
		pHpsRegs->DacRange = tmpRange;
		tmpRange = rangeNormalized | 0x00000034;           // Fix range to 0 (b3..0) and write the enable bit (b5) - Now fixing to bipolar
		pHpsRegs->bit_gain = tmpRange;           // Write range setting

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		// Write DAC value from User
		pHpsRegs->dac_data = ( (pCommon->test_value)>>RangeNormalizer );

    }


    if( (CalSigDacOff==TRUE) || (CalSigDacGain==TRUE) )
    {
    	// Place DAC onto BIT signal
    	pHpsRegs->bit_sel=0x2;

    	// Make sure BIT channel is updated so it uses the correct multiplier gain in the FPGA
    	pHpsRegs->bit_chan = calChan;

    	// Now place BIT signal into A/D Channels (this will connect DAC to those channels)
    	//pHpsRegs->connect_dac=0xFFFF;
    	pHpsRegs->connect_dac = (1 << calChan);

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Write CAL Value
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Write Range and Gain from User
		// b5:Enable, b4:Polarity, b[3..0]:Range
		u32 tmpRange;
		u32 Range = pCommon->test_range; 		 // Get Polarity from user (range is no longer used)
		tmpRange = Range & RANGE_MASK;           // Just pull out the user's range

		if( CalSigDacOff == TRUE)
		{
			pCalRegs->sig_DacCal[0][calChan]  = pHostRegs->sig_cal_val;
		}
		else
		{
			pCalRegs->sig_DacCal[1][calChan]  = pHostRegs->sig_cal_val;
		}

		//tmpRange = Range | 0x00000030;           // Fix range to 0 (b3..0) and write the enable bit (b5) - Now fixing to bipolar
		pHpsRegs->DacRange = tmpRange;
		tmpRange = 0x00000034;           // Fix range to 0 (b3..0) and write the enable bit (b5) - Now fixing to bipolar
		pHpsRegs->bit_gain = tmpRange;           // Write range setting

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		// Write DAC value from User
		pHpsRegs->dac_data = ( (pCommon->test_value));//>>1); // Divide by 2 when DAC signal is connected to signal
    }

    // Write signal A/D average to DPRAM
    pHostRegs->sig_avg_val = pHpsRegs->ad_reading[calChan];

    // Write Bit A/D average to DPRAM
    pHostRegs->bit_avg_val = pHpsRegs->bit_reading;

}

