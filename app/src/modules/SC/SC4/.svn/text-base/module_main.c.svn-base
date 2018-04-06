/*
 * SC4 module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Global variables */
static bool ch_init[NUM_CHANNELS] = { FALSE };

/* Local functions */
static void init(CHANNEL ch);
static int bist(CHANNEL ch);
static void load_params(CHANNEL ch);


/**************************** Exported functions ****************************/

void module_init(void)
{
    CHANNEL ch;
    
    /* Initialize all channels */
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        init(ch);
    }
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

static void init(CHANNEL ch)
{
    /*
     * Insert module specific initialization code here...
     */

    /* Run BIST */
    if (bist(ch) == 0)
    {
        /* Load configuration parameters */
        load_params(ch);

        /* Channel is now initialized */
        ch_init[ch] = TRUE;
    }
}


static int bist(CHANNEL ch)
{
    /*
     * Implement module specific BIST code here...
     */

    return 0;
}


static void load_params(CHANNEL ch)
{
    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */

        /*
         * Set module specific defaults here...
         */
    }
}

