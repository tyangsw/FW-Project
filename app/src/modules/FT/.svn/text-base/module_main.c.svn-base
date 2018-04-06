/*
 * FTx module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Global variables */
static bool mod_init = FALSE;

static bool ch_impl[NUM_CHANNELS] = {
    /* Channel 1 is always implemented */
    TRUE,
    /* Channel 2 is only implemented on 4-channel versions */
#if MODULE_FT3 || MODULE_FT6
    TRUE,
#else
    FALSE,
#endif
    /* Channel 3 is only implemented on 2-channel and 4-channel versions */
#if MODULE_FT2 || MODULE_FT3 || MODULE_FT5 || MODULE_FT6 || MODULE_FT8
    TRUE,
#else
    FALSE,
#endif
    /* Channel 4 is only implemented on 4-channel versions */
#if MODULE_FT3 || MODULE_FT6
    TRUE
#else
    FALSE
#endif
};

static CH_REGS  *pChRegs[NUM_CHANNELS];
static CH_MEM   *pChMem[NUM_CHANNELS];
static HPS_REGS *pHpsRegs[NUM_CHANNELS];
static ID_REGS  *pIdRegs = (ID_REGS *)ID_REGS_BASE;

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
            if (ch_impl[ch])
            {
                //@@@ implement "cpu assisted mode"
            }
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
        pChRegs[ch] = (CH_REGS *)CHx_REGS_BASE(ch);
        pChMem[ch] = (CH_MEM *)CHx_MEM_BASE(ch);
        pHpsRegs[ch] = (HPS_REGS *)CHx_HPS_REGS_BASE(ch);
    }

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

        /*
         * Set module specific defaults here...
         */
    }
}

