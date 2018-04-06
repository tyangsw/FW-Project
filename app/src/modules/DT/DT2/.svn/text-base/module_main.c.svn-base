/*
 * DT2 module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Global variables */
static bool mod_init = FALSE;

static SYS_REGS  *pSysRegs = (SYS_REGS *)SYS_REGS_BASE;
static CH_REGS   *pChRegs[NUM_CHANNELS];
static HPS_REGS  *pHpsRegs = (HPS_REGS *)HPS_REGS_BASE;
static CAL_DATA  *pCalData = (CAL_DATA *)MODULE_CAL_BASE;
static ID_REGS   *pIdRegs = (ID_REGS *)ID_REGS_BASE;

/* Local functions */
static void init(void);
static void load_params(void);
static void load_caldata(void);


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
    }
}

/****************************** Local functions *****************************/

static void init(void)
{
    CHANNEL ch;

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
    pIdRegs->id = atol(MODULE_NAME + 2);

    /* Calculate OC gain */
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        pCalData->oc_gain[ch] = OC_GAIN_CONST / pCalData->current_gain[ch];
    }

    /* Reset Over-Current */
    pSysRegs->oc_reset = 1;

    /* Set baud rate */
    pHpsRegs->baud = 0x189374BC;

    /* Enable power supplies */
    pHpsRegs->ps_en = 0xFFFF;
    usleep(10000);  // Wait 10 ms

    /* Serial Control = Tx Go */
    pHpsRegs->ser_ctrl = 0x1;

    /* Module is now initialized */
    mod_init = TRUE;
}


static void load_params(void)
{
    CHANNEL ch;

    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */
        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            pChRegs[ch]->debounce_time  = 0x4;
            pChRegs[ch]->max_hi_thr     = 0x64;
            pChRegs[ch]->upper_thr      = 0x32;
            pChRegs[ch]->lower_thr      = 0x1E;
            pChRegs[ch]->min_lo_thr     = 0x0;
            pChRegs[ch]->oc_value       = 0x138;
        }
    }
}


static void load_caldata(void)
{
    CHANNEL ch;

    if (!(pCommon->mod_ready & MOD_READY_CAL_LOADED))
    {
        /* Use default values */
        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            pCalData->volt_gain[ch]     = 0x4000;
            pCalData->current_gain[ch]  = 0x4000;
        }
    }
}

