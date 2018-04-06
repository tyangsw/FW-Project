/*
 * RTx module test menu.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#if SERIAL_DEBUG

#include <includes.h>


/* HOST register names */
#define HOST_NUM_REGS     10
char *host_regs[HOST_NUM_REGS] = {
    "res",
    "temp",
    "temp_f",
    "res0",
    "mode",
    "res_comp",
    "low1_thr",
    "low2_thr",
    "high1_thr",
    "high2_thr"
};

/* HPS register names */
#define HPS_NUM_ADC_REGS    15
char *ch_regs[HPS_NUM_ADC_REGS] = {
    "mux0",
    "vbias",
    "mux1",
    "sys0",
    "ofc0",
    "ofc1",
    "ofc2",
    "fsc0",
    "fsc1",
    "fsc2",
    "idac0",
    "idac1",
    "gpiocfg",
    "gpiodir",
    "gpiodat",
};

#define STR_RT_HOST_REG     "  %p: %-11s= %lu\n"
#define STR_RT_HOST_REG_F   "  %p: %-11s= 0x%08lX (%.3f)\n"
#define STR_RT_CH_REG       "  %p: %-11s= 0x%02lX\n"
#define STR_RT_CAL_DATA     "%p: Ch%d, %d-wire, %-7s: %-7s= 0x%08lX (%.4f)\n"

/* Local functions */
static void reg_dump(void);
static void rtd_mon(void);
static void adc_mon(void);
static void cal_dump(void);
static void bit_test(void);


/**************************** Exported functions ****************************/

void module_menu(void)
{
    bool    exit_menu = FALSE;
    char    *inbuf;

    /* Print menu choices */
    while (exit_menu == FALSE)
    {
        printf("\n\n>>>  %s Menu  <<<\n\n", MODULE_NAME);
        puts("1. Reg Dump");
        puts("2. RTD Monitor");
        puts("3. ADC Monitor");
        puts("4. Dump Cal Data");
        puts("5. BIT Test");
        puts("------------------");
        puts("M. Main menu");
        printf("\nEnter Selection: ");
        inbuf = pend_rx_line();
        if (!inbuf || (strlen(inbuf) > 1))
            continue;

        switch (tolower((int)*inbuf))
        {
            case '1':
                reg_dump();
                break;
            case '2':
                rtd_mon();
                break;
            case '3':
                adc_mon();
                break;
            case '4':
                cal_dump();
                break;
            case '5':
                bit_test();
                break;
            case 'm':
                exit_menu = TRUE;
                break;
        }
    }
}

/****************************** Local functions *****************************/

static void reg_dump(void)
{
    static CHANNEL  ch = CHANNEL_1;
    u32     val;
    char    *inbuf;
    u32     addr, i;

    /* Get channel number */
    printf("\nChannel (1-%d) [%d]: ", NUM_CHANNELS, ch + 1);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && (val >= 1) && (val <= NUM_CHANNELS))
        {
            ch = val - 1;
        }
        else
        {
            printf("Valid values are 1 to %d\n", NUM_CHANNELS);
            pause();
            return;
        }
    }

    printf("\nCh %d HOST registers:\n", ch + 1);
    addr = (u32)&pHostRegs[ch]->res;
    for (i = 0; i < HOST_NUM_REGS; ++i)
    {
        if (i == 3 || i == 4)
            printf(STR_RT_HOST_REG, (void *)addr, host_regs[i], Xil_In32(addr));
        else
            printf(STR_RT_HOST_REG_F, (void *)addr, host_regs[i], Xil_In32(addr), *(float *)addr);

        addr += 4;
    }

    printf("\nCh %d ADC registers:\n", ch + 1);
    addr = (u32)&pChRegs[ch]->adc;
    for (i = 0; i < HPS_NUM_ADC_REGS; ++i)
    {
        if (adc_read_reg(ch, (volatile u32 *)addr, &val))
        {
            pause();
            return;
        }
        printf(STR_RT_CH_REG, (void *)addr, ch_regs[i], val);
        addr += 4;
    }

    printf("\nCh %d CTRL registers:\n", ch + 1);
    printf(STR_RT_CH_REG, &pChRegs[ch]->ctrl.cmd, "cmd", pChRegs[ch]->ctrl.cmd);
    printf(STR_RT_CH_REG, &pChRegs[ch]->ctrl.spi_rdy, "spi_rdy", pChRegs[ch]->ctrl.spi_rdy);
    printf(STR_RT_CH_REG, &pChRegs[ch]->ctrl.adc_data, "adc_data", pChRegs[ch]->ctrl.adc_data);
    printf(STR_RT_CH_REG, &pChRegs[ch]->ctrl.adc_start, "adc_start", pChRegs[ch]->ctrl.adc_start);
    printf(STR_RT_CH_REG, &pChRegs[ch]->ctrl.check_drdy, "check_drdy", pChRegs[ch]->ctrl.check_drdy);

    pause();
}


static void rtd_mon(void)
{
    CHANNEL ch;

    puts("\nResistance/Temperature (Ohm/C):\n");

    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        printf("Channel %d ", ch + 1);
    }
    putchar('\n');
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        printf("--------- ");
    }
    putchar('\n');

    while (1)
    {
        /* Get ADC data from all channels */
        if (rtd_read())
        {
            pause();
            return;
        }

        /* Print it */
        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            printf("%4.0f/%4.0f ", pHostRegs[ch]->res, pHostRegs[ch]->temp);
        }
        putchar('\r');

        sleep(1);
        if (getchar() > 0)
            break;
    }
    putchar('\n');

    pause();
}


static void adc_mon(void)
{
    static CHANNEL  ch = CHANNEL_1;
    u32     val, data;
    char    *inbuf;
    float   temp, avdd, dvdd;

    /* Get channel number */
    printf("\nChannel (1-%d) [%d]: ", NUM_CHANNELS, ch + 1);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && (val >= 1) && (val <= NUM_CHANNELS))
        {
            ch = val - 1;
        }
        else
        {
            printf("Valid values are 1 to %d\n", NUM_CHANNELS);
            pause();
            return;
        }
    }

    puts("\n Temp AVDD DVDD");
    puts(" ---- ---- ----");

    while (1)
    {
        /* Read ADC internal temperature */
        if (adc_write_reg(ch, &pChRegs[ch]->adc.mux1, MUX1_TEMP_MODE))
        {
            pause();
            return;
        }
        if (adc_get_data(ch))
        {
            pause();
            return;
        }
        data = pChRegs[ch]->ctrl.adc_data;
        temp = (((data * CONST_ADC_CONV) - 0.118) / 405e-6) + 25;

        /* Read ADC internal AVDD */
        if (adc_write_reg(ch, &pChRegs[ch]->adc.mux1, MUX1_AVDD_MODE))
        {
            pause();
            return;
        }
        if (adc_get_data(ch))
        {
            pause();
            return;
        }
        data = pChRegs[ch]->ctrl.adc_data;
        avdd = data * 4 * CONST_ADC_CONV;

        /* Read ADC internal DVDD */
        if (adc_write_reg(ch, &pChRegs[ch]->adc.mux1, MUX1_DVDD_MODE))
        {
            pause();
            return;
        }
        if (adc_get_data(ch))
        {
            pause();
            return;
        }
        data = pChRegs[ch]->ctrl.adc_data;
        dvdd = data * 4 * CONST_ADC_CONV;

        printf("%4.0fC %1.1fV %1.1fV\r", temp, avdd, dvdd);

        sleep(1);
        if (getchar() > 0)
            break;
    }
    putchar('\n');

    /* Return to normal ADC operation */
    if (adc_write_reg(ch, &pChRegs[ch]->adc.mux1, MUX1_NORMAL_MODE))
    {
        pause();
        return;
    }
    adc_sync();

    pause();
}


static void cal_dump(void)
{
    CHANNEL ch;
    MODE    m;
    u32     addr;

    puts("\nCalibration data:\n");

    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        for (m = MODE_2_WIRE; m < NUM_MODES; ++m)
        {
            addr = (u32)&pCalData[ch]->pt100[m].offset;
            printf(STR_RT_CAL_DATA, (void *)addr, ch+1, m+2, "PT100", "offset", Xil_In32(addr), *(float *)addr);
            addr = (u32)&pCalData[ch]->pt100[m].gain;
            printf(STR_RT_CAL_DATA, (void *)addr, ch+1, m+2, "PT100", "gain", Xil_In32(addr), *(float *)addr);
            addr = (u32)&pCalData[ch]->pt100[m].temp;
            printf(STR_RT_CAL_DATA, (void *)addr, ch+1, m+2, "PT100", "temp", Xil_In32(addr), *(float *)addr);

            addr = (u32)&pCalData[ch]->pt500[m].offset;
            printf(STR_RT_CAL_DATA, (void *)addr, ch+1, m+2, "PT500", "offset", Xil_In32(addr), *(float *)addr);
            addr = (u32)&pCalData[ch]->pt500[m].gain;
            printf(STR_RT_CAL_DATA, (void *)addr, ch+1, m+2, "PT500", "gain", Xil_In32(addr), *(float *)addr);
            addr = (u32)&pCalData[ch]->pt500[m].temp;
            printf(STR_RT_CAL_DATA, (void *)addr, ch+1, m+2, "PT500", "temp", Xil_In32(addr), *(float *)addr);

            addr = (u32)&pCalData[ch]->pt1000[m].offset;
            printf(STR_RT_CAL_DATA, (void *)addr, ch+1, m+2, "PT1000", "offset", Xil_In32(addr), *(float *)addr);
            addr = (u32)&pCalData[ch]->pt1000[m].gain;
            printf(STR_RT_CAL_DATA, (void *)addr, ch+1, m+2, "PT1000", "gain", Xil_In32(addr), *(float *)addr);
            addr = (u32)&pCalData[ch]->pt1000[m].temp;
            printf(STR_RT_CAL_DATA, (void *)addr, ch+1, m+2, "PT1000", "temp", Xil_In32(addr), *(float *)addr);
        }
    }

    pause();
}


static void bit_test(void)
{
    bit_me(TRUE);

    pause();
}

#endif // #if SERIAL_DEBUG

