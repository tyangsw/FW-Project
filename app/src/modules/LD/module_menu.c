/*
 * LDx/SDx module test menu.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#if SERIAL_DEBUG

#include <includes.h>


/* Global variables */
int Channel;
int NumOfSamples;
int UserSkipCount;

/* Local functions */
static void GetAngle_Position(void);
static void SetChanMode(void);
static void GetChanInfo(void);
static void Get_All_AD_Values(void);
static void Get_Individual_FiltAD_Values(void);
static void Get_Individual_PreFiltAD_Values(void);
static void Get_Individual_PreFiltRefAD_Values(void);
static void GetAD_ScaledValues(void);
static void Set_DAC_ConfigState(void);
static void Set_BIT_Angle(void);
static void Check_BIT_Bandwidth(void);
static void Check_SinCosError(void);
static void Check_DMOD_Error(void);
static void Check_PI_Error(void);
static void Check_Integrator(void);
static void Check_SynthRef(void);
static void ReadFIFOData(void);
static void InitFIFOData(void);
static void GetBITData(void);
static void GetAddressData(void);
static void GetBkgndBitData(void);


/**************************** Exported functions ****************************/

void module_menu(void)
{
    bool    exit_menu = FALSE;
    char    *inbuf;

    /* Print menu choices */
    while (exit_menu == FALSE)
    {
        printf("\n\n>>>  %s Menu  <<<\n\n", MODULE_NAME);
        puts("1.  GetAngle/Position");
        puts("2.  SetChanMode (RSL|4W|2W|SYN)");
        puts("3.  GetChanInfo");
        puts("4.  Get All AD Values");
        puts("5.  Get Individual Filt AD Values");
        puts("6.  Get Individual PreFilt AD Values");
        puts("7.  Get Individual PreFilt Ref AD Values");
        puts("8.  Get AD Scaled Values (SIN/COS/REF)");
        puts("9.  Set DAC ConfigState  (ON / OFF)");
        puts("a.  Set BIT Angle");
        puts("b.  Check BIT Bandwidth");
        puts("c.  Check SinCos Error");
        puts("d.  Check DMOD Error");
        puts("e.  Check PI Error");
        puts("f.  Check Integrator");
        puts("g.  Check SynthRef");
        puts("h.  Init FIFO Data");
        puts("i.  Read FIFO Data");
        puts("j.  Get BIT Data");
        puts("k.  Get Address Data");
        puts("l.  Get Bkgnd BIT Data");

         puts("------------------");
        puts("M. Main menu");
        printf("\nEnter Selection: ");
        inbuf = pend_rx_line();
        if (!inbuf || (strlen(inbuf) > 1))
            continue;

        switch (tolower((int)*inbuf))
        {
			case '1':
				GetAngle_Position();
				break;
			case '2':
				SetChanMode();
				break;
			case '3':
				GetChanInfo();
				break;
			case '4':
				Get_All_AD_Values();
				break;
			case '5':
				Get_Individual_FiltAD_Values();
				break;
			case '6':
				Get_Individual_PreFiltAD_Values();
				break;
			case '7':
				Get_Individual_PreFiltRefAD_Values();
				break;
			case '8':
				GetAD_ScaledValues();
				break;
			case '9':
				Set_DAC_ConfigState();
				break;
			case 'a':
				Set_BIT_Angle();
				break;
			case 'b':
				Check_BIT_Bandwidth();
				break;
			case 'c':
				Check_SinCosError();
				break;
			case 'd':
				Check_DMOD_Error();
				break;
			case 'e':
				Check_PI_Error();
				break;
			case 'f':
				Check_Integrator();
				break;
			case 'g':
				Check_SynthRef();
				break;
			case 'h':
				InitFIFOData();
				break;
			case 'i':
				ReadFIFOData();
				break;
			case 'j':
				GetBITData();
				break;
			case 'k':
				GetAddressData();
				break;
			case 'l':
				GetBkgndBitData();
				break;
			case 'm':
                exit_menu = TRUE;
                break;
        }
    }
}

/****************************** Local functions *****************************/
static void GetAngle_Position(void)
{
    char    		*inbuf;
	float   		tempValue;
	int 			skipCount;
	int 			sampleIndex = 0;
	int 			index = 0;
	int				chanMode;
	volatile int 	*pDPRAM;
	bool			ReadContinuous = FALSE;

	// User Input
	printf("\nEnter Channel(1-4)or(5:BIT): ");
	inbuf = pend_rx_line();
	Channel = atoi(inbuf);

	printf("\nEnter Sample Count(1-%d,C=Cont): ",MAX_SAMPLES);
	inbuf = pend_rx_line();
	if((inbuf[0] == 'c') || (inbuf[0] == 'C'))
	{
		NumOfSamples = 1;
		ReadContinuous = TRUE;
	}
	else
	{
		NumOfSamples =atoi(inbuf);
		ReadContinuous = FALSE;
	}
	if((NumOfSamples < 1) || (NumOfSamples > MAX_SAMPLES))
	{
		printf("\nValid values are 1 - %d",MAX_SAMPLES);
		return;
	}

	printf("\nEnter Skip Count(0-NoSkip): ");
	inbuf = pend_rx_line();
	UserSkipCount = atoi(inbuf);

	if(Channel == 5)
	{
		// Point to channel
		pDPRAM 	= (volatile int*)FPGA_ANGLE_VALUE_BIT;
		chanMode = RESOLVER;
	}
	else
	{
		// Get channel to display
		ChanIndex = (Channel - 1);
		if(ChanIndex < 0)
			ChanIndex = 0;
		if(ChanIndex > (MAX_CHAN-1))
			ChanIndex = (MAX_CHAN-1);

		// Get Mode to display either Angle or Position
		pDPRAM 	= (volatile int*)DPRAM_MODE_SEL_CH1;
		pDPRAM 	+= (ChanIndex * 0x14);
		chanMode = *pDPRAM;

		// Point to channel
		pDPRAM 	= (volatile int*)DPRAM_ANGLE_CH1;
		pDPRAM 	+= (ChanIndex * 0x14);
	}

	do
	{
        if (getchar() == ESCAPE)
        	ReadContinuous = FALSE;

    	skipCount = UserSkipCount;
		for(sampleIndex = 0; sampleIndex < NumOfSamples;)
		{
			// Get data
			if(skipCount == 0)
			{
				WaitForDataReady();
				DataValue[0][sampleIndex++] = *pDPRAM;
				skipCount = UserSkipCount;
			}
			else
			{
				WaitForDataReady();
				if(skipCount > 0)
					skipCount--;
			}
		}
		// Output data to terminal
		if((chanMode == RESOLVER) || (chanMode == SYNCHRO))
		{
			for(index = 0; index < NumOfSamples; index++)
			{
				tempValue = ((float)DataValue[0][index] * ANGLE_LSB32);
				printf("%9.5f\n",tempValue);
			}
		}
		else // LVDT
		{
			for(index = 0; index < NumOfSamples; index++)
			{
				tempValue = ((float)DataValue[0][index] * POSITION_LSB32);
				if(tempValue > 100)
					tempValue = tempValue - 200;
				printf("%9.5f\n",tempValue);
			}
		}

	}while(ReadContinuous);
    pause();// leave this at the end
}
//==========================================================================================
static void SetChanMode(void)
{
    char    		*inbuf;
	volatile int 	*pDPRAM;
    s16				mode;
    s16				chanIndex;


   	// Point to channel
    pDPRAM 	= (volatile int*)DPRAM_MODE_SEL_CH1;
    // dISPLAY CURRENT MODES
    for(chanIndex = 0; chanIndex < MAX_CHAN; chanIndex++)
    {
    	mode = *pDPRAM;
    	switch(mode)
    	{
			case 0:	printf("CH%d set to RSL\n",chanIndex);		break;
			case 1:	printf("CH%d set to 4 WIRE\n",chanIndex);	break;
			case 2:	printf("CH%d set to 2 WIRE\n",chanIndex);	break;
			case 3:	printf("CH%d set to SYN\n",chanIndex);		break;
    	}
    	pDPRAM += 0x14;	// point to next channel
    }
	// User Input
	printf("\nEnter Channel(1-4): ");
	inbuf = pend_rx_line();
	Channel = atoi(inbuf);

	printf("\nEnter Mode(R|S|2|4): ");
	inbuf = pend_rx_line();
	if((inbuf[0] == 'r') || (inbuf[0] == 'R'))
		mode = 0;
	else if((inbuf[0] == 's') || (inbuf[0] == 'S'))
		mode = 3;
	else if(inbuf[0] == '4')
		mode = 1;
	else if(inbuf[0] == '2')
		mode = 2;

	// Get channel
	ChanIndex = (Channel - 1);
	if(ChanIndex < 0)
		ChanIndex = 0;
	if(ChanIndex > (MAX_CHAN-1))
		ChanIndex = (MAX_CHAN-1);

	// Point to channel
	pDPRAM 	= (volatile int*)DPRAM_MODE_SEL_CH1;
	pDPRAM 	+= (ChanIndex * 0x14);

	// Set mode
	*pDPRAM = mode;

    pause();// leave this at the end
}
//==========================================================================================
static void GetChanInfo(void)
{
    char    		*inbuf;
    char			tempMode[10];
    u32				temp_U32_Value;
    s32				tempVel, tempFreq, tempRatio;
    s32				dpramVelocity;
	float   		tempAngle, tempVelocity, tempRefRMS, tempSigRMS;
	volatile int 	*pFPGA_Vel;
	volatile int 	*pDPRAM_Angle, *pDPRAM_RefRMS, *pDPRAM_SigRMS, *pDPRAM_Freq, *pDPRAM_Mode, *pDPRAM_Ratio;
	bool			ReadContinuous = TRUE;

	// User Input
	printf("\nEnter Channel(1-4): ");
	inbuf = pend_rx_line();
	Channel = atoi(inbuf);

	// Get channel to display
	ChanIndex = (Channel - 1);
	if(ChanIndex < 0)
		ChanIndex = 0;
	if(ChanIndex > (MAX_CHAN-1))
		ChanIndex = (MAX_CHAN-1);

	// Point to channel
	pDPRAM_Angle 	= (volatile int*)DPRAM_ANGLE_CH1;
	pDPRAM_Angle 	+= (ChanIndex * 0x14);
	pFPGA_Vel 		= (volatile int*)FPGA_VELOCITY_CH1;
	pFPGA_Vel	 	+= (ChanIndex * 0x1);
	pDPRAM_RefRMS	= (volatile int*)DPRAM_MEAS_REF_CH1;
	pDPRAM_RefRMS 	+= (ChanIndex * 0x14);
	pDPRAM_SigRMS	= (volatile int*)DPRAM_MEAS_SIG_CH1;
	pDPRAM_SigRMS 	+= (ChanIndex * 0x14);
	pDPRAM_Freq		= (volatile int*)DPRAM_MEAS_FREQ_CH1;
	pDPRAM_Freq 	+= (ChanIndex * 0x14);
	pDPRAM_Mode		= (volatile int*)DPRAM_MODE_SEL_CH1;
	pDPRAM_Mode 	+= (ChanIndex * 0x14);
	pDPRAM_Ratio	= (volatile int*)DPRAM_RATIO_CH1_2;
	pDPRAM_Ratio 	+= ((ChanIndex / 2) * 0x28);


	printf("\n   ANGLE     VELOCITY  REF RMS     SIG RMS      FREQ    MODE    RATIO \n");
	do
	{
        if (getchar() == ESCAPE)
        	ReadContinuous = FALSE;

        // Get Angle
        temp_U32_Value = *pDPRAM_Angle;
		tempAngle = ((float)temp_U32_Value * ANGLE_LSB32);
        // Get Velocity
		tempVel = *pFPGA_Vel;
		tempVelocity = (float)tempVel / VELOCITY_SCALE;
		dpramVelocity = (s32)(tempVelocity);

	    // Get RefRMS
		temp_U32_Value = *pDPRAM_RefRMS;
		tempRefRMS = ((float)(temp_U32_Value) / 100.0);
	    // Get SigRMS
		temp_U32_Value = *pDPRAM_SigRMS;
		tempSigRMS = ((float)(temp_U32_Value) / 100.0);
	    // Get Freq
		tempFreq = *pDPRAM_Freq;

	    // Get Mode
		temp_U32_Value = *pDPRAM_Mode;
		switch(temp_U32_Value)
		{
			case 0: strcpy(tempMode,"RSL"); break;
			case 1: strcpy(tempMode,"4W"); break;
			case 2: strcpy(tempMode,"2W"); break;
			case 3: strcpy(tempMode,"SYN"); break;
			default: strcpy(tempMode,"N/A");
		}
	    // Get Ratio
		tempRatio = *pDPRAM_Ratio;

		// display
		printf("%9.4f    %+6ld     %6.2f      %6.2f    %+6d     %s     %2d\r",tempAngle, dpramVelocity, tempRefRMS, tempSigRMS, (s16)tempFreq, tempMode, (s16)tempRatio);


	}while(ReadContinuous);
    pause();// leave this at the end
}
//==========================================================================================
void Get_All_AD_Values(void)
{
    char    		*inbuf;
	s16 			temp_s16;
	u16 			temp_u16;
	u16 			sampleIndex;
	u16 			skipCount;
	u16				a2d_num_index;
	volatile int 	*pFPGA, *pFPGA2, *pFPGA3;

	//---------------------------------
	printf("\nEnter AD (Sin/Cos/LVRef/HVRef Ch1=1, Ch2=2, Ch3=3, Ch4=4: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if((temp_u16 < 1) || (temp_u16 > 4))
	{
		printf("\nValid values are 1 - 4");
		return;
	}
	a2d_num_index = temp_u16 - 1;
	//---------------------------------
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if((temp_u16 < 1) || (temp_u16 > MAX_SAMPLES))
	{
		printf("\nValid values are 1 - %d",MAX_SAMPLES);
		return;
	}
	NumOfSamples = temp_u16;
	//---------------------------------
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	temp_s16 = atoi(inbuf);
	if(temp_s16 < 0)
	{
		printf("\nValid values are 0 - 65535");
		return;
	}
	UserSkipCount = temp_s16;
	//---------------------------------
	// Point to proper AD
	pFPGA    =  (volatile int*)FPGA_PRE_FILTER_SIN_AD_CH1;
	pFPGA 	+= (a2d_num_index * 0x02);
	pFPGA2   =  (volatile int*)FPGA_PRE_FILTER_REF_LV_AD_CH1;
	pFPGA2 	+= (a2d_num_index * 0x01);
	pFPGA3   =  (volatile int*)FPGA_FILTERED_SIN_AD_CH1;
	pFPGA3 	+= (a2d_num_index * 0x01);

	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex] = *(pFPGA + 0x0000);	// FPGA_PRE_FILTER_SIN_AD
			DataValue[1][sampleIndex] = *(pFPGA + 0x0001);	// FPGA_PRE_FILTER_COS_AD
			DataValue[2][sampleIndex] = *(pFPGA2 + 0x0000);	// FPGA_PRE_FILTER_REF_LV_AD
			DataValue[3][sampleIndex] = *(pFPGA2 + 0x0004);	// FPGA_PRE_FILTER_REF_HV_AD
			DataValue[4][sampleIndex] = *(pFPGA2 + 0x0008);	// FPGA_PRE_FILTERED_REF_AD (selected)
			DataValue[5][sampleIndex] = *(pFPGA3 + 0x0000);	// FPGA_FILTERED_SIN_AD
			DataValue[6][sampleIndex] = *(pFPGA3 + 0x0004);	// FPGA_FILTERED_COS_AD
			DataValue[7][sampleIndex] = *(pFPGA3 + 0x0008);	// FPGA_FILTERED_REF_AD

			sampleIndex++;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\t",(int)((DataValue[0][sampleIndex]) * 256));// FPGA_PRE_FILTER_SIN_AD
		printf("%+11d\t",(int)((DataValue[1][sampleIndex]) * 256));// FPGA_PRE_FILTER_COS_AD
		printf("%+11d\t",(int)((DataValue[2][sampleIndex]) * 256));// FPGA_PRE_FILTER_REF_LV_AD
		printf("%+11d\t",(int)((DataValue[3][sampleIndex]) * 256));// FPGA_PRE_FILTER_REF_HV_AD
		printf("%+11d\t",(int)((DataValue[4][sampleIndex]) * 256));// FPGA_PRE_FILTER_REF_AD
		printf("%+11d\t",(int)((DataValue[5][sampleIndex]) * 256));// FPGA_FILTERED_SIN_AD
		printf("%+11d\t",(int)((DataValue[6][sampleIndex]) * 256));// FPGA_FILTERED_COS_AD
		printf("%+11d\t",(int)((DataValue[7][sampleIndex]) * 256));// FPGA_FILTERED_REF_AD
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................
void Get_Individual_FiltAD_Values(void)
{
    char    		*inbuf;
	s16 			temp_s16;
	u16 			temp_u16;
	u16 			sampleIndex;
	u16 			skipCount;
	u16				a2d_num_index;
	bool			ReadContinuous = FALSE;
	volatile int 	*pFPGA;

	//---------------------------------
	printf("\nEnter AD index: (SIN CH1=0, SIN CH2=1, SIN CH3=2, SIN CH4=3....,COS CH4=7,REF CH1=8...: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if(temp_u16 > 11)
	{
		printf("\nValid values are 0 - 11");
		return;
	}
	a2d_num_index = temp_u16;
	//---------------------------------
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if((inbuf[0] == 'c') || (inbuf[0] == 'C'))
	{
		NumOfSamples = 1;
		ReadContinuous = TRUE;
	}
	else
	{
		NumOfSamples =atoi(inbuf);
		ReadContinuous = FALSE;
	}
	if((NumOfSamples < 1) || (NumOfSamples > MAX_SAMPLES))
	{
		printf("\nValid values are 1 - %d",MAX_SAMPLES);
		return;
	}
	//---------------------------------
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	temp_s16 = atoi(inbuf);
	if(temp_s16 < 0)
	{
		printf("\nValid values are 0 - 65535");
		return;
	}
	UserSkipCount = temp_s16;
	//---------------------------------
	// Point to proper AD
	pFPGA =  (volatile int*)FPGA_FILTERED_SIN_AD_CH1;
	pFPGA 	+= (a2d_num_index * 0x01);

	do
	{
        if (getchar() == ESCAPE)
        	ReadContinuous = FALSE;

        skipCount = UserSkipCount;
		for(sampleIndex = 0; sampleIndex < NumOfSamples;)
		{
			// Get data
			if(skipCount == 0)
			{
				WaitForDataReady();
				DataValue[0][sampleIndex] = *pFPGA;
				sampleIndex++;
				skipCount = UserSkipCount;
			}
			else
			{
				WaitForDataReady();
				if(skipCount > 0)
					skipCount--;
			}
		}
		// Output data to terminal
		for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
		{
			printf("%+11d\n",(int)((DataValue[0][sampleIndex]) * 256));
		}

	}while(ReadContinuous);
	pause();// leave this at the end
}
//................................................................................
void Get_Individual_PreFiltAD_Values(void)
{
    char    		*inbuf;
	s16 			temp_s16;
	u16 			temp_u16;
	u16 			sampleIndex;
	u16 			skipCount;
	u16				a2d_num_index;
	volatile int 	*pFPGA;

	//---------------------------------
	printf("\nEnter AD index: (SIN CH1=0, COS CH1=1, SIN CH2=2, COS CH2=3....,COS CH4=7: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if(temp_u16 > 11)
	{
		printf("\nValid values are 0 - 11");
		return;
	}
	a2d_num_index = temp_u16;
	//---------------------------------
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if((temp_u16 < 1) || (temp_u16 > MAX_SAMPLES))
	{
		printf("\nValid values are 1 - %d",MAX_SAMPLES);
		return;
	}
	NumOfSamples = temp_u16;
	//---------------------------------
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	temp_s16 = atoi(inbuf);
	if(temp_s16 < 0)
	{
		printf("\nValid values are 0 - 65535");
		return;
	}
	UserSkipCount = temp_s16;
	//---------------------------------
	// Point to proper AD
	pFPGA =  (volatile int*)FPGA_PRE_FILTER_SIN_AD_CH1;
	pFPGA 	+= (a2d_num_index * 0x01);

	skipCount = UserSkipCount;
	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex] = *pFPGA;
			sampleIndex++;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\n",(int)((DataValue[0][sampleIndex]) * 256));
	}

	pause();// leave this at the end
}
//................................................................................
void Get_Individual_PreFiltRefAD_Values(void)
{
    char    		*inbuf;
	s16 			temp_s16;
	u16 			temp_u16;
	u16 			sampleIndex;
	u16 			skipCount;
	u16				a2d_num_index;
	volatile int 	*pFPGA;

	//---------------------------------
	printf("\nEnter AD index: (REF-LV CH1=0, REF-LV CH2=1,....,REF-HV CH1=4, REF-HV CH2=5,....,REF-SEL CH1=8, REF-SEL CH2=9: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if(temp_u16 > 11)
	{
		printf("\nValid values are 0 - 11");
		return;
	}
	a2d_num_index = temp_u16;
	//---------------------------------
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if((temp_u16 < 1) || (temp_u16 > MAX_SAMPLES))
	{
		printf("\nValid values are 1 - %d",MAX_SAMPLES);
		return;
	}
	NumOfSamples = temp_u16;
	//---------------------------------
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	temp_s16 = atoi(inbuf);
	if(temp_s16 < 0)
	{
		printf("\nValid values are 0 - 65535");
		return;
	}
	UserSkipCount = temp_s16;
	//---------------------------------
	// Point to proper AD
	pFPGA =  (volatile int*)FPGA_PRE_FILTER_REF_LV_AD_CH1;
	pFPGA 	+= (a2d_num_index * 0x01);

	skipCount = UserSkipCount;
	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex] = *pFPGA;
			sampleIndex++;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\n",(int)((DataValue[0][sampleIndex]) * 256));
	}

	pause();// leave this at the end
}
//................................................................................
void GetAD_ScaledValues(void)
{
    char    		*inbuf;
	u16 			sampleIndex;
	u16 			skipCount;
	u16				a2d_num_index;
	volatile int 	*pFPGA;

	//....................................
	printf("\nEnter AD (Sin/Cos/Ref Ch1=1, Sin/Cos/Ref Ch2=2, Sin/Cos/Ref Ch3=3, Sin/Cos/Ref Ch4=4: ");
	inbuf = pend_rx_line();
	a2d_num_index = atoi(inbuf);
	if((a2d_num_index < 1) || (a2d_num_index > MAX_CHAN))
	{
		printf("\nValid values are 1 - %d",MAX_CHAN);
		return;
	}
	//....................................
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	NumOfSamples =atoi(inbuf);
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	UserSkipCount = atoi(inbuf);

	// Point to proper AD
	pFPGA =  (volatile int*)FPGA_SCALED_SIN_CH1;
	pFPGA 	+= ((a2d_num_index - 1) * 0x01);

	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex] = *(pFPGA + 0x0000);
			DataValue[1][sampleIndex] = *(pFPGA + 0x0004);
			DataValue[2][sampleIndex] = *(pFPGA + 0x0008);
			sampleIndex++;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\t",(int)((DataValue[0][sampleIndex]) * 256));
		printf("%+11d\t",(int)((DataValue[1][sampleIndex]) * 256));
		printf("%+11d\t",(int)((DataValue[2][sampleIndex]) * 256));
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................
void Set_DAC_ConfigState(void)
{
    char    		*inbuf;
	u16 			dac_state;
	u16				waitCount;
	volatile int 	*pFPGA;

	// User Input
	printf("\nEnter Channel(1-4): ");
	inbuf = pend_rx_line();
	Channel = atoi(inbuf);

	// Get channel to display
	ChanIndex = (Channel - 1);
	if(ChanIndex < 0)
		ChanIndex = 0;
	if(ChanIndex > (MAX_CHAN-1))
		ChanIndex = (MAX_CHAN-1);

	// User Input
	printf("\nEnter DAC Config State:  [0=On, 1=Off] ");
	inbuf = pend_rx_line();
	dac_state = atoi(inbuf);

	// First, set mux input to receive configuration word instead of Sine/Cosine
	pFPGA =  (volatile int*)FPGA_DAC_CONFIG_MODE;
	*pFPGA = 0x0001;

	// Now set state (For now we will do all channels the same)
	pFPGA =  (volatile int*)FPGA_DAC_CONFIG_WORD;
	if(dac_state == 1)
		*pFPGA = 0x004FFFFF;
	else
		*pFPGA = 0x004F0000;

	// Wait a few cycles to allow write to take place
	for(waitCount = 0; waitCount < 10; waitCount++)
		WaitForDataReady();

	// Last, set mux input to receive Sine/Cosine
	pFPGA =  (volatile int*)FPGA_DAC_CONFIG_MODE;
	*pFPGA = 0x0000;


	pause();// leave this at the end
}
//................................................................................
void Set_BIT_Angle(void)
{
    char    		*inbuf;
    float			bit_angle;
    u32				bit_angle_hex;
	volatile u32 	*pFPGA;

	// User Input
	printf("\nEnter Angle: ");
	inbuf = pend_rx_line();
	bit_angle = atof(inbuf);


	bit_angle_hex = (bit_angle / ANGLE_LSB32);

	// Now set state (For now we will do all channels the same)
	pFPGA 	= (volatile u32*)FPGA_ARM_BIT_ANGLE;
	*pFPGA = bit_angle_hex;

	pause();// leave this at the end
}
//................................................................................
void Check_BIT_Bandwidth(void)
{
    char    		*inbuf;
    u32				bit_mode;
    u32				bit_angle_hex;
    u32				sampleIndex, index;
	u32				waitCount;
	u16 			skipCount;
	float			tempValue;
	volatile u32 	*pFPGA;
	volatile int 	*pDPRAM;

	// User Input
	printf("\nEnter Channel(1-4): ");
	inbuf = pend_rx_line();
	Channel = atoi(inbuf);

	// User Input
	printf("\nEnter Skip Count(0-NoSkip): ");
	inbuf = pend_rx_line();
	UserSkipCount = atoi(inbuf);


	//------------------------------------------------
	// 			Set device to BIT mode
	//------------------------------------------------
	pFPGA 	= (volatile u32*)DPRAM_BIT_MODE;
	bit_mode = *pFPGA;
	*pFPGA  = (bit_mode | 0x0001);				// Set D0 bit
	InBITMode = 1;
	pFPGA 	= (volatile u32*)FPGA_IN_BIT_MODE;
	*pFPGA 	= InBITMode;

	// Get pointers
	// Point to channel
	pDPRAM 	= (volatile int*)DPRAM_ANGLE_CH1;
	pDPRAM 	+= (ChanIndex * 0x14);
	pFPGA 	= (volatile u32*)FPGA_ARM_BIT_ANGLE;

	//------------------------------------------------
	// 			Set angle to 5 degrees
	//------------------------------------------------
	bit_angle_hex = (5.0 / ANGLE_LSB32);
	*pFPGA = bit_angle_hex;

	// Wait 2 seconds
	for(waitCount = 0; waitCount < (48828 * 2); waitCount++)
		WaitForDataReady();

	skipCount = UserSkipCount;
	//------------------------------------------------
	// 		Take 200 samples of data @ 5 degrees
	//------------------------------------------------
	for(sampleIndex = 0; sampleIndex < 200;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex++] = *pDPRAM;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	//------------------------------------------------
	// 			Set angle to 6 degrees
	//------------------------------------------------
	bit_angle_hex = (6.0 / ANGLE_LSB32);
	*pFPGA = bit_angle_hex;
	//------------------------------------------------
	// 		Take 3896 samples of data @ 6 degrees
	//------------------------------------------------
	for(sampleIndex = 200; sampleIndex < 4096;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex++] = *pDPRAM;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	//------------------------------------------------
	// 			Set angle back to 5 degrees
	//------------------------------------------------
	bit_angle_hex = (5.0 / ANGLE_LSB32);
	*pFPGA = bit_angle_hex;
	//------------------------------------------------
	// 		Take 4096 samples of data @ 5 degrees
	//------------------------------------------------
	for(sampleIndex = 4096; sampleIndex < 7992;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex++] = *pDPRAM;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	//------------------------------------------------
	// 			Set angle to 6 degrees
	//------------------------------------------------
	bit_angle_hex = (6.0 / ANGLE_LSB32);
	*pFPGA = bit_angle_hex;
	//------------------------------------------------
	// 		Take 3896 samples of data @ 6 degrees
	//------------------------------------------------
	for(sampleIndex = 7992; sampleIndex < 8192;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex++] = *pDPRAM;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	//------------------------------------------------
	// 			Output data to terminal
	//------------------------------------------------
	for(index = 0; index < 8192; index++)
	{
		tempValue = ((float)DataValue[0][index] * ANGLE_LSB32);
		printf("%9.5f\n",tempValue);
	}

	pause();// leave this at the end
}
//................................................................................
void Check_SinCosError(void)
{
    char    		*inbuf;
	u16 			sampleIndex;
	u16 			skipCount;
	u16				chan_index;
	volatile int 	*pFPGA;

	printf("\nEnter Channel: ");
	inbuf = pend_rx_line();
	chan_index = (atoi(inbuf) - 1);
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	NumOfSamples =atoi(inbuf);
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	UserSkipCount = atoi(inbuf);

	// Point to proper AD
	pFPGA =  (volatile int*)FPGA_SIN_COS_ERROR_CH1;
	pFPGA 	+= (chan_index * 0x01);

	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex++] = *pFPGA;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\t",(int)(DataValue[0][sampleIndex])  );
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................
void Check_DMOD_Error(void)
{
    char    		*inbuf;
	u16 			sampleIndex;
	u16 			skipCount;
	u16				chan_index;
	volatile int 	*pFPGA;

	printf("\nEnter Channel: ");
	inbuf = pend_rx_line();
	chan_index = (atoi(inbuf) - 1);
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	NumOfSamples =atoi(inbuf);
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	UserSkipCount = atoi(inbuf);

	// Point to proper AD
	pFPGA =  (volatile int*)FPGA_DEMOD_ERROR_CH1;
	pFPGA 	+= (chan_index * 0x01);

	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex++] = *pFPGA;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\t",(int)((DataValue[0][sampleIndex]) * 1));
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................
void Check_PI_Error(void)
{
    char    		*inbuf;
	u16 			sampleIndex;
	u16 			skipCount;
	u16				chan_index;
	volatile int 	*pFPGA;

	printf("\nEnter Channel: ");
	inbuf = pend_rx_line();
	chan_index = (atoi(inbuf) - 1);
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	NumOfSamples =atoi(inbuf);
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	UserSkipCount = atoi(inbuf);

	// Point to proper AD
	pFPGA =  (volatile int*)FPGA_PI_ERROR_CH1;
	pFPGA 	+= (chan_index * 0x01);

	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex++] = *pFPGA;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\t",(int)((DataValue[0][sampleIndex]) * 1));
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................
void Check_Integrator(void)
{
    char    		*inbuf;
	u16 			sampleIndex;
	u16 			skipCount;
	u16				chan_index;
	volatile int 	*pFPGA;

	printf("\nEnter Channel: ");
	inbuf = pend_rx_line();
	chan_index = (atoi(inbuf) - 1);
//	printf("\nEnter Sample Count: ");
//	inbuf = pend_rx_line();
	NumOfSamples  = 2048;
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	UserSkipCount = atoi(inbuf);

	// Point to proper AD
	pFPGA =  (volatile int*)FPGA_INTEGRATOR_VAL_CH1;
	pFPGA 	+= (chan_index * 0x01);

	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			Integrator[sampleIndex++] = *(volatile float*)pFPGA;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%f\t",Integrator[sampleIndex]);
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................
void Check_SynthRef(void)
{
    char    		*inbuf;
	u16 			sampleIndex;
	u16 			skipCount;
	u16				chan_index;
	volatile int 	*pFPGA;

	printf("\nEnter Channel: ");
	inbuf = pend_rx_line();
	chan_index = (atoi(inbuf) - 1);
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	NumOfSamples =atoi(inbuf);
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	UserSkipCount = atoi(inbuf);

	// Point to proper AD
	pFPGA =  (volatile int*)FPGA_SYNTH_REF_CH1;
	pFPGA 	+= (chan_index * 0x01);

	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex++] = *pFPGA;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\t",(int)((DataValue[0][sampleIndex]) * 1));
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................
void InitFIFOData(void)
{
	volatile int 	*pFPGA;


	// Point to proper AD
	pFPGA =  (volatile int*)FPGA_ENABLE_FIFO_DATA;

   *pFPGA = 1;

	pause();// leave this at the end
}
//................................................................................
void ReadFIFOData(void)
{
	int				sampleIndex;
	volatile int 	*pFPGA1;
//	volatile int 	*pFPGA2;


	// Point to proper AD
	pFPGA1 =  (volatile int*)FPGA_RD_FIFO_DATA1;
//	pFPGA2 =  (volatile int*)FPGA_RD_FIFO_DATA2;

	for(sampleIndex = 0; sampleIndex < 2048; sampleIndex++)
	{
		WaitForDataReady();
		DataValue[0][sampleIndex] = *pFPGA1;
//		DataValue[1][sampleIndex] = *pFPGA2;
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < 2048; sampleIndex++)
	{
		printf("%+13d\t",(int)(DataValue[0][sampleIndex]));
//		printf("%+13d\t",(int)(DataValue[1][sampleIndex]));
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................
void GetBITData(void)
{
    char    		*inbuf;
	s16 			temp_s16;
	u16 			temp_u16;
	u16 			sampleIndex;
	u16 			skipCount;
	volatile int 	*pFPGA;
	volatile int 	*pFPGA2;
	volatile int 	*pFPGA3;
	volatile int 	*pFPGA4;

	//---------------------------------
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if((temp_u16 < 1) || (temp_u16 > MAX_SAMPLES))
	{
		printf("\nValid values are 1 - %d",MAX_SAMPLES);
		return;
	}
	NumOfSamples = temp_u16;
	//---------------------------------
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	temp_s16 = atoi(inbuf);
	if(temp_s16 < 0)
	{
		printf("\nValid values are 0 - 65535");
		return;
	}
	UserSkipCount = temp_s16;
	//---------------------------------
	// Point to proper AD
	pFPGA     =  (volatile int*)FPGA_SCALED_SIN_AD_BIT;
	pFPGA2    =  (volatile int*)FPGA_DEMOD_ERROR_BIT;
	pFPGA3    =  (volatile int*)FPGA_PI_ERROR_BIT;
	pFPGA4    =  (volatile int*)FPGA_SIN_NCO_DSP_BIT;

	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex] = *(pFPGA + 0x0000);	// SIN_AD
			DataValue[1][sampleIndex] = *(pFPGA + 0x0001);	// COS_AD
			DataValue[2][sampleIndex] = *(pFPGA + 0x0002);	// REF_AD
			DataValue[3][sampleIndex] = *(pFPGA2 + 0x0000);	// DMOD ERR
			DataValue[4][sampleIndex] = *(pFPGA3 + 0x0000);	// PI   ERR
			DataValue[5][sampleIndex] = *(pFPGA4 + 0x0000);	// NCO SIN
			DataValue[6][sampleIndex] = *(pFPGA4 + 0x0001);	// NCO COS

			sampleIndex++;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\t",(int)((DataValue[0][sampleIndex]) * 256));// SIN_AD
		printf("%+11d\t",(int)((DataValue[1][sampleIndex]) * 256));// COS_AD
		printf("%+11d\t",(int)((DataValue[2][sampleIndex]) * 256));// REF_AD
		printf("%+11d\t",(int)((DataValue[3][sampleIndex]) * 1));// DMOD ERR
		printf("%+11d\t",(int)((DataValue[4][sampleIndex]) * 1));// PI   ERR
		printf("%+11d\t",(int)((DataValue[5][sampleIndex]) * 1));// NCO SIN
		printf("%+11d\t",(int)((DataValue[6][sampleIndex]) * 1));// NCO COS
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................
void GetAddressData(void)
{
    char    			*inbuf;
    u32     			val;
	u16 				temp_u16;
	u16 				sampleIndex;
	u16 				skipCount;
    static u32  		address	 	 = 0x60000000UL;
    static u32  		multiplier	 = 0x00000001UL;
	volatile int 		*pFPGA;

	/* Get  address */
	printf("Address [%lX]: ", address);
	inbuf = pend_rx_line();
	if (!inbuf)
	{
		pause();
		return;
	}
	if (strlen(inbuf))
	{
		if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
		{
			address = val;
		}
		else
		{
			puts("Valid values are 0x0 to 0xFFFFFFFF");
			return;
		}
	}
	/* Get  multiplier */
	printf("Multiplier(in Hex) [0x%lX]: ", multiplier);
	inbuf = pend_rx_line();
	if (!inbuf)
	{
		pause();
		return;
	}
	if (strlen(inbuf))
	{
		if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
		{
			multiplier = val;
		}
		else
		{
			puts("Valid values are 0x0001 to 0x0100");
			return;
		}
	}
	//---------------------------------
	printf("\nEnter Sample Count: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if((temp_u16 < 1) || (temp_u16 > MAX_SAMPLES))
	{
		printf("\nValid values are 1 - %d",MAX_SAMPLES);
		return;
	}
	NumOfSamples = temp_u16;
	//---------------------------------
	printf("\nEnter Skip Count: ");
	inbuf = pend_rx_line();
	temp_u16 = atoi(inbuf);
	if(temp_u16 < 0)
	{
		printf("\nValid values are 0 - 65535");
		return;
	}
	UserSkipCount = temp_u16;
	//---------------------------------
	// Point to proper AD
	pFPGA     =  (volatile int*)address;
	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
		// Get data
		if(skipCount == 0)
		{
			WaitForDataReady();
			DataValue[0][sampleIndex] = (*pFPGA * multiplier);

			sampleIndex++;
			skipCount = UserSkipCount;
		}
		else
		{
			WaitForDataReady();
			if(skipCount > 0)
				skipCount--;
		}
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\t",(int)((DataValue[0][sampleIndex]) * 1));
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................
void GetBkgndBitData(void)
{
	u16 			sampleIndex;
	volatile int 	*pFPGA1;
	volatile int 	*pFPGA2;
	volatile int 	*pFPGA3;

	NumOfSamples  = 1024;
	UserSkipCount = 0;
	//---------------------------------
	// Point to proper AD
	pFPGA1    =  (volatile int*)FPGA_SIN_AD_BKND_BIT;
	pFPGA2    =  (volatile int*)FPGA_COS_AD_BKND_BIT;
	pFPGA3    =  (volatile int*)FPGA_REF_AD_BKND_BIT;

	for(sampleIndex = 0; sampleIndex < NumOfSamples;)
	{
			WaitForDataReady();
			DataValue[0][sampleIndex] = *pFPGA1;
			DataValue[1][sampleIndex] = *pFPGA2;
			DataValue[2][sampleIndex] = *pFPGA3;
			sampleIndex++;
	}
	// Output data to terminal
	for(sampleIndex = 0; sampleIndex < NumOfSamples; sampleIndex++)
	{
		printf("%+11d\t",(int)((DataValue[0][sampleIndex]) * 256));// SIN_AD
		printf("%+11d\t",(int)((DataValue[1][sampleIndex]) * 256));// COS_AD
		printf("%+11d\t",(int)((DataValue[2][sampleIndex]) * 256));// REF_AD
		printf("\n");
	}

	pause();// leave this at the end
}
//................................................................................


#endif // #if SERIAL_DEBUG

