/*
 * SC3 module test menu.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#if SERIAL_DEBUG

#include <includes.h>


/* IRQ register names */
#define IRQ_NUM_REGS    4
char *irq_regs[IRQ_NUM_REGS] = {
    "raw_status",
    "status",
    "mask",
    "edge_level"
};

/* HOST register names */
#define HOST_NUM_REGS     25
char *host_regs[HOST_NUM_REGS] = {
    "tx_fifo",
    "rx_fifo",
    "tx_count",
    "rx_count",
    "proto",
    "clk_mode",
    "if_levels",
    "tx_rx_cfg",
    "ctrl",
    "data_cfg",
    "baud",
    "preamble",
    "tx_fifo_a_empty",
    "rx_fifo_a_full",
    "rx_fifo_hi",
    "rx_fifo_lo",
    "hdlc_rx_char",
    "hdlc_tx_char",
    "xon_char",
    "xoff_char",
    "term_char",
    "timeout",
    "fifo_stat",
    "tx_fifo_size",
    "rx_fifo_size"
};

/* HPS register names */
#define HPS_NUM_REGS    8
char *hps_regs[HPS_NUM_REGS] = {
    "tx_fifo_start",
    "rx_fifo_start",
    "tx_fifo_full",
    "rx_fifo_full",
    "hps_ctrl",
    "baud",
    "ps_en",
    "comm_params"
};

#define REG_STRIDE  0x04

#define STR_SC_REG  "  %p: %-16s= 0x%08lX\n"

/* Local functions */
static void reg_dump(void);
static void sc_test(MODE mode, bool echo);
#if ! MODULE_SF2
static void bit_test(void);
#endif


/**************************** Exported functions ****************************/

void module_menu(void)
{
    bool    exit_menu = FALSE;
    char    *inbuf;

    /* Print menu choices */
    while (exit_menu == FALSE)
    {
        printf("\n\n>>>  %s Menu  <<<\n\n", MODULE_NAME);
        puts("1. Reg Dump");
        puts("2. HDLC Test");
        puts("3. ASync Test");
        puts("4. Echo Mode");
#if ! MODULE_SF2
        puts("5. BIT Test");
#endif
        puts("------------------");
        puts("M. Main menu");
        printf("\nEnter Selection: ");
        inbuf = pend_rx_line();
        if (!inbuf || (strlen(inbuf) > 1))
            continue;

        switch (tolower((int)*inbuf))
        {
            case '1':
                reg_dump();
                break;
            case '2':
                sc_test(HDLC_MODE, FALSE);
                break;
            case '3':
                sc_test(ASYNC_MODE, FALSE);
                break;
            case '4':
                sc_test(ASYNC_MODE, TRUE);
                break;
#if ! MODULE_SF2
            case '5':
                bit_test();
                break;
#endif
            case 'm':
                exit_menu = TRUE;
                break;
        }
    }
}

/****************************** Local functions *****************************/

static void reg_dump(void)
{
    static CHANNEL  ch = CHANNEL_1;
    u32     val;
    char    *inbuf;
    u32     addr, i;

    /* Get channel number */
    printf("\nChannel (1-%d) [%d]: ", NUM_CHANNELS, ch + 1);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && (val >= 1) && (val <= NUM_CHANNELS))
        {
            ch = val - 1;
        }
        else
        {
            printf("Valid values are 1 to %d\n", NUM_CHANNELS);
            pause();
            return;
        }
    }

    printf("\nCh %d IRQ registers:\n", ch + 1);
    addr = (u32)&pCommon->irq[CH_IRQ(ch)].raw_status;
    for (i = 0; i < IRQ_NUM_REGS; ++i)
    {
        printf(STR_SC_REG, (void *)addr, irq_regs[i], Xil_In32(addr));
        addr += 4;
    }

    printf("\nCh %d HOST registers:\n", ch + 1);
    addr = (u32)&pHostRegs[ch]->tx_fifo;
    for (i = 0; i < HOST_NUM_REGS; ++i)
    {
        printf(STR_SC_REG, (void *)addr, host_regs[i], Xil_In32(addr));
        addr += REG_STRIDE;
    }

    printf("\nCh %d HPS registers:\n", ch + 1);
    addr = (u32)&pHpsRegs[ch]->tx_fifo_start;
    for (i = 0; i < HPS_NUM_REGS; ++i)
    {
        printf(STR_SC_REG, (void *)addr, hps_regs[i], Xil_In32(addr));
        addr += REG_STRIDE;
    }

    pause();
}


static void sc_test(MODE mode, bool echo)
{
    static u32      buf_size = MAX_FIFO_SIZE;
    static u32      baud = 1000000;
    static u32      params = 0;
    static u32      ifl = HOST_IF_LEVELS_MODE_M_LOOPBACK | HOST_IF_LEVELS_FPGA_LOOPBACK;
    static MODE     init = INVALID_MODE;
    static CHANNEL  ch = CHANNEL_1;
    u32     addr;
    u32     val, temp;
    int     i;
    char    *inbuf;
    XTime   time;
    u32     stat_mask = HOST_CH_STAT_RX_DATA_AVAIL_BIT | HOST_CH_STAT_TX_COMPLETE_BIT;

    if (params == 0)
        params = echo ? HPS_COMM_PARAMS_RS422_INT_CLK : HPS_COMM_PARAMS_M_LOOPBACK;

    /* Get channel number */
    printf("\nChannel (1-%d) [%d]: ", NUM_CHANNELS, ch + 1);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && (val >= 1) && (val <= NUM_CHANNELS))
        {
            if (ch != (val - 1))
                init = INVALID_MODE;
            ch = val - 1;
        }
        else
        {
            printf("Valid values are 1 to %d\n", NUM_CHANNELS);
            pause();
            return;
        }
    }

    if (!echo)
    {
        /* Get buffer size */
        printf("Buffer size (0-%lX) [%lX]: ", MAX_FIFO_SIZE, buf_size);
        inbuf = pend_rx_line();
        if (!inbuf)
        {
            pause();
            return;
        }
        if (strlen(inbuf))
        {
            if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1) && (val <= MAX_FIFO_SIZE))
            {
                buf_size = val;
            }
            else
            {
                printf("Valid values are 0x0 to 0x%lX\n", MAX_FIFO_SIZE);
                pause();
                return;
            }
        }
    }

    /* Get baud rate */
    printf("Baud rate (300-%d) [%ld]: ", MAX_BAUD_RATE, baud);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && (val >= 300) && (val <= MAX_BAUD_RATE))
        {
            if (baud != val)
                init = INVALID_MODE;
            baud = val;
        }
        else
        {
            printf("Valid values are 300 to %d\n", MAX_BAUD_RATE);
            pause();
            return;
        }
    }

    /* Get comm params */
    printf("Comm params [%lX]: ", params);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
        {
            if (params != val)
                init = INVALID_MODE;
            params = val;
        }
        else
        {
            puts("Valid values are 0x0 to 0xFFFFFFFF");
            pause();
            return;
        }
    }

    /* Get interface levels */
    printf("Interface levels [%lX]: ", ifl);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
        {
            if (ifl != val)
                init = INVALID_MODE;
            ifl = val;
        }
        else
        {
            puts("Valid values are 0x0 to 0xFFFFFFFF");
            pause();
            return;
        }
    }
    putchar('\n');

    if ((init != mode) || echo)
    {
        /* 1. Set it up */
        //mw.l 0xff300040 0x2
        pHpsRegs[ch]->hps_ctrl.reg = 0x2;
        //mw.l 0xff300040 0x0
        pHpsRegs[ch]->hps_ctrl.reg = 0x0;
        //mw.l 0xff300060 0x1
        pHpsRegs[ch]->ps_en.reg = 0x1;
        // Wait 10ms after enabling each power supply
        usleep(10000);
        //mw.l 0xff300070 0xCC10
        pHpsRegs[ch]->comm_params.reg = params;
        //mw.l 0xff300040 0x4
        pHpsRegs[ch]->hps_ctrl.reg = 0x4;
        //mw.l 0xff300040 0x0
        pHpsRegs[ch]->hps_ctrl.reg = 0x0;
        if (mode == HDLC_MODE)
        {
            //mw.l 0xff200040 0x3
            pHostRegs[ch]->proto.reg = HOST_PROTO_MODE_HDLC;
        }
        else if (mode == ASYNC_MODE)
        {
            //mw.l 0xff200040 0x0
            pHostRegs[ch]->proto.reg = HOST_PROTO_MODE_ASYNC;
        }
        //mw.l 0xff200050 0x0
        pHostRegs[ch]->clk_mode.reg = 0x0;
        //mw.l 0xff200060 0x5
        pHostRegs[ch]->if_levels.reg = ifl;
        if (mode == HDLC_MODE)
        {
            //mw.l 0xff200070 0x40000
            pHostRegs[ch]->tx_rx_cfg.reg = 0x40000;
        }
        else if (mode == ASYNC_MODE)
        {
            //mw.l 0xff200070 0x0
            pHostRegs[ch]->tx_rx_cfg.reg = 0x0;
        }
        //mw.l 0xff200080 0x0
        pHostRegs[ch]->ctrl.reg = 0x0;
        //mw.l 0xff200090 0x0
        pHostRegs[ch]->data_cfg.reg = 0x0108;
        //mw.l 0xff2000B0 0x0
        pHostRegs[ch]->preamble.reg = 0x0;
        //mw.l 0xff2000C0 0x64
        pHostRegs[ch]->tx_fifo_a_empty = 0x64;
        //mw.l 0xff2000D0 0x7F9B
        pHostRegs[ch]->rx_fifo_a_full = FIFO_WATERMARK;
        //mw.l 0xff2000E0 0x7F9B
        pHostRegs[ch]->rx_fifo_hi = FIFO_WATERMARK;
        //mw.l 0xff2000F0 0x0800
        pHostRegs[ch]->rx_fifo_lo = 0x0800;
        //mw.l 0xff200100 0xA5
        pHostRegs[ch]->hdlc_rx_char = 0xA5;
        //mw.l 0xff200110 0x03
        pHostRegs[ch]->term_char = 0x03;
        //mw.l 0xff200120 0x11
        pHostRegs[ch]->xon_char = 0x11;
        //mw.l 0xff200130 0x13
        pHostRegs[ch]->xoff_char = 0x13;
        //mw.l 0xff200150 0x9C40
        pHostRegs[ch]->timeout = 0x9C40;
        //mw.l 0xff200160 0xA5
        pHostRegs[ch]->hdlc_tx_char = 0xA5;
        //mw.l 0xff300050 0x26E978D - 1Mbps @ 100MHz ref_clock
        pHpsRegs[ch]->baud = (u32)(baud * BAUD_MULTIPLIER);
        //mw.l 0xff300000 0x0020
        pHpsRegs[ch]->tx_fifo_start = TX_FIFO(ch) >> SDRAM_TO_FIFO_SHIFT;
        //mw.l 0xff300010 0x0040
        pHpsRegs[ch]->rx_fifo_start = RX_FIFO(ch) >> SDRAM_TO_FIFO_SHIFT;
        //mw.l 0xff300020 0x07fff
        pHpsRegs[ch]->tx_fifo_full = MAX_FIFO_SIZE;
        //mw.l 0xff300030 0x07fff
        pHpsRegs[ch]->rx_fifo_full = MAX_FIFO_SIZE;
        //mw.l 0xff300040 0x1
        pHpsRegs[ch]->hps_ctrl.reg = 0x1;
        //mw.l 0xff200080 0xE000
        pHostRegs[ch]->ctrl.reg = 0x2000;
        //mw.l 0xff200080 0x0
        pHostRegs[ch]->ctrl.reg = 0x100;
        //mw.l 0xff300040 0x2
        pHpsRegs[ch]->hps_ctrl.reg = 0x2;
        //mw.l 0xff300040 0x0
        pHpsRegs[ch]->hps_ctrl.reg = 0x0;
        //mw.l 0xff300040 0x1
        pHpsRegs[ch]->hps_ctrl.reg = 0x1;

        // Init done
        init = mode;
    }

    /* 2. TX count should equal 0 */
    //md.l 0xff200020 1   TX count
    val = pHostRegs[ch]->tx_count;
    if (val != 0)
        printf("-TX FIFO write: TX count != 0; count=%08lX\n", val);

    /* 3. RX count should equal 0 */
    //md.l 0xff200030 1   RX count
    val = pHostRegs[ch]->rx_count;
    if (val != 0)
        printf("-TX FIFO write: RX count != 0; count=%08lX\n", val);

    /* 4. Clear TX/RX FIFOs in SDRAM */
    printf("Clearing TX/RX FIFOs in SDRAM... ");
    for (i = 0; i < MAX_FIFO_SIZE; i++)
    {
        addr = TX_FIFO(ch) + (i * 4);
        Xil_Out32(addr, 0);
        addr = RX_FIFO(ch) + (i * 4);
        Xil_Out32(addr, 0);
    }
    puts("Done");

    /* If we are in "echo" mode, this loop runs until aborted by user */
    if (echo)
    {
        /* Enable Receiver */
        pHostRegs[ch]->ctrl.reg = 0x40100;

        /* Wait for data */
        printf("Receiving... ");
        while (1)
        {
            val = pHostRegs[ch]->rx_count;
            if (val)
            {
                /* Get data and print it */
                temp = pHostRegs[ch]->rx_fifo & 0xFF;
                printf("%02lX ", temp);

                /* Echo it back */
                pHostRegs[ch]->tx_fifo = temp;
                /*
                val = pHostRegs[ch]->tx_count;
                printf("(%lu)", val);
                */
                pHostRegs[ch]->ctrl.reg = 0x50100;

                time = get_timer(0);
                do {
                    val = pCommon->irq[CH_IRQ(ch)].raw_status;
                    /* It should take ~32ms to send 1 DWORD at 300 baud; Abort on timeout or key press */
                    if ((get_timer(time) > (unsigned long)(32)) || (getchar() > 0))
                    {
                        printf("Timeout/abort (status=0x%08lX, time=%lldms)\n", val, time);
                        break;
                    }
                } while ((val & HOST_CH_STAT_TX_COMPLETE_BIT) != HOST_CH_STAT_TX_COMPLETE_BIT);
                time = get_timer(time);
                
                pHostRegs[ch]->ctrl.reg = 0x40100;
            }

            /* Check for abort */
            if (getchar() > 0)
                break;
        };

        /* Stop */
        pHostRegs[ch]->ctrl.reg = 0x100;

        putchar('\n');
        pause();
        return;
    }

    /* 4. Fill with known unique pattern */
    printf("Writing TX FIFO... ");
    for (i = 0; i < buf_size; i++)
    {
        val = i & 0xFF;
        pHostRegs[ch]->tx_fifo = val;
    }
    puts("Done");

    /* 5. TX count should equal buf_size */
    //md.l 0xff200020 1   TX count
    val = pHostRegs[ch]->tx_count;
    if (val != buf_size)
        printf("-TX/RX EN: TX count != buf_size; buf_size=%08lX, count=%08lX\n", buf_size, val);

    /* 6. RX count should equal 0 */
    //md.l 0xff200030 1   RX count
    val = pHostRegs[ch]->rx_count;
    if (val != 0)
        printf("-TX/RX EN: RX count != 0; count=%08lX\n", val);

    /* 7. Go */
    //mw.l 0xff200080 0x40000 RX EN
    pHostRegs[ch]->ctrl.reg = 0x40100;
    //mw.l 0xff200080 0x50000 TX
    pHostRegs[ch]->ctrl.reg = 0x50100;

    /* 8. Wait for TX complete and RX data available */
    printf("Transmitting... ");
    time = get_timer(0);
    do {
        val = pCommon->irq[CH_IRQ(ch)].raw_status;
        /* It should take ~19hrs to send 0x200000 DWORDs at 300 baud; Abort on timeout or key press */
        if ((get_timer(time) > (unsigned long)(70000000)) || (getchar() > 0))
            break;
    } while ((val & stat_mask) != stat_mask);
    time = get_timer(time);
    printf("%s (status=0x%08lX, time=%lldms)\n", (val & 0x10) ? "COMPLETED" : "FAILED!", val, time);

    /* 9. Stop */
    //mw.l 0xff200080 0x0     OFF
    pHostRegs[ch]->ctrl.reg = 0x100;

    /* 10. TX count should equal 0 */
    //md.l 0xff200020 1   TX count
    val = pHostRegs[ch]->tx_count;
    if (val != 0)
        printf("-RX FIFO read: TX count != 0; count=%08lX\n", val);

    /* 11. RX count should equal buf_size */
    //md.l 0xff200030 1   RX count
    val = pHostRegs[ch]->rx_count;
    if (val != buf_size)
        printf("-RX FIFO read: RX count != buf_size; buf_size=%08lX, count=%08lX\n", buf_size, val);

    /* 12. Read back and verify */
    printf("Verifying... ");
    for (i = 0; i < buf_size; i++)
    {
        val = pHostRegs[ch]->rx_fifo;
        if (mode == HDLC_MODE)
        {
            if (i == 0)
            {
                if ((val & 0x4000) == 0)
                    printf("\nRX FIFO word 0: 0x4000 not set, data=%08lX\n", val);
                val &= ~0x4000;
            }
            else if (i == (buf_size - 1))
            {
                if ((val & 0x200) == 0)
                    printf("\nRX FIFO word 0x%X: 0x200 not set, data=%08lX\n", i, val);
                val &= ~0x200;
            }
        }
        else if (mode == ASYNC_MODE)
        {
            val &= ~0x200;  // Mask parity bit for now
        }

        temp = i & 0xFF;
        if (val != temp)
        {
            printf("\nData mismatch at word %08X: expected %08lX, got %08lX\n", i, temp, val);
            pause();
            return;
        }
    }
    puts("Done");

    /* 13. TX count should equal 0 */
    //md.l 0xff200020 1   TX count
    val = pHostRegs[ch]->tx_count;
    if (val != 0)
        printf("+RX FIFO read: TX count != 0; count=%08lX\n", val);

    /* 14. RX count should equal 0 */
    //md.l 0xff200030 1   RX count
    val = pHostRegs[ch]->rx_count;
    if (val != 0)
        printf("+RX FIFO read: RX count != 0; count=%08lX\n", val);

    if (i == buf_size)
        puts("\nPASSED (RX data = TX data)");

    pause();
}


#if ! MODULE_SF2
static void bit_test(void)
{
    static CHANNEL  ch = CHANNEL_1;
    u32     val;
    char    *inbuf;

    /* Get channel number */
    printf("\nChannel (1-%d) [%d]: ", NUM_CHANNELS, ch + 1);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && (val >= 1) && (val <= NUM_CHANNELS))
        {
            ch = val - 1;
        }
        else
        {
            printf("Valid values are 1 to %d\n", NUM_CHANNELS);
            pause();
            return;
        }
    }

    bit_me(ch, TRUE);

    pause();
}
#endif // #if ! MODULE_SF2

#endif // #if SERIAL_DEBUG

