/*
 * RTx module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>

/* Exported global variables */
HOST_REGS *pHostRegs[NUM_CHANNELS];
SYS_REGS  *pSysRegs = (SYS_REGS *)SYS_REGS_BASE;
CTRL_REGS *pCtrlRegs = (CTRL_REGS *)CTRL_REGS_BASE;
CH_REGS   *pChRegs[NUM_CHANNELS];
CAL_DATA  *pCalData[NUM_CHANNELS];

/* Global variables */
static bool mod_init = FALSE;

static u8 res0[NUM_CHANNELS] = { 0 };
static u32 ohm[NUM_RES0] = { 100, 500, 1000 };

static u8 mode[NUM_CHANNELS] = { 0 };
static u8 gpio[NUM_MODES] = { GPIO_2_WIRE, GPIO_3_WIRE, GPIO_4_WIRE };
static u8 idac1[NUM_MODES] = { IDAC1_2_WIRE, IDAC1_3_WIRE, IDAC1_4_WIRE };

/* Local functions */
static void init(void);
static void load_params(CHANNEL ch);
static void load_caldata(CHANNEL ch);
static u8 get_pga(CHANNEL ch);
static float cal(CHANNEL ch, float res);
static void update_config(void);
static XStatus config_channel(CHANNEL ch, bool bit);
static XStatus adc_init(CHANNEL ch);
static XStatus spi_ready_wait(CHANNEL ch);
static XStatus spi_ready_wait_all(void);


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
        /* Run BIT on all channels */
        if (pSysRegs->bit_interval)
            bit_me(FALSE);

        /* Check for calibration data updates */
        caldata_rw();

        /* Check for configuration changes */
        update_config();

        /* Read ADC data from all channels */
        rtd_read();
    }
}


XStatus rtd_read(void)
{
    CHANNEL ch;
    s32     adcdata;
    float   res, temp;
    u32     low1_mask = 0;
    u32     low2_mask = 0;
    u32     high1_mask = 0;
    u32     high2_mask = 0;
    
    /* Get ADC data from all channels */
    if (adc_get_data_all())
        return XST_FAILURE;

    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ch++)
    {
        /* Read ADC data */
    	adcdata = pChRegs[ch]->ctrl.adc_data;

    	/*
    	 * Perform a sign extension on the 24-bit ADC value
    	 */
        if( (adcdata & 0x800000) != 0 )
        {
        	adcdata |= 0xFF000000;
        }

        /* Calculate resistance; Iexc = 100ua, Rref = 8k, divide by PGA */
        res = (adcdata * RESOLUTION_100UA) / (1 << get_pga(ch)) / IEXC_100UA;

        /* Apply calibration data */
        res = cal(ch, res);

        /* Apply 2-wire mode resistance compensation */
        if (pHostRegs[ch]->mode == MODE_2_WIRE)
            res -= pHostRegs[ch]->res_comp;

        /*
         * Don't try and calculate a negative resistance
         */
        if( res < 0.0f )
        {
        	res = 0.0f;
        }

        /* Update DPRAM */
        pHostRegs[ch]->res = res;

        /* Convert to temperature (from Analog Devices AN-709, pg. 4) */
        res /= ohm[res0[ch]] / 100.0f;

        /*
         * The formulas only work on a resistance range of 18.52008 <= res <= 390.4811
         * If outside this range clamp the temperature to -200 <= temp <= 850
         */
        if( res < 18.52008f )
        {
        	temp = -200.0f;
        }
        else if( res > 390.4811f )
        {
        	temp = 850.0f;
        }
        else
        {
			if (res >= 100.0f)
			{
				temp = 17.58480889e-6 + -23.10e-9 * res;

				/*
				 * Prevent us from doing a sqrt on a negative number ( Avoids NaN )
				 */
				if( temp < 0.0f )
				{
					temp = 0.0f;
				}
				temp = -3.9083e-3 + sqrtf(temp);
				temp /= -1.155e-6;
			}
			else
			{
				temp = -242.02e-0 + 2.2228e-0 * res;
				temp += 2.5859e-3 * res * res;
				temp -= 4.8260e-6 * res * res * res;
				temp -= 2.8183e-8 * res * res * res * res;
				temp += 1.5243e-10 * res * res * res * res * res;
			}
        }

        /* Update DPRAM */
        pHostRegs[ch]->temp = temp;
        pHostRegs[ch]->temp_f = (temp * 1.8) + 32.0f;

        /* Low 1 threshold reached? */
        if (temp <= pHostRegs[ch]->low1_thr)
            low1_mask |= (1 << ch);

        /* Low 2 threshold reached? */
        if (temp <= pHostRegs[ch]->low2_thr)
            low2_mask |= (1 << ch);

        /* High 1 threshold reached? */
        if (temp >= pHostRegs[ch]->high1_thr)
            high1_mask |= (1 << ch);

        /* High 2 threshold reached? */
        if (temp >= pHostRegs[ch]->high2_thr)
            high2_mask |= (1 << ch);
    }

    /* Write threshold status registers - generates interrupts to host */
    pCtrlRegs->low1  = low1_mask;
    pCtrlRegs->low2  = low2_mask;
    pCtrlRegs->high1 = high1_mask;
    pCtrlRegs->high2 = high2_mask;

    return XST_SUCCESS;
}


XStatus bit_me(bool verbose)
{
    CHANNEL ch;
    u32     adcdata;
    float   res;
    u32     bit_mask = 0;
    u32     oc_mask = 0;
    static XTime time;
    static bool  bit_time = TRUE;

    if (bit_time || verbose)
    {
        debug("\nBIT Test (100 Ohm +/- %.2f Ohm):\n\n", BIT_TOLERANCE);

        /* Reconfigure all channels for BIT */
        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            config_channel(ch, TRUE);
        }
        
        /* Sync all channels */
        adc_sync();

        /* Get ADC data from all channels */
        if (adc_get_data_all())
        {
            /* Treat as failure on all channels */
            bit_mask = CHANNEL_MASK;
        }
        else
        {
            for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
            {
                /* Read ADC data */
                adcdata = pChRegs[ch]->ctrl.adc_data;

                /* Calculate resistance; Iexc = 1000ua, Rref = 2k */
                res = (adcdata * RESOLUTION_1000UA) / IEXC_1000UA;

                /* BIT failed? */
                if (fabs(res - 100) > BIT_TOLERANCE)
                    bit_mask |= (1 << ch);

                debug("  Ch %d: %6.2f Ohm %s\n", ch + 1, res, (bit_mask & (1 << ch)) ? ": BIT FAILED! " : "");
            }
        }

        debug("\nOC Detection:\n\n");

        /* Reconfigure all channels for OC detection */
        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            config_channel(ch, FALSE);
            adc_write_reg(ch, &pChRegs[ch]->adc.mux0, MUX0_OC);
        }
        
        /* Sync all channels */
        adc_sync();

        /* Get ADC data from all channels */
        if (adc_get_data_all())
        {
            /* Treat as failure on all channels */
            oc_mask = CHANNEL_MASK;
        }
        else
        {
            for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
            {
                /* Read ADC data */
                adcdata = pChRegs[ch]->ctrl.adc_data;

                /* OC detected? */
                if (adcdata == 0x7FFFFF)
                    oc_mask |= (1 << ch);

                debug("  Ch %d: %s\n", ch + 1, (oc_mask & (1 << ch)) ? "OC DETECTED!" : "");
            }
        }

        /* Reconfigure all channels for normal operation */
        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            adc_write_reg(ch, &pChRegs[ch]->adc.mux0, MUX0_NORMAL_MODE);
        }

        /* Sync all channels */
        adc_sync();

        /* Write BIT and OC status registers - generates interrupts to host */
        pCtrlRegs->bit = bit_mask;
        pCtrlRegs->oc = oc_mask;

        /* Restart timer */
        bit_time = FALSE;
        time = get_timer(0);
    }

    /* Time to BIT? */
    if (get_timer(time) > pSysRegs->bit_interval)
        bit_time = TRUE;

    return XST_SUCCESS;
}


XStatus adc_write_reg(CHANNEL ch, volatile u32 *addr, u32 val)
{
    /* Wait until SPI is ready */
    if (spi_ready_wait(ch))
        return XST_FAILURE;

    /* Write it */
    Xil_Out32((u32)addr, val);

    return XST_SUCCESS;
}


XStatus adc_read_reg(CHANNEL ch, volatile u32 *addr, u32 *val)
{
    /* Wait until SPI is ready */
    if (spi_ready_wait(ch))
        return XST_FAILURE;

    /* Read it */
    *val = Xil_In32((u32)addr);

    return XST_SUCCESS;
}


XStatus adc_get_data(CHANNEL ch)
{
    /* Wait until SPI is ready */
    if (spi_ready_wait(ch))
        return XST_FAILURE;

    /* Set ADC chip select low to check DOUT/Drdyn, (CS self clears) */
    pChRegs[ch]->ctrl.check_drdy = 1;

    /* Must wait for Drdyn */
    if (spi_ready_wait(ch))
        return XST_FAILURE;

    return XST_SUCCESS;
}


XStatus adc_get_data_all(void)
{
    /* Wait until SPI is ready an all channels */
    if (spi_ready_wait_all())
        return XST_FAILURE;

    /* Set ADC chip select low to check DOUT/Drdyn, (CS self clears) */
    pCtrlRegs->check_drdy = 1;

    /* Must wait for Drdyn on all channels */
    if (spi_ready_wait_all())
        return XST_FAILURE;

    return XST_SUCCESS;
}


XStatus adc_sync(void)
{
    /* Wait until SPI is ready an all channels */
    if (spi_ready_wait_all())
        return XST_FAILURE;

    /* Synchronize A/D conversion on all channels */
    pCtrlRegs->cmd = CMD_SYNC;

    return XST_SUCCESS;
}


/****************************** Local functions *****************************/

static void init(void)
{
    CHANNEL ch;

    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        /* Initialize global pointers to registers */
        pHostRegs[ch] = (HOST_REGS *)HOST_REGS_BASE + ch;
        pChRegs[ch]   = (CH_REGS *)CH_REGS_BASE + ch;
        pCalData[ch]  = (CAL_DATA *)MODULE_CAL_BASE + ch;

        /* Load configuration parameters */
        load_params(ch);

        /* Load calibration data */
        load_caldata(ch);
    }

    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        /* Initialize ADC */
        if (adc_init(ch))
            return;
    }

    /* Verify we can get ADC data from all channels */
    if (adc_get_data_all())
        return;

    /* Sync all channels */
    if (adc_sync())
        return;

    /* Module is now initialized */
    mod_init = TRUE;
}


static void load_params(CHANNEL ch)
{
    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */
        pHostRegs[ch]->res0     = RES0_100_OHM;
        pHostRegs[ch]->mode     = MODE_3_WIRE;
        pHostRegs[ch]->res_comp = 0;
    }

    /* Save initial configuration */
    res0[ch] = pHostRegs[ch]->res0;
    mode[ch] = pHostRegs[ch]->mode;

    /* Default BIT interval = 20 sec */
    pSysRegs->bit_interval = 20000;
}


static void load_caldata(CHANNEL ch)
{
    MODE    m;

    if (!(pCommon->mod_ready & MOD_READY_CAL_LOADED))
    {
        /* Use default values */
        for (m = MODE_2_WIRE; m < NUM_MODES; ++m)
        {
            pCalData[ch]->pt100[m].offset  = 0;
            pCalData[ch]->pt100[m].gain    = 1;
            pCalData[ch]->pt100[m].temp    = 25;
            pCalData[ch]->pt500[m].offset  = 0;
            pCalData[ch]->pt500[m].gain    = 1;
            pCalData[ch]->pt500[m].temp    = 25;
            pCalData[ch]->pt1000[m].offset = 0;
            pCalData[ch]->pt1000[m].gain   = 1;
            pCalData[ch]->pt1000[m].temp   = 25;
        }
    }
}


static u8 get_pga(CHANNEL ch)
{
    u8  pga;

    /* PGA depends on nominal resistance and wire mode */
    switch (res0[ch])
    {
        case RES0_100_OHM:
            pga = (mode[ch] == MODE_2_WIRE) ? 3 : 4;
            break;
        case RES0_500_OHM:
            pga = 2;
            break;
        case RES0_1000_OHM:
            pga = (mode[ch] == MODE_2_WIRE) ? 0 : 1;
            break;
    }

    return pga;
}


static float cal(CHANNEL ch, float res)
{
    CALD    *pCal;
    float   temp;

    /* Get calibration data based on nominal resistance and wire mode */
    switch (res0[ch])
    {
        case RES0_100_OHM:
            pCal = (CALD *)&pCalData[ch]->pt100[mode[ch]];
            break;
        case RES0_500_OHM:
            pCal = (CALD *)&pCalData[ch]->pt500[mode[ch]];
            break;
        case RES0_1000_OHM:
            pCal = (CALD *)&pCalData[ch]->pt1000[mode[ch]];
            break;
    }

    /* Apply 2-wire mode temperature adjustment (0.5 Ohm per 1 degree C) */
    if (pHostRegs[ch]->mode == MODE_2_WIRE)
    {
        temp = pCal->temp - pCommon->func_mod_temp[FUNC_MOD_PCB_TEMP];
        res  += temp * 0.5;
    }

    /* Apply offset and gain */
    res -= pCal->offset;
    res *= pCal->gain;

    return res;
}


static void update_config(void)
{
    CHANNEL ch;
    bool    ch_cfg;
    bool    cfg = FALSE;

    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        ch_cfg = FALSE;

        /* New range? */
        if (res0[ch] != pHostRegs[ch]->res0)
        {
            if (pHostRegs[ch]->res0 < NUM_RES0)
            {
                /* Save new range */
                res0[ch] = pHostRegs[ch]->res0;
                ch_cfg = TRUE;
            }
        }

        /* New mode? */
        if (mode[ch] != pHostRegs[ch]->mode)
        {
            if (pHostRegs[ch]->mode < NUM_MODES)
            {
                /* Save new mode */
                mode[ch] = pHostRegs[ch]->mode;
                ch_cfg = TRUE;
            }
        }

        /* Reconfigure channel */
        if (ch_cfg)
        {
            config_channel(ch, FALSE);
            cfg = TRUE;
        }
    }

    /* Sync all channels if configuration changed */
    if (cfg)
        adc_sync();
}


static XStatus config_channel(CHANNEL ch, bool bit)
{
    u32 reg;

    /* Program GPIO outputs */
    reg = bit ? GPIO_BIT : gpio[mode[ch]];
    if (adc_write_reg(ch, &pChRegs[ch]->adc.gpiodat, reg))
        return XST_FAILURE;

    /* Program DAC outputs */
    reg = bit ? IDAC1_BIT : idac1[mode[ch]];
    if (adc_write_reg(ch, &pChRegs[ch]->adc.idac1, reg))
        return XST_FAILURE;

    /* Program PGA */
    reg = bit ? 0 : get_pga(ch) << 4;
    if (adc_write_reg(ch, &pChRegs[ch]->adc.sys0, reg))
        return XST_FAILURE;

    /* Program Drdy mode and excitation current */
    reg = bit ? IDAC0_BIT : IDAC0_NORMAL_MODE;
    if (adc_write_reg(ch, &pChRegs[ch]->adc.idac0, reg))
        return XST_FAILURE;

    /* Program differential inputs */
    reg = bit ? MUX0_BIT : MUX0_NORMAL_MODE;
    if (adc_write_reg(ch, &pChRegs[ch]->adc.mux0, reg))
        return XST_FAILURE;

    return XST_SUCCESS;
}


/*
 * Initialize ADC1248
 *
 * Internal reference activated for excitation current
 * Dout/Drdyn pin enabled for dual use
 * Gpio pins selected for Range select
 * Ain2 excitation current selected
 *
 * Contributed by Chris Forte
 */
static XStatus adc_init(CHANNEL ch)
{
    /* Take ADC out of sleep mode */
    if (adc_write_reg(ch, &pChRegs[ch]->ctrl.adc_start, 0x1))
        return XST_FAILURE;
    usleep(16000);  // START pin hi requires 16ms

    /* Enable GPIO7, GPIO1, and GPIO0 */
    if (adc_write_reg(ch, &pChRegs[ch]->adc.gpiocfg, 0x83))
        return XST_FAILURE;

    /* GPIO dir = outputs */
    if (adc_write_reg(ch, &pChRegs[ch]->adc.gpiodir, 0x00))
        return XST_FAILURE;

    /* Configure channel for normal operation */
    if (config_channel(ch, FALSE))
        return XST_FAILURE;

    /* 0x28 = Normal ADC operation, internal Vref on required for excitation current, ext ref1 for RTD measure */
    if (adc_write_reg(ch, &pChRegs[ch]->adc.mux1, MUX1_NORMAL_MODE))
        return XST_FAILURE;

    return XST_SUCCESS;
}


static XStatus spi_ready_wait(CHANNEL ch)
{
    XTime   time = get_timer(0);

    while ((pChRegs[ch]->ctrl.spi_rdy & 1) == 0)
    {
        /* Evaluate timeout */
        if (get_timer(time) > SPI_TIMEOUT)
        {
            printf("spi_ready_wait: ch%d timeout!\n", ch + 1); 
            return XST_FAILURE;
        }
    }

    return XST_SUCCESS;
}


static XStatus spi_ready_wait_all(void)
{
    XTime   time = get_timer(0);

    while ((pCtrlRegs->spi_rdy & CHANNEL_MASK) != CHANNEL_MASK)
    {
        /* Evaluate timeout */
        if (get_timer(time) > SPI_TIMEOUT)
        {
            printf("spi_ready_wait_all: timeout! (spi_rdy=0x%02lX)\n", pCtrlRegs->spi_rdy & 0xFF); 
            return XST_FAILURE;
        }
    }

    return XST_SUCCESS;
}

