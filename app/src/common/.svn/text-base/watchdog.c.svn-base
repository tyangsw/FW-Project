/*
 * Watchdog support functions targeting AWDT0.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */
 
#include <includes.h>


/* Global variables */
static XScuWdt WdtInstance;


XStatus start_watchdog(void)
{
    XScuWdt_Config  *cfg = NULL;
    XStatus         rc;
    static bool started = FALSE;

    if (!started)
    {
        /* Lookup watchdog configuration */
        cfg = XScuWdt_LookupConfig(XPAR_SCUWDT_0_DEVICE_ID);
        if (!cfg)
            return XST_FAILURE;

        /* Initialize watchdog driver */
        memset(&WdtInstance, 0, sizeof(WdtInstance));
        rc = XScuWdt_CfgInitialize(&WdtInstance, cfg, cfg->BaseAddr);
        if (rc)
            return rc;

        /* Initialized */
        started = TRUE;
    }

    /* Set watchdog mode */
    XScuWdt_SetWdMode(&WdtInstance);

    /* Load counter register - 2 seconds */
    XScuWdt_LoadWdt(&WdtInstance, WDT_LOAD_VALUE);

    /* Start it */
    XScuWdt_Start(&WdtInstance);

    /* Kick it */
    XScuWdt_RestartWdt(&WdtInstance);

    return rc;
}


void stop_watchdog(void)
{
    /* Set timer mode to disable watchdog */
    XScuWdt_SetTimerMode(&WdtInstance);

    /* Stop it */
    XScuWdt_Stop(&WdtInstance);
}


void watchdog_reset(void)
{
    XScuWdt_RestartWdt(&WdtInstance);
}

