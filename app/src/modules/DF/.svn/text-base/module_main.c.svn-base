/*
 * DFx module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Global variables */
static bool mod_init = FALSE;

static SYS_REGS *pSysRegs = (SYS_REGS *)SYS_REGS_BASE;
static CH_REGS  *pChRegs[NUM_CHANNELS];
#if ! MODULE_DF3
static ID_REGS  *pIdRegs = (ID_REGS *)ID_REGS_BASE;
#endif

/* Local functions */
static void init(void);
static void load_params(void);


/**************************** Exported functions ****************************/

void module_init(void)
{
    /* Initialize module */
    init();
}


void module_main(void)
{
    CHANNEL ch;
    
    if (mod_init)
    {
        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            /*
             * Insert module specific functional code here...
             */
        }
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

    /* Load configuration parameters */
    load_params();

    /* Clear Overcurrent condition */
    pSysRegs->oc_reset = 0xFFFF;

#if ! MODULE_DF3
    /* Clear BIT condition */
    pSysRegs->bit_clear = 0xFFFF;

    /* Tell FPGA which 'type' we are */
    pIdRegs->id = atol(MODULE_NAME + 2);
#endif

    /* Module is now initialized */
    mod_init = TRUE;
}


static void load_params(void)
{
    CHANNEL ch;

    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */
        pSysRegs->trx_dir              = 0x0;
        pSysRegs->write_out            = 0x0;
        pSysRegs->slew_rate            = 0xFFFF;
        pSysRegs->termination          = 0x0;
        pSysRegs->bit_err_irq_interval = 0x2000;
#if ! MODULE_DF3
        pSysRegs->bit_inversion        = 0x0;
#endif

        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            pChRegs[ch]->period_debounce = 0x8;
        }
    }
}

