/*
 * Interrupt support functions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */
 
#include <includes.h>


/* Global variables */
XScuGic GicInstance;

/* Local functions */
static void exception_handler(char *type);
static void undef_exception_handler(void *data);
static void swi_exception_handler(void *data);
static void pabort_exception_handler(void *data);
static void dabort_exception_handler(void *data);
static void fiq_exception_handler(void *data);


XStatus init_interrupts(void)
{
    XScuGic_Config  *cfg = NULL;
    XStatus         rc;

    /* Initialize exception table */
    Xil_ExceptionInit();

    /* Register exception handlers */
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_UNDEFINED_INT, undef_exception_handler, NULL);
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_SWI_INT, swi_exception_handler, NULL);
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_PREFETCH_ABORT_INT, pabort_exception_handler, NULL);
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_DATA_ABORT_INT, dabort_exception_handler, NULL);
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FIQ_INT, fiq_exception_handler, NULL);

    /* Lookup GIC configuration */
    cfg = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
    if (!cfg)
        return XST_FAILURE;

    /* Initialize GIC driver */
    memset(&GicInstance, 0, sizeof(GicInstance));
    rc = XScuGic_CfgInitialize(&GicInstance, cfg, cfg->CpuBaseAddress);
    if (rc)
        return rc;

    /* Register master IRQ handler */
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, &GicInstance);

    /* Enable interrupts in the ARM */
    Xil_ExceptionEnable();
    
    return rc;
}


void enable_interrupts(bool enable)
{
    u32 temp;

    if (enable)
    {
        __asm(
            "mrs %0, cpsr\n"
            "bic %0, %0, #0x80\n"
            "msr cpsr_c, %0"
            : "=r" (temp)
            :
            : "memory"
        );
    }
}


bool disable_interrupts(void)
{
    u32 old, temp;

    __asm(
        "mrs %0, cpsr\n"
        "orr %1, %0, #0xc0\n"
        "msr cpsr_c, %1"
        : "=r" (old), "=r" (temp)
        :
        : "memory"
    );

    /* Returns the state before disabling */
    return ((old & 0x80) == 0);
}


XStatus irq_register(u32 int_id, Xil_InterruptHandler handler, void *context)
{
    XStatus rc;

    /* Register device IRQ handler */
    rc = XScuGic_Connect(&GicInstance, int_id, handler, context);
    if (rc)
        return rc;

    /* Enable device interrupt */
    XScuGic_Enable(&GicInstance, int_id);

    return rc;
}

/**************************** Exception handlers ****************************/

static void exception_handler(char *type)
{
    printf("\nUnhandled exception! (%s)\n", type);
    cpu_reset();
}


static void undef_exception_handler(void *data)
{
    exception_handler("UNDEF");
}


static void swi_exception_handler(void *data)
{
    exception_handler("SWI");
}


static void pabort_exception_handler(void *data)
{
    exception_handler("PABT");
}


static void dabort_exception_handler(void *data)
{
    exception_handler("DABT");
}


static void fiq_exception_handler(void *data)
{
    exception_handler("FIQ");
}

