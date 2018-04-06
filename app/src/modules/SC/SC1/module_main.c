/*
 * SC1 module main code.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#include <includes.h>


/* Define to fake SERDES host - runs self-contained main loop test */
#undef FAKE_HOST

/* Exported global variables */
HOST_REGS *pHostRegs = (HOST_REGS *)HOST_REGS_BASE;
HPS_REGS  *pHpsRegs  = (HPS_REGS *)HPS_REGS_BASE;

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
    
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        if (ch_init[ch])
        {
            if (!ch_en[ch])
            {
                /* Run BIT? */
                if (pHostRegs->tx_rx_cfg[ch].bits.run_bit)
                    bit_me(ch, FALSE);

                /* Enable channel? */
                if (pHostRegs->tx_rx_cfg[ch].bits.channel_en)
                    enable_channel(ch);
            }

            /* Disable channel? */
            if (ch_en[ch] && (pHostRegs->tx_rx_cfg[ch].bits.channel_en == 0))
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
    u32     ctrl = pHostRegs->ctrl[ch].reg;
    XTime   time;

    /*
     * Set up
     */

    /* Set channel control to HPS */
    pHpsRegs->hps_ctrl[ch].bits.ch_ctrl = HPS_CTRL_CH_CTRL_HPS;

    /* Hold channel in reset */
    pHostRegs->ctrl[ch].bits.ch_reset = 1;

    /* Save user configuration */
    cfg.proto           = pHostRegs->proto[ch].reg;
    cfg.clk_mode        = pHostRegs->clk_mode[ch].reg;
    cfg.if_levels       = pHostRegs->if_levels[ch].reg;
    cfg.tx_rx_cfg       = pHostRegs->tx_rx_cfg[ch].reg;
    cfg.data_cfg        = pHostRegs->data_cfg[ch].reg;
    cfg.baud            = pHostRegs->baud[ch];
    cfg.preamble        = pHostRegs->preamble[ch].reg;
    cfg.tx_fifo_a_empty = pHostRegs->tx_fifo_a_empty[ch];
    cfg.rx_fifo_a_full  = pHostRegs->rx_fifo_a_full[ch];
    cfg.rx_fifo_hi      = pHostRegs->rx_fifo_hi[ch];
    cfg.rx_fifo_lo      = pHostRegs->rx_fifo_lo[ch];
    cfg.hdlc_rx_char    = pHostRegs->hdlc_rx_char[ch];
    cfg.hdlc_tx_char    = pHostRegs->hdlc_tx_char[ch];
    cfg.xon_char        = pHostRegs->xon_char[ch];
    cfg.xoff_char       = pHostRegs->xoff_char[ch];
    cfg.term_char       = pHostRegs->term_char[ch];
    cfg.timeout         = pHostRegs->timeout[ch];

    /* Load default configuration; set manual loopback mode and 1M baud rate */
    set_defaults(ch, TRUE);
    pHostRegs->if_levels[ch].reg = HOST_IF_LEVELS_MODE_M_LOOPBACK;
    pHostRegs->baud[ch] = 1000000;

    /* Apply comm params */
    send_comm_params(ch);

    /* Calculate baud rate */
    pHpsRegs->baud[ch] = (u32)(pHostRegs->baud[ch] * BAUD_MULTIPLIER);

    /* Release channel from reset */
    pHostRegs->ctrl[ch].bits.ch_reset = 0;

    /* Put FPGA in BIT mode */
    pHpsRegs->hps_ctrl[ch].bits.bit_mode = 1;

    /*
     * Run loopback test
     */

    debug("\nBIT Test on Ch%d\n", ch + 1);

    /* Fill TX FIFO with known unique pattern */
    debug("Writing TX FIFO... ");
    for (count = 0; count < BIT_DATA_LEN; count++)
    {
        pHostRegs->tx_fifo[ch] = count & 0xFF;
    }
    debug("Done\n");

    /* Go */
    pHostRegs->ctrl[ch].reg = HOST_CTRL_TRISTATE_TX_BIT | HOST_CTRL_TX_INIT_BIT | HOST_CTRL_RCVR_EN_BIT;

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
    pHostRegs->ctrl[ch].reg = HOST_CTRL_TRISTATE_TX_BIT;

    /* Read back and verify */
    debug("Verifying... ");
    for (count = 0; count < BIT_DATA_LEN; count++)
    {
        val = pHostRegs->rx_fifo[ch] & 0xFF;
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
    pHpsRegs->hps_ctrl[ch].bits.bit_mode = 0;

    /* Hold channel in reset */
    pHostRegs->ctrl[ch].bits.ch_reset = 1;

    /* Restore user configuration */
    pHostRegs->proto[ch].reg        = cfg.proto;
    pHostRegs->clk_mode[ch].reg     = cfg.clk_mode;
    pHostRegs->if_levels[ch].reg    = cfg.if_levels;
    pHostRegs->tx_rx_cfg[ch].reg    = cfg.tx_rx_cfg;
    pHostRegs->data_cfg[ch].reg     = cfg.data_cfg;
    pHostRegs->baud[ch]             = cfg.baud;
    pHostRegs->preamble[ch].reg     = cfg.preamble;
    pHostRegs->tx_fifo_a_empty[ch]  = cfg.tx_fifo_a_empty;
    pHostRegs->rx_fifo_a_full[ch]   = cfg.rx_fifo_a_full;
    pHostRegs->rx_fifo_hi[ch]       = cfg.rx_fifo_hi;
    pHostRegs->rx_fifo_lo[ch]       = cfg.rx_fifo_lo;
    pHostRegs->hdlc_rx_char[ch]     = cfg.hdlc_rx_char;
    pHostRegs->hdlc_tx_char[ch]     = cfg.hdlc_tx_char;
    pHostRegs->xon_char[ch]         = cfg.xon_char;
    pHostRegs->xoff_char[ch]        = cfg.xoff_char;
    pHostRegs->term_char[ch]        = cfg.term_char;
    pHostRegs->timeout[ch]          = cfg.timeout;

    /* Restore comm params */
    send_comm_params(ch);

    /* Restore baud rate */
    pHpsRegs->baud[ch] = (u32)(pHostRegs->baud[ch] * BAUD_MULTIPLIER);

    /* Restore control register */
    pHostRegs->ctrl[ch].reg = ctrl;

    /* Write BIT status register - generates interrupt to host */
    pHpsRegs->hps_ctrl[ch].bits.bit_passed = pass;

    /* Signal completion */
    pHostRegs->tx_rx_cfg[ch].bits.run_bit = 0;

    return XST_SUCCESS;
}

/****************************** Local functions *****************************/

static void init(CHANNEL ch)
{
    /* Enable power supply */
    pHpsRegs->ps_en[ch].bits.enable = 1;
    usleep(1000);

    /* Load configuration parameters */
    load_params(ch);

    /* Let HOST know that channel is ready to be configured */
    pHpsRegs->hps_ctrl[ch].bits.bit_passed = 1;

    /* Channel is now initialized */
    ch_init[ch] = TRUE;
}


static void load_params(CHANNEL ch)
{
    /* Hold channel in reset */
    pHostRegs->ctrl[ch].reg = HOST_CTRL_CHANNEL_RESET_BIT;

    if (!(pCommon->mod_ready & MOD_READY_PARAM_LOADED))
    {
        /* Use default values */
        set_defaults(ch, FALSE);
    }

    /* Apply comm params */
    send_comm_params(ch);

    /* Calculate baud rate */
    pHpsRegs->baud[ch] = (u32)(pHostRegs->baud[ch] * BAUD_MULTIPLIER);

    /* Program FIFO address/size registers */
    pHpsRegs->tx_fifo_start[ch] = TX_FIFO(ch) >> SDRAM_TO_FIFO_SHIFT;
    pHpsRegs->rx_fifo_start[ch] = RX_FIFO(ch) >> SDRAM_TO_FIFO_SHIFT;
    pHpsRegs->tx_fifo_full[ch] = MAX_FIFO_SIZE;
    pHpsRegs->rx_fifo_full[ch] = MAX_FIFO_SIZE;

    /* Reset and enable FIFOs */
    pHpsRegs->hps_ctrl[ch].reg = HPS_CTRL_FIFO_RESET_BIT;
    pHpsRegs->hps_ctrl[ch].reg = 0x0;
    pHpsRegs->hps_ctrl[ch].reg = HPS_CTRL_FIFO_EN_BIT;

    /* Release channel from reset */
    pHostRegs->ctrl[ch].bits.ch_reset = 0;
}


static void set_defaults(CHANNEL ch, bool bit)
{
    pHostRegs->proto[ch].reg        = HOST_PROTO_MODE_ASYNC;
    pHostRegs->clk_mode[ch].reg     = 0x0;
    pHostRegs->if_levels[ch].reg    = HOST_IF_LEVELS_MODE_TRISTATE;
    pHostRegs->tx_rx_cfg[ch].reg    = bit ? HOST_TX_RX_CFG_RUN_BIT : 0x0;
    pHostRegs->data_cfg[ch].reg     = 0x0108;
    pHostRegs->baud[ch]             = 9600;
    pHostRegs->preamble[ch].reg     = 0x0;
    pHostRegs->tx_fifo_a_empty[ch]  = 0x64;
    pHostRegs->rx_fifo_a_full[ch]   = FIFO_WATERMARK;
    pHostRegs->rx_fifo_hi[ch]       = FIFO_WATERMARK;
    pHostRegs->rx_fifo_lo[ch]       = 0x800;
    pHostRegs->hdlc_rx_char[ch]     = 0xA5;
    pHostRegs->hdlc_tx_char[ch]     = 0xA5;
    pHostRegs->xon_char[ch]         = 0x11;
    pHostRegs->xoff_char[ch]        = 0x13;
    pHostRegs->term_char[ch]        = 0x03;
    pHostRegs->timeout[ch]          = 0x9C40;
}


static XStatus validate_config(CHANNEL ch)
{
#if MODULE_SC7
    /* SC7: Hardware flow control not supported */
    if (pHostRegs->tx_rx_cfg[ch].bits.rts_cts)
        return XST_FAILURE;
#endif

    return XST_SUCCESS;
}


static void send_comm_params(CHANNEL ch)
{
    u32 ctrl      = pHostRegs->ctrl[ch].reg;
    u32 if_levels = pHostRegs->if_levels[ch].bits.mode;
    u32 clk_mode  = pHostRegs->clk_mode[ch].bits.mode;
    u32 params    = 0;

    if (pHostRegs->if_levels[ch].bits.fpga_loopback)
        if_levels = HOST_IF_LEVELS_MODE_TRISTATE;

    switch (if_levels)
    {
        case HOST_IF_LEVELS_MODE_RS232:
            if (pHostRegs->proto[ch].reg == HOST_PROTO_MODE_ASYNC)
                params = HPS_COMM_PARAMS_RS232_ASYNC;
            else
                params = HPS_COMM_PARAMS_RS232_SYNC;
            break;
        case HOST_IF_LEVELS_MODE_RS422:
            if (clk_mode ==  HOST_CLOCK_MODE_INTERNAL)
                params = HPS_COMM_PARAMS_RS422_INT_CLK;
            else
                params = HPS_COMM_PARAMS_RS422_EXT_CLK;
            break;
        case HOST_IF_LEVELS_MODE_RS485:
            if (clk_mode == HOST_CLOCK_MODE_INTERNAL)
                params = HPS_COMM_PARAMS_RS485_INT_CLK;
            else
                params = HPS_COMM_PARAMS_RS485_EXT_CLK;
            break;
        case HOST_IF_LEVELS_MODE_RS423:
            params = HPS_COMM_PARAMS_RS423;
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

    pHpsRegs->comm_params[ch].reg = params;
    pHpsRegs->hps_ctrl[ch].bits.send_params = 1;
    pHpsRegs->hps_ctrl[ch].bits.send_params = 0;
    pHostRegs->ctrl[ch].reg = ctrl;
}


static void enable_channel(CHANNEL ch)
{
    /* Don't enable the channel if configuration is invalid */
    if (validate_config(ch))
        return;

    /* Hold channel in reset */
    pHostRegs->ctrl[ch].bits.ch_reset = 1;

    /* Write HOST registers to FPGA */
    HOST2FPGA(pHostRegs->proto[ch].reg);
    HOST2FPGA(pHostRegs->clk_mode[ch].reg);
    HOST2FPGA(pHostRegs->if_levels[ch].reg);
    HOST2FPGA(pHostRegs->tx_rx_cfg[ch].reg);
    HOST2FPGA(pHostRegs->data_cfg[ch].reg);
    HOST2FPGA(pHostRegs->baud[ch]);
    HOST2FPGA(pHostRegs->preamble[ch].reg);
    HOST2FPGA(pHostRegs->tx_fifo_a_empty[ch]);
    HOST2FPGA(pHostRegs->rx_fifo_a_full[ch]);
    HOST2FPGA(pHostRegs->rx_fifo_hi[ch]);
    HOST2FPGA(pHostRegs->rx_fifo_lo[ch]);
    HOST2FPGA(pHostRegs->hdlc_rx_char[ch]);
    HOST2FPGA(pHostRegs->hdlc_tx_char[ch]);
    HOST2FPGA(pHostRegs->xon_char[ch]);
    HOST2FPGA(pHostRegs->xoff_char[ch]);
    HOST2FPGA(pHostRegs->term_char[ch]);
    HOST2FPGA(pHostRegs->timeout[ch]);

    /* Apply comm params */
    send_comm_params(ch);

    /* Calculate baud rate */
    pHpsRegs->baud[ch] = (u32)(pHostRegs->baud[ch] * BAUD_MULTIPLIER);

    /* Release channel from reset */
    pHostRegs->ctrl[ch].bits.ch_reset = 0;

    /* Set channel control to HOST */
    pHpsRegs->hps_ctrl[ch].bits.ch_ctrl = HPS_CTRL_CH_CTRL_HOST;

    /* Channel is now enabled */
    ch_en[ch] = TRUE;
}


static void disable_channel(CHANNEL ch)
{
    /* Set channel control to HPS */
    pHpsRegs->hps_ctrl[ch].bits.ch_ctrl = HPS_CTRL_CH_CTRL_HPS;

    /* Hold channel in reset */
    pHostRegs->ctrl[ch].bits.ch_reset = 1;

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
    u32 ch_ctrl = pHpsRegs->hps_ctrl[ch].bits.ch_ctrl;
	u32 data = 0x55;
    static u32 state = 1;
#ifdef TX_ALWAYS_MODE
    static u32 count = 0;
#else
    u32 stat_mask = HOST_CH_STAT_RX_DATA_AVAIL_BIT | HOST_CH_STAT_TX_COMPLETE_BIT;
#endif

    /* Set channel control to HPS */
    pHpsRegs->hps_ctrl[ch].reg &= ~(HPS_CTRL_CH_CTRL_BIT | HPS_CTRL_BIT_PASSED_BIT);

    // ch_stat->bit_passed is directly controlled by hps_ctrl->bit_passed
    //if (pHostRegs->ch_stat[ch].bits.bit_passed)
    if (ch_init[ch])
    {
        switch (state)
        {
            case 1:
                /* Override defaults and enable channel */
                pHostRegs->if_levels[ch].reg             = HOST_IF_LEVELS_MODE_M_LOOPBACK;
                pHostRegs->baud[ch]                      = 1000000;
                pHostRegs->tx_rx_cfg[ch].bits.channel_en = 1;
                state = 2;
                break;

            case 2:
                // ch_stat->ch_ready is directly controlled by hps_ctrl->ch_ctrl
                //if (!pHostRegs->ch_stat[ch].bits.ch_ready)
                //    break;
#ifdef TX_ALWAYS_MODE
                /* Enable TxAlways and transmit one word at a time */
                if (!count)
                    pHostRegs->ctrl[ch].reg |= (HOST_CTRL_RCVR_EN_BIT | HOST_CTRL_TX_ALWAYS_BIT);
                if (count < buf_size)
                    pHostRegs->tx_fifo[ch] = (data + count++) & 0xFF;
                else
                    state = 3;
#else
                /* Fill with known unique pattern and transmit */
                for (i = 0; i < buf_size; i++)
                {
                    pHostRegs->tx_fifo[ch] = (data + i) & 0xFF;
                }
                pHostRegs->ctrl[ch].reg |= (HOST_CTRL_RCVR_EN_BIT | HOST_CTRL_TX_INIT_BIT);
                state = 3;
#endif
                break;

            case 3:
#ifdef TX_ALWAYS_MODE
                /* Wait for RX count to reach count */
                if (pHostRegs->rx_count[ch] == count)
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
                    val = pHostRegs->rx_fifo[ch];
                    val &= ~0x200;  // Mask parity bit for now
                    if (val != temp)
                    {
                        printf("\nData mismatch at word %08lX: expected %08lX, got %08lX\n", i, temp, val);
                        pHpsRegs->hps_ctrl[ch].bits.bit_passed = 0;
                        ch_init[ch] = FALSE;
                        break;
                    }
                }
                //state = 5;
                state = 2;
                break;

            case 5:
                /* Stop Rx/Tx and Disable Channel */
                pHostRegs->ctrl[ch].reg &= ~(HOST_CTRL_RCVR_EN_BIT | HOST_CTRL_TX_ALWAYS_BIT);
                pHostRegs->tx_rx_cfg[ch].bits.channel_en = 0;
                state = 1;
                break;
        }
    }

    /* Restore channel control */
    pHpsRegs->hps_ctrl[ch].bits.ch_ctrl = ch_ctrl;
}
#endif

