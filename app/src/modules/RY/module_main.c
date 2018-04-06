/*
 * RYx module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Global variables */
static bool mod_init = FALSE;

static SYS_REGS *pSysRegs = (SYS_REGS *)SYS_REGS_BASE;
static ID_REGS  *pIdRegs  = (ID_REGS *)ID_REGS_BASE;

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
    /* Load configuration parameters */
    load_params();

    /* Tell FPGA which 'type' we are */
    pIdRegs->id = atol(MODULE_NAME + 2);

    /* Module is now initialized */
    mod_init = TRUE;
}


static void load_params(void)
{
    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */
        pSysRegs->force_bit_fail    = 0x0;
    }
}

