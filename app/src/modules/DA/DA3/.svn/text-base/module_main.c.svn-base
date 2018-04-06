/*
 * DA3 module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Global variables */
static bool mod_init = FALSE;

//@@@ static SYS_REGS  *pSysRegs = (SYS_REGS *)SYS_REGS_BASE;
static CH_REGS   *pChRegs[NUM_CHANNELS];
static HPS_REGS  *pHpsRegs = (HPS_REGS *)HPS_REGS_BASE;
//@@@ static CAL_DATA  *pCalData = (CAL_DATA *)MODULE_CAL_BASE;
//@@@ static ID_REGS   *pIdRegs = (ID_REGS *)ID_REGS_BASE;

/* Local functions */
static void init(void);
static void load_params(void);
static void load_caldata(void);
static XStatus update_ps_dac(void);
static XStatus check_12v(void);


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
        /* Check for calibration data updates */
        caldata_rw();

        //@@@ code main loop (per channel?)
    }
}

/****************************** Local functions *****************************/

static void init(void)
{
    CHANNEL ch;
    bool    ramping = TRUE;
    u32     data, target, delta;

    /* Initialize global pointers to registers */
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        pChRegs[ch] = (CH_REGS *)CH_REGS_BASE + ch;
    }

    /* Load calibration data */
    load_caldata();

    /* Load configuration parameters */
    load_params();

    /* Tell FPGA which 'type' we are */
    //@@@ pIdRegs->id = atol(MODULE_NAME + 2);

    /* Set initial power supply DAC values */
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        pHpsRegs->ps_dac_data[ch] = (ch << 14) | (1 << 12) | 0xD86;
    }

    /* Update power supply DAC */
    if (update_ps_dac())
        return;

    /* Check 12V power good */
    if (check_12v())
        return;

    //@@@ turn below code into function and pass target?; can be called from main loop? (unless per channel)
    target = 0x9A5;
    while (ramping) //@@@ ramping flag per channel? target per channel?
    {
        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            data = pHpsRegs->ps_dac_data[ch] & 0xFFF;
            if (target < data)
            {
                /* Ramping down */
                delta = data - target;
                if (delta > 8)
                    delta = 8;
                pHpsRegs->ps_dac_data[ch] = (ch << 14) | (1 << 12) | (data - delta);
            }
            else if (target > data)
            {
                /* Ramping up */
                delta = target - data;
                if (delta > 8)
                    delta = 8;
                pHpsRegs->ps_dac_data[ch] = (ch << 14) | (1 << 12) | (data + delta);
            }
            else
            {
                /* Done ramping */
                pHpsRegs->ps_dac_data[ch] = (ch << 14) | data;
                ramping = FALSE;
            }
        }

        /* Update power supply DAC */
        if (update_ps_dac())
            return;
    }

    /* Enable power supplies */
    pHpsRegs->ps_en = 0xF;

    /* Module is now initialized */
    mod_init = TRUE;
}


static void load_params(void)
{
    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */

        /*
         * Set module specific defaults here...
         */
    }
}


static void load_caldata(void)
{
    if (!(pCommon->mod_ready & MOD_READY_CAL_LOADED))
    {
        /* Use default values */

        /*
         * Set module specific defaults here...
         */
    }
}


static XStatus update_ps_dac(void)
{
    XTime time = get_timer(0);

    /* Update power supply DAC */
    pHpsRegs->ps_dac_update = 1;

    while (pHpsRegs->ps_dac_update == 1)
    {
        /* Evaluate timeout */
        if (get_timer(time) > 10)   //@@@ timeout
            return XST_FAILURE;
    }

    return XST_SUCCESS;
}


static XStatus check_12v(void)
{
    XTime time = get_timer(0);

    /* Check 12V power good */
    while (pHpsRegs->power_ok != 0x31325647);
    {
        /* Evaluate timeout */
        if (get_timer(time) > 10)   //@@@ timeout
            return XST_FAILURE;
    }

    return XST_SUCCESS;
}

