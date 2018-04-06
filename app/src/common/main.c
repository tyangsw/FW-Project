/*
 * Firmware for NAI Maximillion modules.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

/*
 * @@@ TODO:
 *  - restructure Makefiles to handle module subfolders
 *  - rewrite temp_read() to use non-blocking state machine?
 *  - remove verify_bsp_patches() when we move to SDK 2015.1 (standalone_v5_0 BSP)
 *  -
 */

#include <includes.h>


/* Local functions */
static void init(void);
static void late_init(void);
static void verify_bsp_patches(void);


int main(int argc, char** argv)
{
    u32 mask = 0;

    /* Tell the world we are running */
    pCommon->mod_ready |= MOD_READY_FW_ENTRY;

    /* Verify BSP patches */
    verify_bsp_patches();

    /* Common initialization code */
    init();

    /* Call module "init" function */
    module_init();

    /* Common initialization code, part 2 (post module specific initialization) */
    late_init();

    /* Module is fully initialized, if all required actions have been performed */
    if (PARAM_FILE_REQUIRED)
        mask |= MOD_READY_PARAM_LOADED;
    if (CAL_FILE_REQUIRED)
        mask |= MOD_READY_CAL_LOADED;
    if (CPLD1_IMPLEMENTED)
        mask |= MOD_READY_CPLD1_PROGRAMMED;
    if (CPLD2_IMPLEMENTED)
        mask |= MOD_READY_CPLD2_PROGRAMMED;
    if ((pCommon->mod_ready & mask) == mask)
        pCommon->mod_ready |= MOD_READY_INIT_DONE;

    /* Main loop */
    while (1)
    {
#if SERIAL_DEBUG
        static bool debug = FALSE;

        /* Go to test menu on <ESC> */
        if (getchar() == ESCAPE)
            debug = TRUE;

        if (debug)
        {
            /* Stop watchdog timer */
            stop_watchdog();

            /* Enter test menu */
            test_menu();
            debug = FALSE;

            /* Restart watchdog timer */
            start_watchdog();
        }
#endif
        /* Read voltage and temperature */
        volt_temp_read();

        /* Call module "main" function */
        module_main();

        /* Kick the dog */
        watchdog_reset();
    }

    /* Should never get here!!! */
    exit(EXIT_FAILURE);
}


static void init(void)
{
    /* Start watchdog timer */
    start_watchdog();

    /* Initialize GIC */
    init_interrupts();

#if SERIAL_DEBUG
    /* Open stdout/stdin */
    usleep(1000);
    freopen("/dev/ttyS0", "w", stdout);
    freopen("/dev/ttyS0", "r", stdin);

    /* Disable buffering on stdout/stdin, which is enabled by default by CSLIBC */
    setvbuf(stdout, (char *)NULL, _IONBF, 0);
    setvbuf(stdin, (char *)NULL, _IONBF, 0);

    /* Say Hello */
    puts("\n\n************************************");
    printf("* %3s Module Firmware, v%-10s *\n", MODULE_NAME, FW_VERSION);
    printf("* Built on %11s at %8s *\n", FW_DATE, FW_TIME);
    puts("************************************");
#endif

    /* Setup assert call back to get some info if we assert */
    Xil_AssertSetCallback(assert_print);

    /* Make M_AXI_GP1 area cacheable: S=b1 TEX=b101 AP=b11, Domain=b1111, C=b0, B=b1 */
    Xil_SetTlbAttributes(SCRATCH_MEM_BASE, 0x15de6);

    /* Initialize I2C bus */
    i2c_init();

    /* Initialize functional module temp sensor */
    func_temp_init();

    /* Initialize interface module voltage and temperature monitor */
    if_temp_volt_init();

    /* Initialize QSPI flash */
    qspi_init();

    /* Initialize module common area */
    init_common();

    /* Load parameter file */
    load_param_file();

    /* Load calibration file */
    load_cal_file();

    /* Program CPLDs */
    if (CPLD1_IMPLEMENTED && !CPLD1_PROGRAM_LATE)
        program_cpld(CPLD_1);
    if (CPLD2_IMPLEMENTED && !CPLD2_PROGRAM_LATE)
        program_cpld(CPLD_2);
}


static void late_init(void)
{
    /* Program CPLDs */
    if (CPLD1_IMPLEMENTED && CPLD1_PROGRAM_LATE)
        program_cpld(CPLD_1);
    if (CPLD2_IMPLEMENTED && CPLD2_PROGRAM_LATE)
        program_cpld(CPLD_2);
}


/* Dummy code to ensure we are linking with patched BSP */
extern u32 __boot_S_patch;
extern u32 __cpu_init_S_patch;
static void verify_bsp_patches(void)
{
    __boot_S_patch     = 1;
    __cpu_init_S_patch = 1;
}

