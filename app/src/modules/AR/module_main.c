/*
 * ARx module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Global variables */
static bool ch_init[NUM_CHANNELS] = { FALSE };

static SYS_REGS *pSysRegs = (SYS_REGS *)SYS_REGS_BASE;
static CH_REGS  *pChRegs[NUM_CHANNELS];

/* Local functions */
static void init(void);
static void load_params(CHANNEL ch);


/**************************** Exported functions ****************************/

void module_init(void)
{
    /* Initialize module */
    init();
}


void module_main(void)
{
    CHANNEL ch;
    
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        if (ch_init[ch])
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

    /* Reset module (toggle bit 0) */
    pSysRegs->reset = 1;
    pSysRegs->reset = 0;

    /* Initialize all channels */
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        /* Load configuration parameters */
        load_params(ch);

        /* Channel is now initialized */
        ch_init[ch] = TRUE;
    }
}


static void load_params(CHANNEL ch)
{
    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */
        pChRegs[ch]->control        = 0x0;
        pChRegs[ch]->tx_fifo_rate   = 0x4;
        pChRegs[ch]->rx_fifo_thr    = 0x80;
        pChRegs[ch]->tx_fifo_thr    = 0x20;
    }
}

