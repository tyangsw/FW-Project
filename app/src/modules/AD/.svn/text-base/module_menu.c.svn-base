/*
 * ADx module test menu.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#if SERIAL_DEBUG

#include <includes.h>


/* Local functions */
static void     ModulePower(void);
static void     InitDefaults(void);
static void     ReadAD(void);
static void     ActiveChannels(void);
static void     ChangePolAndRange(void);
static void     ChangeMode(void);
static void     ChangeSampRate(void);
static void     GetSampRate(void);
static uint8_t  GetActChanCnt(void);
//static bool     NewPLLParameters(uint32_t chanRate, uint32_t ActChanCnt);
//static bool     calcPllParameters(double adRate, double Fvco_D, uint32_t N_var);
static void     ChangeDecimators(void);
static void     TestEnable(void);
static void     InitFifo(void);
static void     EnableFifo(void);
static void     ReadFifo(void);
static void     DumpFifo(void);
static void     ADBuffer(void);

/* Global Variables */
//static uint32_t  M_var, C_var, K_uint;
//static double    Fout;
static uint32_t  chanactive=0;
static uint8_t   ActChanCnt=0;
static uint32_t  Ch_range=0x00000010;
static uint32_t  chanRate=62500;
static uint32_t  decimatorsCnt;

static HOST_REGS *pHostRegs = (HOST_REGS *)HOST_REGS_BASE;
static HPS_REGS  *pHpsRegs  = (HPS_REGS *)HPS_REGS_BASE;


/**************************** Exported functions ****************************/

void module_menu(void)
{
    bool    exit_menu = FALSE;
    char    *inbuf;

    /* Print menu choices */
    while (exit_menu == FALSE)
    {
        printf("\n\n>>>  %s Menu  <<<\n\n", MODULE_NAME);
        puts("1. Module Power");
        puts("2. Defaults (All Active, bipolar, V-Mode, Range 0)");
        puts("3. Read A/D");
        puts("4. Change Polarity & Range");
        puts("5. Active Channels");
        puts("6. Mode (I or V)");
        puts("7. Sampling Rate");
        puts("8. Change Decimation Stages");
        puts("9. Test Modes (D0,D2,D3)");
        puts("A. Initialize FIFO");
        puts("B. Reset and Enable FIFO");
        puts("C. Read FIFO");
        puts("D. Dump FIFO data");
        puts("E. Take A/D Buffer");
        puts("------------------");
        puts("M. Main menu");
        printf("\nEnter Selection: ");
        inbuf = pend_rx_line();
        if (!inbuf || (strlen(inbuf) > 1))
            continue;

        switch (tolower((int)*inbuf))
        {
            case '1':
                ModulePower();
                break;
            case '2':
                InitDefaults();
                break;
            case '3':
                ReadAD();
                break;
            case '4':
                ChangePolAndRange();
                break;
            case '5':
                ActiveChannels();
                break;
            case '6':
                ChangeMode();
                break;
            case '7':
                ChangeSampRate();
                break;
            case '8':
                ChangeDecimators();
                break;
            case '9':
                TestEnable();
                break;
            case 'a':
                InitFifo();
                break;
            case 'b':
                EnableFifo();
                break;
            case 'c':
                ReadFifo();
                break;
            case 'd':
                DumpFifo();
                break;
            case 'e':
                ADBuffer();
                break;
            case 'm':
                exit_menu = TRUE;
                break;
        }
    }
}

/****************************** Local functions *****************************/

static void ModulePower(void)
{
    pause();
    return;
}


static void InitDefaults(void)
{
  //  bool valid;

    printf("\n -<>- Setting Defaults -<>-\n");

    // Make all channels active
    // naiwritereg32 0x00001898 0x0000FFFF
    chanactive = 0x0000FFFF;
    pHostRegs->activeChan=chanactive;
    printf("Active Channels: 0xFFFF\n");

    // Set mode to voltage mode
    //naiwritereg32 0x00001894 0x00000000
    pHostRegs->iv_mode=0x00000000;
    printf("Set to Voltage Mode: 0x0000\n");

    // Write Polarity to 2's compliment and range 0 for PGA281
    // loop all 16 channels naiwritereg32 0x00001080 0x00000008
    Ch_range = 0x00000010;
    for(int i=0;i<NUM_CHANNELS;i++)
    {
        pHostRegs->ad_polrange[i]= Ch_range;
        pHpsRegs ->ad_range[i].reg=Ch_range;
    }
    printf("Set 2's Comp and Range 0: 0x0010\n");


    // Set default sampling rate of 62500Hz per channel
    chanRate = 62500;
    printf("Sampling Rate per Chan: %lu Hz\n", chanRate);

    // Get the amount of active channels that will be used
    //  Count requires that the total is the same between each bank
    ActChanCnt = GetActChanCnt();
    if(ActChanCnt == 0)
    {
        puts("No active channels so exiting");
        pause();
        return;
    }

    // Calculate and write new PLL parameters
    //valid = NewPLLParameters(chanRate, ActChanCnt);
/*    if(valid == FALSE)
    {
        puts("Unsuccessful configuration!");
        pause();
        return;
    }
*/
    printf("\n...done setting Defaults\n");
    pause();
    return;
}


static void ReadAD(void)
{
    uint32_t    val;
    uint32_t    rval[NUM_CHANNELS];
    uint32_t    chanindex = 0;
    uint32_t    readagain = 1;
    char        *inbuf;

    /* Get channel number */
    printf("\nChannel (1-%d) or '0' for all [%lu]: ", NUM_CHANNELS, chanindex);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && (val>=0) && (val<=NUM_CHANNELS))
        {
            chanindex = val;
        }
        else
        {
            puts("Valid values are 0x0 to 0x11");
            pause();
            return;
        }
    }

    // Create loop that will keep reading until user wants to exit
    while(readagain==1)
    {
        if(chanindex == 0)                                          // See if user wants to read all channels
        {
            int i;
            for(i=0;i<NUM_CHANNELS;i++)                                       // Loop through each channel
            {
                rval[i] = pHpsRegs->ad_reading[i];                  // Store values to array
                printf("Channel %2d : %08lX\n", (i+1), rval[i]);    // Display all the channels data
            }
        }
        else                                                        // Single channel read
        {
            rval[0] = pHpsRegs->ad_reading[chanindex-1];            // Store read value into first array index
            printf("The A/D value of Channel %lu is:%08lX\n", chanindex, rval[0]); // Display it
        }

        // See if user wants to repeate the reading or exit
        printf("Read again? 1:yes, 0:no [%lu]: ", readagain);
        inbuf = pend_rx_line();
        if (!inbuf)
        {
            pause();
            return;
        }
        if (strlen(inbuf) > 0)
        {
            if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && (val>=0) && (val<=1))
            {
                readagain = val;
            }
            else
            {
                puts("Valid values are 0x0 or 0x1...exiting");
                pause();
                return;
            }
        }
    }
    return;
}


static void ChangePolAndRange(void)
{
    char        *inbuf;
    uint32_t    val;

    // Get new polarity and range value from user
    puts("\nb[4]=Polarity(1=2's Comp), b[3:0]=Range");
    printf("Enter Bitmapped pattern (0:Act,1:Dis) [0x%04lX]: ", Ch_range);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && (val>=0) && (val<=0x0000001F))
        {
            Ch_range = val;
        }
        else
        {
            puts("Valid values are 0x0000 to 0x001F for 16 channels");
            pause();
            return;
        }
    }

    // Write value to all channels
    for(int i=0;i<NUM_CHANNELS;i++)
    {
        pHostRegs->ad_polrange[i]= Ch_range;
        pHpsRegs ->ad_range[i].reg=Ch_range;
    }

    pause();
    return;
}


static void ActiveChannels(void)
{
    HOST_REGS  *pHostRegs = (HOST_REGS *)HOST_REGS_BASE;
    char        *inbuf;
    uint32_t    val;

    // See which channels user wants to make active
    printf("Enter Bitmapped pattern [0x%04lX]: ", chanactive);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && (val>=0) && (val<=0x0000FFFF))
        {
            chanactive = val;
        }
        else
        {
            puts("Valid values are 0x0000 to 0xFFFF for 16 channels");
            pause();
            return;
        }
    }

    // Write active channels
    pHostRegs->activeChan=chanactive;
    pause();
    return;

}


static void ChangeMode(void)
{
    HOST_REGS  *pHostRegs = (HOST_REGS *)HOST_REGS_BASE;
    char        *inbuf;
    uint32_t    val;
    uint32_t    chanmode=0;

    // See what mode the user wants to set each channel to
    printf("Enter Bitmapped pattern (0:V, 1:I) [0x%04lX]: ", chanmode);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && (val>=0) && (val<=0x0000FFFF))
        {
            chanmode = val;
        }
        else
        {
            puts("Valid values are 0x0000 to 0xFFFF for 16 channels");
            pause();
            return;
        }
    }

    // Write active channels
    pHostRegs->iv_mode=chanmode;
    pause();
    return;
}

static void ChangeSampRate(void)
{
 //   bool valid;

    // Get number of active channels
    ActChanCnt = GetActChanCnt();
    if (ActChanCnt == 0)
    {
        puts("No channels Active so exiting");
        pause();
        return;
    }

    // Get new sampling rate from user
    GetSampRate();

    // Configure PLL with new sampling rate
   // valid = NewPLLParameters(chanRate, ActChanCnt);
/*    if(valid == FALSE)
    {
        puts("Unsuccessful configuration!");
        pause();
        return;
    }
    else
    {
        puts("New Sampling rate configured");
        pause();
        return;
    }
*/
}

static void GetSampRate(void)
{

    char        *inbuf;
    uint32_t    val;


    // Get the amount of active channels that will be used
    //  Count requires that the total is the same between each bank
    ActChanCnt = GetActChanCnt();
    printf("The full active channel count is %d \n", ActChanCnt);
    
    // Abort function if no channels are active
    if(ActChanCnt==0)
    {
        puts("No channels active, not recalculating.");
        pause();
        return;
    }

    //Get sampling rate
    printf("Enter the sampling rate per channel in Hz [%ld]: ", chanRate);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%ld", &val) == 1) && (val>=1) && (val<=62500))
        {
            chanRate = val;
        }
        else
        {
            puts("Valid values are 1 to 62500 Hz");
            pause();
            return;
        }
    }
}

#ifdef RobsOldCode
static bool NewPLLParameters(uint32_t chanRate, uint32_t ActChanCnt)
{
    bool valid;
    double Fvco_D, pllRate, adRate;
    uint32_t N_var;

    // Calculate the A/D sampling rate based on the number of channels and rate per channel
    adRate = chanRate * ActChanCnt; // Channel rate x number of most active channels

    // Get number of decimation stages needed
    if (adRate<250000)
        decimatorsCnt = ceil(log2(250000/adRate));
    else
        decimatorsCnt = 0;

    // Display and write data
    printf("\nNumber of decimation stages needed: %ld \n", decimatorsCnt);
    pHpsRegs->decimator_sel=decimatorsCnt;

    // Calculate PLL Rate
    pllRate = adRate*128*exp2(decimatorsCnt);

    // Loop to get PLL parameters
    for(Fvco_D=1000000000; Fvco_D>=300000000; Fvco_D-=10000000)
    {
        for(N_var=2; N_var<=15; N_var++)
        {
            valid = calcPllParameters(pllRate, Fvco_D, N_var);
            if (valid==TRUE)
                break;
        }
        if (valid==TRUE)
            break;
    }

    if (valid == TRUE)
    {
        // Now format each of the variables according to AN661 from Altera
        // (http://www.altera.com/literature/an/an661.pdf)
        // Then write them to FPGA and trigger a frequency reconfiguration
        char buffer[100];
        uint32_t tmpVar;

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if (M_var==1)
            sprintf(buffer, "00010001");
        else
        {
            if (M_var % 2) // if odd
                sprintf(buffer, "0002%02lX%02lX",(M_var-1)/2+1,(M_var-1)/2);
            else
                sprintf(buffer, "0000%02lX%02lX",(M_var)/2,(M_var)/2);
        }
        tmpVar = M_var;                                         // Store unformatted value
        M_var = strtoul(buffer, NULL, 16);                      // Copy over formatted value
        pHpsRegs->pll_m_var = M_var;                            // Write value to FPGA
        printf("   M_var = 0x%08lX (%lu) \n", M_var, tmpVar);   // Display value

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if (K_uint==1)
            sprintf(buffer, "00000001");
        else
            sprintf(buffer, "%08lX", K_uint);
        tmpVar = K_uint;                                        // Store unformatted value
        K_uint = strtoul(buffer, NULL, 16);                     // Copy over formatted value
        pHpsRegs->pll_k_var = K_uint;                           // Write value to FPGA
        printf("   K_var = 0x%08lX (%lu) \n", K_uint, tmpVar);  // Display value

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if (N_var==1)
            sprintf(buffer, "00010001");
        else
        {
            if (N_var % 2) // if odd
                sprintf(buffer, "0002%02lX%02lX",(N_var-1)/2+1,(N_var-1)/2);
            else
                sprintf(buffer, "0000%02lX%02lX",(N_var)/2,(N_var)/2);
        }
        tmpVar = N_var;                                         // Store unformatted value
        N_var = strtoul(buffer, NULL, 16);                      // Copy over formatted value
        pHpsRegs->pll_n_var = N_var;                            // Write value to FPGA
        printf("   N_var = 0x%08lX (%lu) \n", N_var, tmpVar);   // Display value

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if (C_var==1)
            printf(buffer, "00010001");
        else
        {
            if (C_var % 2) // if odd
                sprintf(buffer, "0002%02lX%02lX",(C_var-1)/2+1,(C_var-1)/2);
            else
                sprintf(buffer, "0000%02lX%02lX",(C_var)/2,(C_var)/2);
        }
        tmpVar = C_var;                                         // Store unformatted value
        C_var = strtoul(buffer, NULL, 16);                      // Copy over formatted value
        pHpsRegs->pll_c_var = C_var;                            // Write value to FPGA
        printf("   C_var = 0x%08lX (%lu) \n", C_var, tmpVar);   // Display value

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        printf("PLL Fout = %.0f Hz\n", Fout);                   // Display PLL output frequency

        // Start reconfiguration state machine in FPGA
        pHpsRegs->pll_start = 1;

        // Reset reconfiguration trigger
        pHpsRegs->pll_start = 0;

    }

    return valid;
}
#endif
/*
 * Get the amount of active channels that will be used
 * Count requires that the total is the same between each bank
 * i.e. if the active channel word is 0x0307 then three channels are
 * active in bank A and two channels are active in bank B.  Therefore the total number
 * of active channels will be the greater of the two banks which is 3 channels.
 */
static uint8_t GetActChanCnt(void)
{
    HOST_REGS  *pHostRegs = (HOST_REGS *)HOST_REGS_BASE;
    uint8_t     bitcnt[16]={0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
    uint8_t     tmpCnt1,tmpCnt2,tmpCnt3,tmpCnt4;

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
        ActChanCnt = tmpCnt2;                               //  and use whichever has the larger amount
    else
        ActChanCnt = tmpCnt4;

    return ActChanCnt;
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


static void ChangeDecimators(void)
{
    char        *inbuf;
    uint32_t    val;

    // Get the number of decimation stages desired by user
    printf("Enter Number of decimation states (0x0-0xF) [0x%01lX]: ", decimatorsCnt);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && (val>=0) && (val<=15))
            decimatorsCnt = val;
        else
        {
            puts("Valid values are 0 to 15");
            pause();
            return;
        }
    }

    // Write them to FPGA
    pHpsRegs->decimator_sel=decimatorsCnt;
    pause();
    return;
}


static void TestEnable(void)
{
    /*
     * This is for the D0, D2 and D3 test
     *  D0 is for background bit where the voltage of one channel at a time is checked.
     *  D2 is where the channels get hooked up to the on board DAC and the user generates a voltage.
     *  D3 is where the channels get hooked up to the on board DAC and it cycles through various voltages.
     */
    char        *inbuf;
    uint32_t    val;
    uint32_t    testmode=0;
    uint32_t    testval=0;
    uint8_t     exitTest=0;

    printf("Enter Test Mode (b[3]=D3, b[2]=D2, b[0]=D0) [0x%01lX]: ", testmode);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && ( (val==0) || (val==1) || (val==4) || (val==5) || (val==8) || (val==9) ) )
            testmode = val;
        else
        {
            puts("Test Mode Invalid!");
            pause();
            return;
        }
    }

    // Put DAC into configuration mode
    //mw.l 0xFF300004 0x00000003
    pHpsRegs->dac_rstn=0x3;
    puts("\nDAC configured continue to take out of reset");
    pause();

    // Put DAC into Normal Mode
    //mw.l 0xFF300004 0x00000001
    pHpsRegs->dac_rstn=0x1;
    puts("\nDAC now out of reset\n");

    while(exitTest == 0)
    {
        if(testmode&8) // If D3 test enabled
        {
            puts("<> Add code to cycle through voltages <>\n");
        }
        else
        {
            if(testmode&4) // If D2 test enabled
            {
                // Place DAC onto BIT signal
                pHpsRegs->bit_sel=0x2;

                // Now place BIT signal into A/D Channels (this will connect DAC to those channels)
                pHpsRegs->connect_dac=0xFFFF;

                // Request Range and Voltage and write it
                // (will eventually get it from user!)
                printf("Choose a range 0-3 [%d]: ", 0);
                inbuf = pend_rx_line();

                if (!inbuf)
                {   pause();
                    return;
                }

                if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && (val<4) )
                    testval = val;
                else
                    puts("Invalid value so using default!");

                pHpsRegs->bit_gain=testval;

                // Reinitialize variable and get DAC value
                testval = 0;
                printf("Input a 16 bit DAC value [%d]: ", 0);
                inbuf = pend_rx_line();

                if (!inbuf)
                {   pause();
                    return;
                }
                if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) )
                    testval = val;
                else
                    puts("Invalid value so writing 0 to DAC");

                pHpsRegs->dac_data=val;

            }
            else
            {
                // Disconnect dac and gnd connections to re-enabled normal mode
                pHpsRegs->connect_dac=0x0000;
                pHpsRegs->connect_gnd=0x0000;
                puts("DAC and GND disconnected");
            }
        }

        // See if user wants to exit the testing mode or not
        printf("Exit Test? (0:no, 1:yes) [%d]: ", 0);
        inbuf = pend_rx_line();

        if (!inbuf)
        {
            pause();
            return;
        }
        if (strlen(inbuf) > 0)
        {
            if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && ( (val==0) || (val==1)))
                exitTest = (uint8_t)val;
            else
            {
                puts("Valid values are 0 or 1 - Exiting Test");
                pause();
                return;
            }
        }

    }

    return;
}

static void InitFifo(void)
{
    char        *inbuf;
    uint32_t    val;
    uint32_t    fifoCtrl=1;

    printf("Reset FIFO Controller? (0=No, 1=Yes) [0x%01lX]: ", fifoCtrl);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && ((val==0) || (val==1) ))
            fifoCtrl = val;
        else
        {
            puts("Invalid input...exiting");
            pause();
            return;
        }
    }

    if(fifoCtrl==0)
    {
        pause();
        return;
    }

    // Write starting address for FIFOs
    pHpsRegs->fifo_strtAddy[0]  = 0x10;
    pHpsRegs->fifo_strtAddy[1]  = 0x11;
    pHpsRegs->fifo_strtAddy[2]  = 0x12;
    pHpsRegs->fifo_strtAddy[3]  = 0x13;
    pHpsRegs->fifo_strtAddy[4]  = 0x14;
    pHpsRegs->fifo_strtAddy[5]  = 0x15;
    pHpsRegs->fifo_strtAddy[6]  = 0x16;
    pHpsRegs->fifo_strtAddy[7]  = 0x17;
    pHpsRegs->fifo_strtAddy[8]  = 0x18;
    pHpsRegs->fifo_strtAddy[9]  = 0x19;
    pHpsRegs->fifo_strtAddy[10] = 0x1A;
    pHpsRegs->fifo_strtAddy[11] = 0x1B;
    pHpsRegs->fifo_strtAddy[12] = 0x1C;
    pHpsRegs->fifo_strtAddy[13] = 0x1D;
    pHpsRegs->fifo_strtAddy[14] = 0x1E;
    pHpsRegs->fifo_strtAddy[15] = 0x1F;

    // Write max size
    for(int i=0; i<16; i++)
        pHpsRegs->fifo_fullCnt[i]=0x1FFFFF;

    // Reset and enable FIFO controller
    pHpsRegs->fifo_init.bits.Fiforst=1;
    pHpsRegs->fifo_init.bits.Fiforst=0;
    pHpsRegs->fifo_init.bits.enable=1;

    puts("FIFO Initialized");
    pause();
    return;
}

static void EnableFifo(void)
{
    char        *inbuf;
    uint32_t    val;
    uint32_t    fifoCtrl=1;

    printf("Enable FIFO? (0=No, 1=Yes) [0x%01lX]: ", fifoCtrl);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) &&  ((val==0) || (val==1) ))
            fifoCtrl = val;
        else
        {
            puts("Invalid input...exiting");
            pause();
            return;
        }
    }

    for(int i=0; i<NUM_CHANNELS; i++)
        pHostRegs->clearFifo[i]=1;

    if(fifoCtrl==0)
    {
        pHpsRegs->fifo_init.reg=1;
        puts("FIFO Disabled");
    }
    else
    {
        pHpsRegs->fifo_init.reg=5;
        puts("FIFO Enabled");
    }

    pause();
    return;

}

static void ReadFifo(void)
{
    int32_t val;
    int32_t     rval[NUM_CHANNELS];
    uint32_t    chanindex = 0;
    uint32_t    readagain = 1;
    char        *inbuf;

    /* Get channel number */
    printf("\nEnter Channel (1-%d) [%lu]: ", NUM_CHANNELS, chanindex);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%ld", &val) == 1) && (val>=0) && (val<=NUM_CHANNELS))
        {
            chanindex = val;
        }
        else
        {
            puts("Valid values are 0x1 to 0x11");
            pause();
            return;
        }
    }

    // Create loop that will keep reading until user wants to exit
    while(readagain==1)
    {
        if(chanindex == 0)                                          // See if user wants to read all channels
        {
            int i;
            for(i=0;i<NUM_CHANNELS;i++)                                       // Loop through each channel
            {
                rval[i] = pHostRegs->fifodata[i];                   // Store values to array
                printf("Channel %2d : %ld\n", (i+1), rval[i]);      // Display all the channels data
            }
        }
        else                                                        // Single channel read
        {
            rval[0] = pHostRegs->fifodata[chanindex-1];         // Store read value into first array index
            printf("The FIFO value of Channel %ld is:%08lX\n", chanindex, rval[0]); // Display it
        }

        // See if user wants to repeat the reading or exit
        printf("Read again? 1:yes, 0:no [%lu]: ", readagain);
        inbuf = pend_rx_line();
        if (!inbuf)
        {
            pause();
            return;
        }
        if (strlen(inbuf) > 0)
        {
            if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && (val>=0) && (val<=1))
            {
                readagain = val;
            }
            else
            {
                puts("Valid values are 0x0 or 0x1...exiting");
                pause();
                return;
            }
        }
    }
    return;

}

static void DumpFifo(void)
{
    int32_t     val;
    int32_t     rval;
    uint32_t    sampleCnt = 10;
    uint32_t    maxchan = 1;
    uint32_t    readagain = 1;
    char        *inbuf;

    /* Get number of channels */
    printf("\nEnter max channel to dump (1-16) [%ld]: ", maxchan);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%ld", &val) == 1) && (val>=0) && (val<=NUM_CHANNELS))
        {
            maxchan = val;
        }
        else
        {
            puts("Valid values are 1 to 16");
            pause();
            return;
        }
    }


    /* Get number of samples */
    printf("\nEnter Number of Samples (1-0x1FFFFF) [%ld]: ", sampleCnt);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%ld", &val) == 1) && (val>=0) && (val<=0x1FFFFF))
        {
            sampleCnt = val;
        }
        else
        {
            puts("Valid values are 0x1 to 0x1FFFFF");
            pause();
            return;
        }
    }

    // Create loop that will keep reading until user wants to exit
    while(readagain==1)
    {
        int chan;
        int i;
        for(chan=0; chan<maxchan; chan++)
        {
            printf("Data for Channel %d\n", chan+1);
            for(i=0; i<sampleCnt; i++)                                      // Loop through each channel
            {
                rval = pHostRegs->fifodata[chan];                   // Store values to array
                printf("%ld\n", rval);              // Display all the channels data
            }
        }


        // See if user wants to repeat the reading or exit
        printf("Read again? 1:yes, 0:no [%lu]: ", readagain);
        inbuf = pend_rx_line();
        if (!inbuf)
        {
            pause();
            return;
        }
        if (strlen(inbuf) > 0)
        {
            if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && (val>=0) && (val<=1))
            {
                readagain = val;
            }
            else
            {
                puts("Valid values are 0x0 or 0x1...exiting");
                pause();
                return;
            }
        }
    }
    return;

}


static void ADBuffer(void)
{
    int32_t     val;
    uint32_t    sampleCnt = 10;
    uint32_t    dmpchan = 1;
    uint32_t    readagain = 1;
    char        *inbuf;

    /* Get number of channels */
    printf("\nEnter channel to dump (1-16) [%ld]: ", dmpchan);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%ld", &val) == 1) && (val>=0) && (val<=NUM_CHANNELS))
        {
            dmpchan = val;
        }
        else
        {
            puts("Valid values are 1 to 16");
            pause();
            return;
        }
    }


    /* Get number of samples */
    printf("\nEnter Number of Samples (1-0x1FFFFF) [%ld]: ", sampleCnt);
    inbuf = pend_rx_line();

    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf) > 0)
    {
        if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%ld", &val) == 1) && (val>=0) && (val<=0x1FFFFF))
        {
            sampleCnt = val;
        }
        else
        {
            puts("Valid values are 0x1 to 0x1FFFFF");
            pause();
            return;
        }
    }

    // Create loop that will keep reading until user wants to exit
    while(readagain==1)
    {
        int i;
        int32_t     rval[sampleCnt];

        for(i=0; i<(sampleCnt+1); i++)                      // Loop through each sample
        {
            rval[i] = pHpsRegs->ad_reading[dmpchan-1];          // Store values to array
            usleep(100);                                    // delay
        }

        printf("Data for Channel %ld\n", dmpchan);
        for(i=0; i<(sampleCnt+1); i++)                      // Loop through each sample
            printf("%ld\n", rval[i]);                       // Display all the channels data



        // See if user wants to repeat the reading or exit
        printf("Read again? 1:yes, 0:no [%lu]: ", readagain);
        inbuf = pend_rx_line();
        if (!inbuf)
        {
            pause();
            return;
        }
        if (strlen(inbuf) > 0)
        {
            if (ishex(inbuf, sizeof(uint32_t)) && (sscanf(inbuf, "%lx", &val) == 1) && (val>=0) && (val<=1))
            {
                readagain = val;
            }
            else
            {
                puts("Valid values are 0x0 or 0x1...exiting");
                pause();
                return;
            }
        }
    }
    return;

}

#endif // #if SERIAL_DEBUG

