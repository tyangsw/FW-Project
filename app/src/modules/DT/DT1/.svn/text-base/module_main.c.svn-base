/*
 * DT1 module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Global variables */
static bool mod_init = FALSE;

static SYS_REGS  *pSysRegs = (SYS_REGS *)SYS_REGS_BASE;
static CH_REGS   *pChRegs[NUM_CHANNELS];
static BANK_REGS *pBankRegs[NUM_BANKS];
static CAL_DATA  *pCalData = (CAL_DATA *)MODULE_CAL_BASE;
static HPS_REGS  *pHpsRegs = (HPS_REGS *)HPS_REGS_BASE;
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
    BANK    bank;

    /* Initialize global pointers to registers */
    for (bank = BANK_A; bank < NUM_BANKS; ++bank)
    {
        pBankRegs[bank] = (BANK_REGS *)BANK_REGS_BASE + bank;
    }
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        pChRegs[ch] = (CH_REGS *)CH_REGS_BASE + ch;
    }

    /* Load configuration parameters */
    load_params();

    /* Load calibration data */
    load_caldata();

    /* Tell FPGA which 'type' we are */
    pIdRegs->id = atol(MODULE_NAME + 2);

    /* Enable CPLD */
    pHpsRegs->cpld_en = 1;

    /* Module is now initialized */
    mod_init = TRUE;
}


static void load_params(void)
{
    CHANNEL ch;
    BANK    bank;

    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */
        pSysRegs->polarity              = 0x0;
        pSysRegs->start_patt_out        = 0x0;
        pSysRegs->master_sel            = 0x0;
        pSysRegs->bit_err_irq_interval  = 0x0;
        pSysRegs->write_out             = 0x0;
        pSysRegs->ram_period            = 0x30D4;
        pSysRegs->io_mode_en            = 0x0;
        pSysRegs->io_format_lo          = 0x0;
        pSysRegs->io_format_hi          = 0x0;
        pSysRegs->pullup_down           = 0x0;

        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            // 2014-02-20 per ChrisF: don't init channel specific registers
            // 2014-03-12 per LarryR: init channel specific registers for DTx
            pChRegs[ch]->reset_mode_timers  = 0x0;
            pChRegs[ch]->reset_timers       = 0x0;
            pChRegs[ch]->reset_fifo         = 0x0;
            pChRegs[ch]->period_debounce    = 0x0;
            pChRegs[ch]->duty_cycle         = 0x0;
            pChRegs[ch]->num_cycles         = 0x0;
            pChRegs[ch]->mode               = 0x0;
            pChRegs[ch]->start_pwm          = 0x0;
            pChRegs[ch]->max_hi_thr         = 0x32;
            pChRegs[ch]->upper_thr          = 0x28;
            pChRegs[ch]->lower_thr          = 0x10;
            pChRegs[ch]->min_lo_thr         = 0xA;
            pChRegs[ch]->debounce_time      = 0x0;
        }

        for (bank = BANK_A; bank < NUM_BANKS; ++bank)
        {
            pBankRegs[bank]->source_sink_current = 0x0;
        }
    }
}


static void load_caldata(void)
{
    CHANNEL ch;
    BANK    bank;

    if (!(pCommon->mod_ready & MOD_READY_CAL_LOADED))
    {
        /* Use default values */
        for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        {
            pCalData->volt_gain_ch[ch]          = 0xC80;
            pCalData->current_gain[ch]          = 0xC000;
            pCalData->bit_volt_gain[ch]         = 0x1030;
            pCalData->volt_offset_ch[ch]        = 0x0;
            pCalData->current_offset[ch]        = 0x0;
            pCalData->bit_volt_offset[ch]       = 0x0;
            pCalData->current_offset_60v[ch]    = 0x0;
        }

        for (bank = BANK_A; bank < NUM_BANKS; ++bank)
        {
            pCalData->volt_gain_bank[bank]      = 0x800;
            pCalData->volt_offset_bank[bank]    = 0x40;
        }
    }
}

