/*
 * SC3 module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Define to fake SERDES host - runs self-contained main loop test */
#undef FAKE_HOST

/* Exported global variables */
HOST_REGS *pHostRegs[NUM_CHANNELS];
HPS_REGS  *pHpsRegs[NUM_CHANNELS];

/* Global variables */
static bool ch_en[NUM_CHANNELS]   = { FALSE };
static bool ch_init[NUM_CHANNELS] = { FALSE };

/* Local functions */
static void init(CHANNEL ch);
static void load_params(CHANNEL ch);
static void set_defaults(CHANNEL ch, bool bit);
static XStatus validate_config(CHANNEL ch);
static void send_comm_params(CHANNEL ch);
static void enable_channel(CHANNEL ch);
static void disable_channel(CHANNEL ch);
#ifdef FAKE_HOST
static void fake_host_test(CHANNEL ch);
#endif


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
#if ! MODULE_SF2
    CHANNEL ch2;
#endif
    
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        if (ch_init[ch])
        {
            if (!ch_en[ch])
            {
#if ! MODULE_SF2
                /* Run BIT? */
                if (pHostRegs[ch]->tx_rx_cfg.bits.run_bit)
                {
                    /* Transceivers on SC3 are shared; make sure both channels in a pair are disabled */
                    ch2 = (ch & 1) ? (ch - 1) : (ch + 1);
                    if (!ch_en[ch2])
                        bit_me(ch, FALSE);
                }
#endif
                /* Enable channel? */
                if (pHostRegs[ch]->tx_rx_cfg.bits.channel_en)
                    enable_channel(ch);
            }

            /* Disable channel? */
            if (ch_en[ch] && (pHostRegs[ch]->tx_rx_cfg.bits.channel_en == 0))
                disable_channel(ch);
        }
    }

#ifdef FAKE_HOST
    fake_host_test(CHANNEL_1);
#endif
}


XStatus bit_me(CHANNEL ch, bool verbose)
{
    CONFIG  cfg;
    u32     stat_mask = HOST_CH_STAT_RX_DATA_AVAIL_BIT | HOST_CH_STAT_TX_COMPLETE_BIT;
    bool    pass = TRUE;
    u32     val, temp, count;
    u32     ctrl = pHostRegs[ch]->ctrl.reg;
    XTime   time;

    /*
     * Set up
     */

    /* Set channel control to HPS */
    pHpsRegs[ch]->hps_ctrl.bits.ch_ctrl = HPS_CTRL_CH_CTRL_HPS;

    /* Hold channel in reset */
    pHostRegs[ch]->ctrl.bits.ch_reset = 1;

    /* Save user configuration */
    cfg.proto           = pHostRegs[ch]->proto.reg;
    cfg.clk_mode        = pHostRegs[ch]->clk_mode.reg;
    cfg.if_levels       = pHostRegs[ch]->if_levels.reg;
    cfg.tx_rx_cfg       = pHostRegs[ch]->tx_rx_cfg.reg;
    cfg.data_cfg        = pHostRegs[ch]->data_cfg.reg;
    cfg.baud            = pHostRegs[ch]->baud;
    cfg.preamble        = pHostRegs[ch]->preamble.reg;
    cfg.tx_fifo_a_empty = pHostRegs[ch]->tx_fifo_a_empty;
    cfg.rx_fifo_a_full  = pHostRegs[ch]->rx_fifo_a_full;
    cfg.rx_fifo_hi      = pHostRegs[ch]->rx_fifo_hi;
    cfg.rx_fifo_lo      = pHostRegs[ch]->rx_fifo_lo;
    cfg.hdlc_rx_char    = pHostRegs[ch]->hdlc_rx_char;
    cfg.hdlc_tx_char    = pHostRegs[ch]->hdlc_tx_char;
    cfg.xon_char        = pHostRegs[ch]->xon_char;
    cfg.xoff_char       = pHostRegs[ch]->xoff_char;
    cfg.term_char       = pHostRegs[ch]->term_char;
    cfg.timeout         = pHostRegs[ch]->timeout;

    /* Load default configuration; set manual loopback mode and 1M baud rate */
    set_defaults(ch, TRUE);
    pHostRegs[ch]->if_levels.reg = HOST_IF_LEVELS_MODE_M_LOOPBACK;
    pHostRegs[ch]->baud = 1000000;

    /* Apply comm params */
    send_comm_params(ch);

    /* Calculate baud rate */
    pHpsRegs[ch]->baud = (u32)(pHostRegs[ch]->baud * BAUD_MULTIPLIER);

    /* Release channel from reset */
    pHostRegs[ch]->ctrl.bits.ch_reset = 0;

    /* Put FPGA in BIT mode */
    pHpsRegs[ch]->hps_ctrl.bits.bit_mode = 1;

    /*
     * Run loopback test
     */

    debug("\nBIT Test on Ch%d\n", ch + 1);

    /* Fill TX FIFO with known unique pattern */
    debug("Writing TX FIFO... ");
    for (count = 0; count < BIT_DATA_LEN; count++)
    {
        pHostRegs[ch]->tx_fifo = count & 0xFF;
    }
    debug("Done\n");

    /* Go */
    pHostRegs[ch]->ctrl.reg = HOST_CTRL_TRISTATE_TX_BIT | HOST_CTRL_TX_INIT_BIT | HOST_CTRL_RCVR_EN_BIT;

    /* Wait for TX complete and RX data available */
    debug("Transmitting... ");
    time = get_timer(0);
    do {
        val = pCommon->irq[CH_IRQ(ch)].raw_status;
        /* It should take < 1ms to send 32 DWORDs at 1M baud */
        if (get_timer(time) > 10)
            break;
    } while ((val & stat_mask) != stat_mask);
    time = get_timer(time);
    if ((val & stat_mask) != stat_mask)
        pass = FALSE;
    debug("%s (status=0x%08lX, time=%lldms)\n", pass ? "COMPLETED" : "FAILED!", val, time);

    /* Stop */
    pHostRegs[ch]->ctrl.reg = HOST_CTRL_TRISTATE_TX_BIT;

    /* Read back and verify */
    debug("Verifying... ");
    for (count = 0; count < BIT_DATA_LEN; count++)
    {
        val = pHostRegs[ch]->rx_fifo & 0xFF;
        temp = count & 0xFF;
        if (val != temp)
        {
            debug("\nData mismatch at word %08lX: expected %08lX, got %08lX\n", count, temp, val);
            pass = FALSE;
            break;
        }
    }
    if (pass)
        debug("Done\n");

    if (count == BIT_DATA_LEN)
        debug("PASSED (RX data = TX data)\n");
    else
        debug("FAILED (RX data != TX data)\n");

    /*
     * Clean up
     */

    /* Put FPGA in normal mode */
    pHpsRegs[ch]->hps_ctrl.bits.bit_mode = 0;

    /* Hold channel in reset */
    pHostRegs[ch]->ctrl.bits.ch_reset = 1;

    /* Restore user configuration */
    pHostRegs[ch]->proto.reg        = cfg.proto;
    pHostRegs[ch]->clk_mode.reg     = cfg.clk_mode;
    pHostRegs[ch]->if_levels.reg    = cfg.if_levels;
    pHostRegs[ch]->tx_rx_cfg.reg    = cfg.tx_rx_cfg;
    pHostRegs[ch]->data_cfg.reg     = cfg.data_cfg;
    pHostRegs[ch]->baud             = cfg.baud;
    pHostRegs[ch]->preamble.reg     = cfg.preamble;
    pHostRegs[ch]->tx_fifo_a_empty  = cfg.tx_fifo_a_empty;
    pHostRegs[ch]->rx_fifo_a_full   = cfg.rx_fifo_a_full;
    pHostRegs[ch]->rx_fifo_hi       = cfg.rx_fifo_hi;
    pHostRegs[ch]->rx_fifo_lo       = cfg.rx_fifo_lo;
    pHostRegs[ch]->hdlc_rx_char     = cfg.hdlc_rx_char;
    pHostRegs[ch]->hdlc_tx_char     = cfg.hdlc_tx_char;
    pHostRegs[ch]->xon_char         = cfg.xon_char;
    pHostRegs[ch]->xoff_char        = cfg.xoff_char;
    pHostRegs[ch]->term_char        = cfg.term_char;
    pHostRegs[ch]->timeout          = cfg.timeout;

    /* Restore comm params */
    send_comm_params(ch);

    /* Restore baud rate */
    pHpsRegs[ch]->baud = (u32)(pHostRegs[ch]->baud * BAUD_MULTIPLIER);

    /* Restore control register */
    pHostRegs[ch]->ctrl.reg = ctrl;

    /* Write BIT status register - generates interrupt to host */
    pHpsRegs[ch]->hps_ctrl.bits.bit_passed = pass;

    /* Signal completion */
    pHostRegs[ch]->tx_rx_cfg.bits.run_bit = 0;

    return XST_SUCCESS;
}

/****************************** Local functions *****************************/

static void init(CHANNEL ch)
{
    /* Initialize global pointers to registers */
    pHostRegs[ch] = (HOST_REGS *)HOST_REGS_BASE + ch;
    pHpsRegs[ch]  = (HPS_REGS *)HPS_REGS_BASE + ch;

    /* Enable power supply */
    pHpsRegs[ch]->ps_en.bits.enable = 1;
    usleep(1000);

    /* Load configuration parameters */
    load_params(ch);

    /* Let HOST know that channel is ready to be configured */
    pHpsRegs[ch]->hps_ctrl.bits.bit_passed = 1;

    /* Channel is now initialized */
    ch_init[ch] = TRUE;
}


static void load_params(CHANNEL ch)
{
    /* Hold channel in reset */
    pHostRegs[ch]->ctrl.reg = HOST_CTRL_CHANNEL_RESET_BIT;

    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */
        set_defaults(ch, FALSE);
    }

    /* Apply comm params */
    send_comm_params(ch);

    /* Calculate baud rate */
    pHpsRegs[ch]->baud = (u32)(pHostRegs[ch]->baud * BAUD_MULTIPLIER);

    /* Program FIFO address/size registers */
    pHpsRegs[ch]->tx_fifo_start = TX_FIFO(ch) >> SDRAM_TO_FIFO_SHIFT;
    pHpsRegs[ch]->rx_fifo_start = RX_FIFO(ch) >> SDRAM_TO_FIFO_SHIFT;
    pHpsRegs[ch]->tx_fifo_full = MAX_FIFO_SIZE;
    pHpsRegs[ch]->rx_fifo_full = MAX_FIFO_SIZE;

    /* Reset and enable FIFOs */
    pHpsRegs[ch]->hps_ctrl.reg = HPS_CTRL_FIFO_RESET_BIT;
    pHpsRegs[ch]->hps_ctrl.reg = 0x0;
    pHpsRegs[ch]->hps_ctrl.reg = HPS_CTRL_FIFO_EN_BIT;

    /* Release channel from reset */
    pHostRegs[ch]->ctrl.bits.ch_reset = 0;
}


static void set_defaults(CHANNEL ch, bool bit)
{
    pHostRegs[ch]->proto.reg        = HOST_PROTO_MODE_ASYNC;
    pHostRegs[ch]->clk_mode.reg     = 0x0;
    pHostRegs[ch]->if_levels.reg    = HOST_IF_LEVELS_MODE_TRISTATE;
    pHostRegs[ch]->tx_rx_cfg.reg    = bit ? HOST_TX_RX_CFG_RUN_BIT : 0x0;
    pHostRegs[ch]->data_cfg.reg     = 0x0108;
    pHostRegs[ch]->baud             = 9600;
    pHostRegs[ch]->preamble.reg     = 0x0;
    pHostRegs[ch]->tx_fifo_a_empty  = 0x64;
    pHostRegs[ch]->rx_fifo_a_full   = FIFO_WATERMARK;
    pHostRegs[ch]->rx_fifo_hi       = FIFO_WATERMARK;
    pHostRegs[ch]->rx_fifo_lo       = 0x800;
    pHostRegs[ch]->hdlc_rx_char     = 0xA5;
    pHostRegs[ch]->hdlc_tx_char     = 0xA5;
    pHostRegs[ch]->xon_char         = 0x11;
    pHostRegs[ch]->xoff_char        = 0x13;
    pHostRegs[ch]->term_char        = 0x03;
    pHostRegs[ch]->timeout          = 0x9C40;
}


static XStatus validate_config(CHANNEL ch)
{
    /* SC3: Async only */
    if (pHostRegs[ch]->proto.reg != HOST_PROTO_MODE_ASYNC)
        return XST_FAILURE;

    /* SC3: Internal clock only */
    if (pHostRegs[ch]->clk_mode.bits.mode != HOST_CLOCK_MODE_INTERNAL)
        return XST_FAILURE;

    /* SC3: RS423 not supported */
    if (pHostRegs[ch]->if_levels.bits.mode == HOST_IF_LEVELS_MODE_RS423)
        return XST_FAILURE;

    /* SC3: Data encoding not supported */
    if (pHostRegs[ch]->data_cfg.bits.encoding != HOST_DATA_CFG_ENC_NONE)
        return XST_FAILURE;

#if MODULE_SF2
    /* SF2: RS422 and RS485 only */
    if ((pHostRegs[ch]->if_levels.bits.mode != HOST_IF_LEVELS_MODE_RS422) &&
        (pHostRegs[ch]->if_levels.bits.mode != HOST_IF_LEVELS_MODE_RS485))
        return XST_FAILURE;

    /* SF2: RTS/CTS not supported */
    if (pHostRegs[ch]->tx_rx_cfg.bits.rts_cts)
        return XST_FAILURE;
#endif

    return XST_SUCCESS;
}


static void send_comm_params(CHANNEL ch)
{
    u32 ctrl      = pHostRegs[ch]->ctrl.reg;
    u32 if_levels = pHostRegs[ch]->if_levels.bits.mode;
    u32 params    = 0;

    if (pHostRegs[ch]->if_levels.bits.fpga_loopback)
        if_levels = HOST_IF_LEVELS_MODE_TRISTATE;

    switch (if_levels)
    {
        case HOST_IF_LEVELS_MODE_RS232:
            params = HPS_COMM_PARAMS_RS232;
            break;
        case HOST_IF_LEVELS_MODE_RS422:
            params = HPS_COMM_PARAMS_RS422_INT_CLK;
            break;
        case HOST_IF_LEVELS_MODE_RS485:
            params = HPS_COMM_PARAMS_RS485_INT_CLK;
            break;
        case HOST_IF_LEVELS_MODE_M_LOOPBACK:
            params = HPS_COMM_PARAMS_M_LOOPBACK;
            ctrl |= HOST_CTRL_TRISTATE_TX_BIT;
            break;
        case HOST_IF_LEVELS_MODE_TRISTATE:
            params = HPS_COMM_PARAMS_TRISTATE;
            ctrl |= HOST_CTRL_TRISTATE_TX_BIT;
            break;
    }

    pHpsRegs[ch]->comm_params.reg = params;
    pHpsRegs[ch]->hps_ctrl.bits.send_params = 1;
    pHpsRegs[ch]->hps_ctrl.bits.send_params = 0;
    pHostRegs[ch]->ctrl.reg = ctrl;
}


static void enable_channel(CHANNEL ch)
{
    /* Don't enable the channel if configuration is invalid */
    if (validate_config(ch))
        return;

    /* Hold channel in reset */
    pHostRegs[ch]->ctrl.bits.ch_reset = 1;

    /* Write HOST registers to FPGA */
    HOST2FPGA(pHostRegs[ch]->proto.reg);
    HOST2FPGA(pHostRegs[ch]->clk_mode.reg);
    HOST2FPGA(pHostRegs[ch]->if_levels.reg);
    HOST2FPGA(pHostRegs[ch]->tx_rx_cfg.reg);
    HOST2FPGA(pHostRegs[ch]->data_cfg.reg);
    HOST2FPGA(pHostRegs[ch]->baud);
    HOST2FPGA(pHostRegs[ch]->preamble.reg);
    HOST2FPGA(pHostRegs[ch]->tx_fifo_a_empty);
    HOST2FPGA(pHostRegs[ch]->rx_fifo_a_full);
    HOST2FPGA(pHostRegs[ch]->rx_fifo_hi);
    HOST2FPGA(pHostRegs[ch]->rx_fifo_lo);
    HOST2FPGA(pHostRegs[ch]->hdlc_rx_char);
    HOST2FPGA(pHostRegs[ch]->hdlc_tx_char);
    HOST2FPGA(pHostRegs[ch]->xon_char);
    HOST2FPGA(pHostRegs[ch]->xoff_char);
    HOST2FPGA(pHostRegs[ch]->term_char);
    HOST2FPGA(pHostRegs[ch]->timeout);

    /* Apply comm params */
    send_comm_params(ch);

    /* Calculate baud rate */
    pHpsRegs[ch]->baud = (u32)(pHostRegs[ch]->baud * BAUD_MULTIPLIER);

    /* Release channel from reset */
    pHostRegs[ch]->ctrl.bits.ch_reset = 0;

    /* Set channel control to HOST */
    pHpsRegs[ch]->hps_ctrl.bits.ch_ctrl = HPS_CTRL_CH_CTRL_HOST;

    /* Channel is now enabled */
    ch_en[ch] = TRUE;
}


static void disable_channel(CHANNEL ch)
{
    /* Set channel control to HPS */
    pHpsRegs[ch]->hps_ctrl.bits.ch_ctrl = HPS_CTRL_CH_CTRL_HPS;

    /* Hold channel in reset */
    pHostRegs[ch]->ctrl.bits.ch_reset = 1;

    /* Disable power supply */
    //pHpsRegs->ps_en[ch].bits.enable = 0;

    /* Channel is now disabled */
    ch_en[ch] = FALSE;
}


#ifdef FAKE_HOST
#define TX_ALWAYS_MODE
static void fake_host_test(CHANNEL ch)
{
    u32 i;
    u32 val, temp;
    u32 buf_size = 0x10;
    u32 ch_ctrl = pHpsRegs[ch]->hps_ctrl.bits.ch_ctrl;
	u32 data = 0x55;
    static u32 state = 1;
#ifdef TX_ALWAYS_MODE
    static u32 count = 0;
#else
    u32 stat_mask = HOST_CH_STAT_RX_DATA_AVAIL_BIT | HOST_CH_STAT_TX_COMPLETE_BIT;
#endif

    /* Set channel control to HPS */
    pHpsRegs[ch]->hps_ctrl.reg &= ~(HPS_CTRL_CH_CTRL_BIT | HPS_CTRL_BIT_PASSED_BIT);

    // ch_stat->bit_passed is directly controlled by hps_ctrl->bit_passed
    //if (pHostRegs->ch_stat[ch].bits.bit_passed)
    if (ch_init[ch])
    {
        switch (state)
        {
            case 1:
                /* Override defaults and enable channel */
                pHostRegs[ch]->if_levels.reg             = HOST_IF_LEVELS_MODE_M_LOOPBACK;
                pHostRegs[ch]->baud                      = 1000000;
                pHostRegs[ch]->tx_rx_cfg.bits.channel_en = 1;
                state = 2;
                break;

            case 2:
                // ch_stat->ch_ready is directly controlled by hps_ctrl->ch_ctrl
                //if (!pHostRegs->ch_stat[ch].bits.ch_ready)
                //    break;
#ifdef TX_ALWAYS_MODE
                /* Enable TxAlways and transmit one word at a time */
                if (!count)
                    pHostRegs[ch]->ctrl.reg |= (HOST_CTRL_RCVR_EN_BIT | HOST_CTRL_TX_ALWAYS_BIT);
                if (count < buf_size)
                    pHostRegs[ch]->tx_fifo = (data + count++) & 0xFF;
                else
                    state = 3;
#else
                /* Fill with known unique pattern and transmit */
                for (i = 0; i < buf_size; i++)
                {
                    pHostRegs[ch]->tx_fifo = (data + i) & 0xFF;
                }
                pHostRegs[ch]->ctrl.reg |= (HOST_CTRL_RCVR_EN_BIT | HOST_CTRL_TX_INIT_BIT);
                state = 3;
#endif
                break;

            case 3:
#ifdef TX_ALWAYS_MODE
                /* Wait for RX count to reach count */
                if (pHostRegs[ch]->rx_count == count)
                {
                    state = 4;
                    count = 0;
                }
#else
                /* Wait for TX complete and RX data available */
                if ((pCommon->irq[CH_IRQ(ch)].raw_status & stat_mask) == stat_mask)
                    state = 4;
#endif
                break;

            case 4:
                /* Read back and verify */
                for (i = 0; i < buf_size; i++)
                {
                    temp = (data + i) & 0xFF;
                    val = pHostRegs[ch]->rx_fifo;
                    val &= ~0x200;  // Mask parity bit for now
                    if (val != temp)
                    {
                        printf("\nData mismatch at word %08lX: expected %08lX, got %08lX\n", i, temp, val);
                        pHpsRegs[ch]->hps_ctrl.bits.bit_passed = 0;
                        ch_init[ch] = FALSE;
                        break;
                    }
                }
                //state = 5;
                state = 2;
                break;

            case 5:
                /* Stop Rx/Tx and Disable Channel */
                pHostRegs[ch]->ctrl.reg &= ~(HOST_CTRL_RCVR_EN_BIT | HOST_CTRL_TX_ALWAYS_BIT);
                pHostRegs[ch]->tx_rx_cfg.bits.channel_en = 0;
                state = 1;
                break;
        }
    }

    /* Restore channel control */
    pHpsRegs[ch]->hps_ctrl.bits.ch_ctrl = ch_ctrl;
}
#endif

