/*
 * module_BkgndCal.c
 *
 *  Created on: Dec 11, 2014
 *      Author: randolina
 */

#include <includes.h>

//==============================================
// Background Calibration Local Functions
//==============================================
void BkgndCalWithBit(void);
void GetNextActiveChan(void);
void SetBitRange(void);
void ConnectDACtoBit(void);
void BkgndCalBITOffs(void);
void SetDacVoltage(void);
void BkgndCalBITGain(void);
void ConnectChanToBIT(void);
void ConnectDACtoSig(void);
void BkgndCalSigOffs(void);
//void SetDacVoltage(void);
void BkgndCalSigGain(void);
void BkgndCalCleanup(void);
void AverageForD2(void);
void EndOfBkgrndCalFunctions(void);

//==============================================
// External Functions
//==============================================
extern void init_dac(void);


//==============================================
// Background Calibration Local Variables
//==============================================
static u32 BKCALrangeToCheck;
static u32 BKCALrangeNormalized;
static u32 BKCALGainSel;
static u32 Bkgrnd_Func_Index = 0; // Holds index for function pointer
static u32 bkgndChan = 0;	// Holds current channel that is going through background calibration
static u32 ExpectedDAC = 0; // Holds expected value when reading DAC data
static s32 AccBit, AccSig;
static s32 AccBitReadings;
static u16 BkndAvgCnt = (1<<AMOUNTOFSAMPLES);
static u8 ResetBkgndCal = 0;
static float fpRatioAcc = 0;
static u16 SampleCntUsed = 0;

// Range multiplexor selects
static u32 ADRANGE_SEL[6] = {0x4, 0x5, 0x0, 0x1, 0x2, 0x3}; // (10V, 5V, 2.5V, 1.25V, 0.625V, 0.3V) ranges

static u32 REVSERANGE_SELV[6] = {0x2, 0x3, 0x4, 0x5, 0x0, 0x1}; // This is to reverse the lookup of ADRANGE_SEL.
//static u32 REVSERANGE_SELI[6] = {0x0, 0x0, 0x0, 0x1, 0x0, 0x0}; // This is to reverse the lookup of ADRANGE_SEL.

//==============================================
// Background Calibration External Variables
//==============================================
extern HOST_REGS  *pHostRegs;
extern HPS_REGS   *pHpsRegs;
extern CAL_REGS   *pCalRegs;

extern u32 check_sel_array[NUM_CHANNELS];
extern u32 check_en_array[NUM_CHANNELS];

extern u32 TestMode;
extern u32 RangeNormalizer;
extern u32 allow_currMode;
extern u8 MAXRANGE;

extern bool VI_Mode_Changed_flg;

/*
 *  This will hold the list of background calibration functions
 */
// These Background calibration routines were stripped down and we are now only calibrating the BIT a/d, then looking for the amount of error between the BIT a/d readings and SIGNAL a/d reading.
void (*BkgrndCalFuncPtr[])(void) =
{
		GetNextActiveChan,		// Gets the next active channel
		SetBitRange,
		ConnectDACtoBit, 		// Input DAC to BIT A/D (set range and put voltage at 0)
//ra		BkgndCalBITOffs,		// Calibrate any error in BIT A/D offset
		SetDacVoltage,			// Sets voltage for DAC signal
		BkgndCalBITGain,		// Calibrate any error in BIT A/D gain
		ConnectChanToBIT, 		// Connect bit to next active channel
//		ConnectDACtoSig,		// Input DAC to SIG A/D (set range and put voltage at 0)
//ra		BkgndCalSigOffs, 		// Average and compare both the Sig and BIT A/D Readings -> Adjust Calibration values for Sig A/D (gain only)
//		SetDacVoltage,			// Sets voltage for DAC signal
//		BkgndCalSigGain,		// Calibrate any error in SIG A/D gain
		BkgndCalCleanup,		// Disconnect DAC and put signal back. Also connect input signal to BIT A/D
		AverageForD2,			// Take an average of the ratio Sig/Bit and see if it's within error threshold
		EndOfBkgrndCalFunctions // End of function list
};

// Removing background calibration for now so need to preserve the needed functions for the D2 test
//void (*BkgrndCalFuncPtr[])(void) =
//{
//		GetNextActiveChan,		// Gets the next active channel
//		ConnectDACtoBit, 		// Input DAC to BIT A/D (set range and put voltage at 0)
//ra		BkgndCalBITOffs,		// Calibrate any error in BIT A/D offset
//		SetDacVoltage,			// Sets voltage for DAC signal
//		BkgndCalBITGain,		// Calibrate any error in BIT A/D gain
//		ConnectChanToBIT, 		// Connect bit to next active channel
//		ConnectDACtoSig,		// Input DAC to SIG A/D (set range and put voltage at 0)
//ra		BkgndCalSigOffs, 		// Average and compare both the Sig and BIT A/D Readings -> Adjust Calibration values for Sig A/D (gain only)
//		SetDacVoltage,			// Sets voltage for DAC signal
//		BkgndCalSigGain,		// Calibrate any error in SIG A/D gain
//		BkgndCalCleanup,		// Disconnect DAC and put signal back. Also connect input signal to BIT A/D
//		AverageForD2,			// Take an average of the ratio Sig/Bit and see if it's within error threshold
//		EndOfBkgrndCalFunctions // End of function list
//};



//==============================================================
//============ BACKGROUND CALIBRATION ROUTINES =================
//==============================================================
void BkgndCalWithBit(void)
{
	static u32 RangeCheck[NUM_CHANNELS] = {0};

	// See if Background calibration is enabled
	if(pHostRegs->TroubleshootEn & 0x1)
		ResetBkgndCal = 1;								// Reset Background Cal

	// Check if test modes are Enabled
	u32 tmpTestMode = TestMode;
	if ( (tmpTestMode&D3_EN_BIT) || (tmpTestMode&D0_EN_BIT) )
		ResetBkgndCal = 1;								// Reset Background Cal

	// Check if calibrating
	if(pHostRegs->cal_en)
		ResetBkgndCal = 1;								// Reset Background Cal

	// See if current channel's range was changed
	u32 tmpRange = pHostRegs->ad_polrange[bkgndChan];   // Read register
	tmpRange = tmpRange & 0x3;           		  		// mask out range
	if(tmpRange != RangeCheck[bkgndChan])				// Compare to previous range
	{
		ResetBkgndCal = 1;								// Reset Background Cal
		RangeCheck[bkgndChan] = tmpRange;				// Update previous range
	}

	//------------------------------------------------------
	// Make sure HPS and Host have same range setting
	u32 tmpHpsRange = pHpsRegs->ad_range[bkgndChan].reg;
	tmpHpsRange &= RANGE_MASK;

	if( (pHostRegs->iv_mode) & (1<<bkgndChan) )
		tmpHpsRange = tmpHpsRange;//CURRENT MODE FOLLOWS THE SAME RANGE FOR HPS AND HOST       REVSERANGE_SELI[tmpHpsRange];

	else
		tmpHpsRange = REVSERANGE_SELV[tmpHpsRange];

	if(tmpHpsRange != tmpRange)
		ResetBkgndCal = 1;								// Reset Background Cal
	//------------------------------------------------------

	// See if module's power was turned off
	if(pCommon->ps_enable == 0)
		ResetBkgndCal = 1;								// Reset Background Cal

	// See if current channel was disabled while running
	u32 tmpChkChan = (1 << bkgndChan);
	if( !(pHostRegs->activeChan & tmpChkChan) )
		ResetBkgndCal = 1;

	// If mode has change
	if (VI_Mode_Changed_flg == TRUE)
	{
		VI_Mode_Changed_flg = FALSE;
		ResetBkgndCal = 1;
	}

	if(ResetBkgndCal == 1)
	{
		// Make sure BIT Readings are not being display in Sig A/D Readings
		pHpsRegs->bit_enable = 0;

		// Make sure DAC is not connected to signal if D0 or D3 isn't enabled
		if(!(pCommon->test_enable & 0x9))
			pHpsRegs->connect_dac = 0;

		ResetBkgndCal = 0;								// Clear variable
		Bkgrnd_Func_Index = 0;  						// Reset function pointer

		// Reset average accumulators and counters
		AccBit = 0;	AccSig = 0;
		AccBitReadings = 0;
		BkndAvgCnt = 1<<AMOUNTOFSAMPLES;

		fpRatioAcc 	  = 0;	// Reset Accumulator
		SampleCntUsed = 0;	// Reset Samples Used

		// Disable A/D Sig and Bit latch synchronization
		pHpsRegs->EnSyncAdLatch = 0x00;							//b(0):Enable synchronization, b(4):Start state machine

		u32 UserBitChan = pHostRegs->ManualBitChan;
		pHpsRegs->bit_chan = UserBitChan; 						// Write user's channel for BIT

		// Connect Bit A/D to channel
		pHpsRegs->check_en =  check_en_array[UserBitChan];      // Enable proper mux
		pHpsRegs->check_sel= check_sel_array[UserBitChan];     	// Select mux channel

		// Select Bit mux
		if(UserBitChan<8)
			pHpsRegs->bit_sel = 0;                          	// Connect (channel 1-8)
		else
			pHpsRegs->bit_sel = 1;                         	 	// Connect (channel 9-16)

		// Give user the BIT reading
		pHostRegs->BitADReading = pHpsRegs->bit_reading;

	}

	// If everything passes, perform background calibration
	BkgrndCalFuncPtr[Bkgrnd_Func_Index]();
}

/*
 *  Sets channel variable to the next active channel
 */
void GetNextActiveChan(void)
{
	static u16 ChannelToCheck = 0;

	//Disable A/D sig and bit latch synchronization
	pHpsRegs->EnSyncAdLatch = 0x00;					//b(0):Enable synchronization, b(4):Start state machine

	// Sort through active channels and get the next one
	u32 ActiveChans = pHostRegs->activeChan;
	u32 testChanAct = 0;
	u32 tmpVar;

	if(ActiveChans>0)
	{
		while(testChanAct==0)
		{
			tmpVar = ActiveChans>>ChannelToCheck; 	// Shift channel down to test it next
			testChanAct = tmpVar & 0x0001;			// Test to see if channel is active
			bkgndChan = ChannelToCheck; 			// Set channel to check for background calibration
			ChannelToCheck++;						// Increment channel number for the next time we come back to this function
			if(ChannelToCheck>15)
				ChannelToCheck = 0;
		}
	}

	// Go to next function
	Bkgrnd_Func_Index++;
}

/*
 * Sets range for BIT A/D
 */
void SetBitRange(void)
{
	// Read range for current channel
	BKCALrangeToCheck = pHostRegs->ad_polrange[bkgndChan];     	// Read register
	BKCALrangeToCheck = BKCALrangeToCheck & RANGE_MASK;           		// mask out range

	if(allow_currMode)
	{
		if( (pHostRegs->iv_mode) & (1<<bkgndChan) )
			BKCALrangeNormalized = BKCALrangeToCheck + ImodeNormalizer;
	else
			BKCALrangeNormalized = BKCALrangeToCheck + RangeNormalizer;
	}
	else
		BKCALrangeNormalized = BKCALrangeToCheck + RangeNormalizer;

	if (BKCALrangeNormalized>MAXRANGE)							// Saturate to the maximum range
		BKCALrangeNormalized = MAXRANGE;

	BKCALGainSel = BKCALrangeNormalized;					// Keep normalized gain setting to adjust the DAC word later on.

	BKCALrangeNormalized = 0x10 | ADRANGE_SEL[BKCALrangeNormalized];								// Make DAC data bipolar

	pHpsRegs->bit_gain = BKCALrangeNormalized;					// Sets range in hardware
	pHpsRegs->DacRange = BKCALrangeToCheck;						// Used to point to DAC cal data
	pHpsRegs->bit_chan = bkgndChan;               			// This will point to BIT cal data

	Bkgrnd_Func_Index++;

}

/*
 * This will get the range of the active channel and setup the bit for the same range.
 *  It will then setup the DAC to drive at close to full scale and connect it to the BIT A/D.
 */
void ConnectDACtoBit(void)
{
	// Read range for current channel
/*	BKCALrangeToCheck = pHostRegs->ad_polrange[bkgndChan];     	// Read register
	BKCALrangeToCheck = BKCALrangeToCheck & 0x3;           		// mask out range

	if(allow_currMode)
	{
		if( (pHostRegs->iv_mode) & (1<<bkgndChan) )
			BKCALrangeNormalized = BKCALrangeToCheck + ImodeNormalizer;
		else
			BKCALrangeNormalized = BKCALrangeToCheck + RangeNormalizer;
	}
	else
		BKCALrangeNormalized = BKCALrangeToCheck + RangeNormalizer;

	if (BKCALrangeNormalized>MAXRANGE)							// Saturate to the maximum range
		BKCALrangeNormalized = MAXRANGE;
	BKCALrangeNormalized |= 0x10;								// Make DAC data bipolar

	pHpsRegs->bit_gain = BKCALrangeNormalized;					// Sets range in hardware
	pHpsRegs->DacRange = BKCALrangeToCheck;						// Used to point to DAC cal data
	pHpsRegs->bit_chan = bkgndChan;               			// This will point to BIT cal data
*/
	// Place DAC onto BIT signal
	pHpsRegs->bit_sel=0x2;

	// Indicate that no signal channels are connected to BIT A/D
	pCommon->in_calibration= 0;

	// Set DAC voltage to Zero
//ra	pHpsRegs->dac_data = 0;
//ra	ExpectedDAC = 0;

	// Removing below sleep because we are no longer calibrating the Offset
	// Put in wait for switches to settle
	//ra usleep(1000);
	//usleep(500000);

	// Go to next function
	Bkgrnd_Func_Index++;
}

/*
 * With DAC connected to BIT, calibrate the offset
 */
void BkgndCalBITOffs(void)
{
	// Read Bit A/D Value
	union {
			s16 half[2];
			s32 full;
		  } BitADReading;
	s32 AveragedReading;

	if(BkndAvgCnt == 0)
	{
		AveragedReading = AccBitReadings>>AMOUNTOFSAMPLES; 		// Get averaged accumulator value
		BkndAvgCnt = 1 << AMOUNTOFSAMPLES; 						// Reset Counter
		AccBitReadings = 0;					 					// Reset accumulator

		// See if reading is within tolerance
		if(abs(AveragedReading-ExpectedDAC)>2)
		{
			if( abs(AveragedReading)<100 ) // Make sure offset is reasonable
			{
				// Out of tolerance so keep calibrating

				// Get current offset calibration value
				s32 CurrentCalVal = pCalRegs->bit_offset[bkgndChan][BKCALrangeToCheck]; //chan range

				// Calculate new cal value
				s32 NewCalVal_s32 = CurrentCalVal - AveragedReading ;

				// Check Cal Value range before writing
				if( (NewCalVal_s32>0xFFFF000) && (NewCalVal_s32<0x1000))
				{
					pCalRegs->bit_offset[bkgndChan][BKCALrangeToCheck] = NewCalVal_s32;
					printf("\nCalibrated DAC Offs: %ld with CalVal: 0x%08lX\n", bkgndChan, NewCalVal_s32);
					usleep(10000);
				}
			}
		}
		else
		{
			// Within tolerance so continue
			Bkgrnd_Func_Index++;
		}
	}
	else
	{
		BitADReading.full = pHpsRegs->bit_reading;  // Get Bit A/D Reading
		AccBitReadings += BitADReading.half[0]; 	// Accumulate
		BkndAvgCnt--; 								// Decrement count
	}

}

/*
 * Set DAC voltage for calibrating
 */
void SetDacVoltage(void)
{
	// Set DAC Voltage
	//switch (BKCALrangeToCheck & RANGE_MASK)
	switch (BKCALGainSel)
	{
		case 0 : ExpectedDAC=0x00007000; break;
		case 1 : ExpectedDAC=0x00003800; break;
		case 2 : ExpectedDAC=0x00001C00; break;
		case 3 : ExpectedDAC=0x00000E00; break;
		case 4 : ExpectedDAC=0x00000700; break;
		case 5 : ExpectedDAC=0x00000380; break;
		default: ExpectedDAC=0x00000380; break;
	}

	// Scale ExpectedDAC based on Gain Multiplier
	u32 DACWord = (ExpectedDAC >> pHpsRegs->adMultiplier[bkgndChan]);

	// Write this to DAC data
	pHpsRegs->dac_data = DACWord;

	// Scale ExpectedDAC based on RANGE
	//ExpectedDAC = ExpectedDAC<<(BKCALrangeToCheck & RANGE_MASK);
	ExpectedDAC = ExpectedDAC<<(BKCALGainSel);

	// Give time for D/A output to settle
	usleep(1000);
	//usleep(500000);

	// Go to next function
	Bkgrnd_Func_Index++;

}

/*
 *  Calibrate BIT gain
 */
void BkgndCalBITGain(void)
{
	// Read Bit A/D Value
	union {
			s16 half[2];
			s32 full;
		  } BitADReading;
	s32 AveragedReading;

	if(BkndAvgCnt == 0)
	{
		AveragedReading = AccBitReadings>>AMOUNTOFSAMPLES; 		// Get averaged accumulator value
		BkndAvgCnt = 1 << AMOUNTOFSAMPLES; 						// Reset Counter
		AccBitReadings = 0;					 					// Reset accumulator

		// See if reading is within tolerance
		if(abs(AveragedReading-ExpectedDAC)>2)
		{
			float ratio;
			ratio = ((float)(ExpectedDAC)) / ((float)(AveragedReading));
			if( (ratio>0.5) && (ratio<1.5) ) // Make sure ratio is reasonable
			{
				// Out of tolerance so keep calibrating
				u32 NewCalVal_u32, CurrentCalVal;
				float NewCalVal_flt;

				// Get gain scale
				if( (pHostRegs->iv_mode) & (1<<bkgndChan) )  // If in current mode
					CurrentCalVal = pCalRegs->Imode_bit_gainCal[bkgndChan][BKCALrangeToCheck]; //chan range
				else
					CurrentCalVal = pCalRegs->bit_gainCal[bkgndChan][BKCALrangeToCheck]; //chan range


				NewCalVal_flt = (float)CurrentCalVal * ratio;
				NewCalVal_u32 = (u32)(NewCalVal_flt);

				// Check Cal Value range before writing
				if( (NewCalVal_u32>0x2000) && (NewCalVal_u32<0x6000))
				{
					if( (pHostRegs->iv_mode) & (1<<bkgndChan) ) // If in current mode
						pCalRegs->Imode_bit_gainCal[bkgndChan][BKCALrangeToCheck] = NewCalVal_u32;
					else
						pCalRegs->bit_gainCal[bkgndChan][BKCALrangeToCheck] = NewCalVal_u32;

					printf("\nCalibrated BIT Gain: %ld with CalVal: 0x%08lX\n", bkgndChan, NewCalVal_u32);
					usleep(10000);
				}
			}
			else
			{
				init_dac();
				usleep(100000);
			}
		}
		else
		{
			// Within tolerance so continue
			Bkgrnd_Func_Index++;
		}
	}
	else
	{
		BitADReading.full = pHpsRegs->bit_reading;  // Get Bit A/D Reading
		AccBitReadings += BitADReading.half[0]; 	// Accumulate
		BkndAvgCnt--; 								// Decrement count
	}
}

/*
 *  Connect the next channel to the BIT A/D
 */
void ConnectChanToBIT(void)
{
	// First write 0 to DAC to prevent glitch when switching BIT A/D to point to Signal
	pHpsRegs->dac_data = 0;
	ExpectedDAC = 0;
//	usleep(500000);

	// Write which channel is being used for BIT out to user
	pCommon->in_calibration = (bkgndChan+1);

	pHpsRegs->bit_chan = bkgndChan; 						// Write Cal channel for BIT

	// Connect Bit A/D to channel
	pHpsRegs->check_en =  check_en_array[bkgndChan];      	// Enable proper mux
	pHpsRegs->check_sel= check_sel_array[bkgndChan];     	// Select mux channel

	// Select Bit mux
	if(bkgndChan<8)
		pHpsRegs->bit_sel = 0;                          	// Connect (channel 1-8)
	else
		pHpsRegs->bit_sel = 1;                         	 	// Connect (channel 9-16)

	// Wait some time for switching signal and letting it settle
	usleep(10000);
	//usleep(500000);

	// Tell FPGA to start reading BIT A/D for that channel so it seems seamless to user
//	pHpsRegs->bit_enable = (1<<bkgndChan);

	usleep(100);
	//usleep(500000);

	// Go to next function
	Bkgrnd_Func_Index++;
}

/*
 *  Connect the DAC to the signal A/D and set the voltage to 0
 */
void ConnectDACtoSig(void)
{
	// Set DAC voltage to Zero
//	pHpsRegs->dac_data = 0;
//	ExpectedDAC = 0;

	// Connect DAC to specific channel
	pHpsRegs->connect_dac = (1<<bkgndChan);


	// Go to next function
	Bkgrnd_Func_Index++;
}

/*
 * Calibrate signal offset
 */
void BkgndCalSigOffs(void)
{
	// Read Bit A/D Value
	s32 SigADReading;
	s32 AveragedReading;

	if(BkndAvgCnt == 0)
	{
		AveragedReading = AccBitReadings>>AMOUNTOFSAMPLES; 		// Get averaged accumulator value
		BkndAvgCnt = 1 << AMOUNTOFSAMPLES; 						// Reset Counter
		AccBitReadings = 0;					 					// Reset accumulator

		// See if reading is within tolerance
		if(abs(AveragedReading-ExpectedDAC)>2)
		{
			if( abs(AveragedReading)<100 ) // Make sure offset is reasonable
			{
				// Out of tolerance so keep calibrating

				// Get current offset calibration value
				s32 CurrentCalVal = pCalRegs->ad_offset[bkgndChan][BKCALrangeToCheck]; //chan range

				// Calculate new cal value
				s32 NewCalVal_s32 = CurrentCalVal - AveragedReading ;

				// Check Cal Value range before writing
				if( (NewCalVal_s32>0xFFFF000) && (NewCalVal_s32<0x1000))
				{
					pCalRegs->ad_offset[bkgndChan][BKCALrangeToCheck] = NewCalVal_s32;
					printf("\nCalibrated Sig Offs: %ld with CalVal: 0x%08lX\n", bkgndChan, NewCalVal_s32);
					usleep(10000);
				}
			}
		}
		else
		{
			// Within tolerance so continue
			Bkgrnd_Func_Index++;
		}
	}
	else
	{
		SigADReading = pHpsRegs->ad_readForBit; //pHpsRegs->ad_reading[bkgndChan];  // Get Bit A/D Reading
		AccBitReadings += SigADReading; 				 // Accumulate
		BkndAvgCnt--; 								     // Decrement count
	}

}

/*
 *  Calibrate Gain of Sig A/D
 */
void BkgndCalSigGain(void)
{
	// Read Bit A/D Value
	s32 SigADReading;
	s32 AveragedReading;

	if(BkndAvgCnt == 0)
	{
		AveragedReading = AccBitReadings>>AMOUNTOFSAMPLES; 		// Get averaged accumulator value
		BkndAvgCnt = 1 << AMOUNTOFSAMPLES; 						// Reset Counter
		AccBitReadings = 0;					 					// Reset accumulator

		// See if reading is within tolerance
		if(abs(AveragedReading-ExpectedDAC)>2)
		{
			float ratio;
			ratio = ((float)(ExpectedDAC)) / ((float)(AveragedReading));
			if( (ratio>0.5) && (ratio<1.5) ) // Make sure ratio is reasonable
			{
				// Out of tolerance so keep calibrating
				u32 NewCalVal_u32, CurrentCalVal;
				float NewCalVal_flt;

				// Get gain scale
				CurrentCalVal = pCalRegs->ad_gain[bkgndChan][BKCALrangeToCheck]; //chan range
				NewCalVal_flt = (float)CurrentCalVal * ratio;
				NewCalVal_u32 = (u32)(NewCalVal_flt);

				// Check Cal Value range before writing
				if( (NewCalVal_u32>0x2000) && (NewCalVal_u32<0x6000))
				{
					pCalRegs->ad_gain[bkgndChan][BKCALrangeToCheck] = NewCalVal_u32;
					printf("\nCalibrated Sig Gain: %ld with CalVal: 0x%08lX\n", bkgndChan, NewCalVal_u32);
					usleep(10000);
				}
			}
			else
			{
				init_dac();
				usleep(100000);
			}
		}
		else
		{
			// Within tolerance so continue
			Bkgrnd_Func_Index++;
		}
	}
	else
	{
		SigADReading = pHpsRegs->ad_readForBit; // pHpsRegs->ad_reading[bkgndChan];  // Get Bit A/D Reading
		AccBitReadings += SigADReading; 		// Accumulate
		BkndAvgCnt--; 							// Decrement count
	}
}

/*
 *  Disconnect DAC from Sig A/D and tell FPGA to take readings from normal Sig A/D instead of BIT A/D
 */
void BkgndCalCleanup(void)
{
	// Remove DAC from signal A/D
	pHpsRegs->connect_dac = 0;

	// Place Signal onto BIT A/D for D2 testing
//	pHpsRegs->check_en =  check_en_array[bkgndChan];      	// Enable proper mux
//	pHpsRegs->check_sel= check_sel_array[bkgndChan];     	// Select mux channel

	// Select Bit A/D mux
//	if(bkgndChan<8)
//		pHpsRegs->bit_sel = 0;                          	// Connect (channel 1-8)
//	else
//		pHpsRegs->bit_sel = 1;                         	 	// Connect (channel 9-16)

	// Give enough time for A/D to get input switched and sampled
	usleep(500);

	// Tell FPGA to take readings from normal Sig A/D
	pHpsRegs->bit_enable = 0;

	// Start state machine to get a new sample for D2 averaging which will be the next function
	pHpsRegs->EnSyncAdLatch = 0x11;							//b(0):Enable synchronization, b(4):Start state machine

	// Go to next function
	Bkgrnd_Func_Index++;
}

/*
 *  This function will average the ratio of the Signal A/D over the Bit A/D reading.
 *   The ratio will determine if the module is within spec after running the background cal.
 */
void AverageForD2(void)
{
	static u8 DoneSamplingSig=FALSE;
	static u16 D2Count[NUM_CHANNELS] = {0};

	// Check inner loop sample counter
	if(SampleCntUsed == 4096)
		DoneSamplingSig=TRUE;
	else
	{
		SampleCntUsed++;

		// Not enough samples so check latest sample received
		s32 BitReading = pHpsRegs->bit_reading;
		s32 SigReading = pHpsRegs->ad_reading[bkgndChan];
		u32 Difference;

		// @@@ TODO This threshold comparison of the BitReading was put in because when nothing is attached, the open detect circuit is causing bad readings on the A/D which was causing a false BIT to be flagged.
		//  Eventually this will need to be fixed and this threshold comparison should be removed.
		if (abs(BitReading)<0x1200)
			Difference = 0;
		else
			Difference = abs(((s16)BitReading - (s16)SigReading));  // Converting to 16 bits since data that is being read of signal A/D was
																	//  not sign extended when in unipolar mode.  I didn't want to break something when
																	//  changing it so instead, I casted here.

		// Accumulate difference into a float (only because the accumulator was reused from an altnerate algorithm)
		fpRatioAcc += (float)Difference;

		// Start state machine to get a new sample
		pHpsRegs->EnSyncAdLatch = 0x11;		//b(0):Enable synchronization, b(4):Start state machine
		usleep(100);
	}


	if(DoneSamplingSig==TRUE)
	{
		DoneSamplingSig = FALSE;						// Reset flag
		float AverageRatio = fpRatioAcc/SampleCntUsed;	// Get averaged ratio
		fpRatioAcc    = 0; 								// Reset Accumulator
		SampleCntUsed = 0; 								// Reset sample count

		//===========================================
		//=====         ADDED FOR D2 TEST       =====
		//===========================================
		static u32 bit_flags = {0};
		if( TestMode&(pCommon->test_enable) )					// Check if D2 is enabled
		{
			// Create a flag bit
			u32 flgBit = 1;

			// Compare Ratio to Average
			if(AverageRatio>D3_ERROR_LIMIT)						// 0.2%*65,535 = 131 counts
			{
				if (D2Count[bkgndChan] < 16)			 		// Check hysteresis count
					D2Count[bkgndChan] = D2Count[bkgndChan] +2; // Increment hysteresis count by 2
			}
			else
			{
				if (D2Count[bkgndChan] > 0)				 		// Check if hysteresis count is not 0
					D2Count[bkgndChan] = D2Count[bkgndChan]-1;  // Decrement hysteresis count by 1
			}

			// Compare Ratio to Average
			if(D2Count[bkgndChan]>=8) 				 	 		// if off by 0.2% compared to BIT channel (using hysteresis)
			 bit_flags |=  (flgBit<<bkgndChan);          		// Set flag if difference is greater than threshold
			else
			 bit_flags &= ~(flgBit<<bkgndChan);          		// Clear flag if not

			// Write status flag to FPGA for the current channel
			pHpsRegs->bit_status = bit_flags;

			// Write Test Verify Value
			pCommon->test_verify = 0x55;
		}
		else
		{	// Make all flags passing if D2 is not enabled
			bit_flags = 0;
			pHpsRegs->bit_status = 0;
		}
		//===========================================

		// Move on to next function
		Bkgrnd_Func_Index++;

	}
}




/*
 * Resets function pointer back to beginning
 */
void EndOfBkgrndCalFunctions(void)
{
	Bkgrnd_Func_Index = 0;
}


