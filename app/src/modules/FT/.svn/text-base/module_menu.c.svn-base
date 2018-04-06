/*
 * FTx module test menu.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#if SERIAL_DEBUG

#include <includes.h>


/* Local functions */
static void bc_test(void);
static void mm_test(void);
static void rt_test(void);


/**************************** Exported functions ****************************/

void module_menu(void)
{
    bool    exit_menu = FALSE;
    char    *inbuf;

    /* Print menu choices */
    while (exit_menu == FALSE)
    {
        printf("\n\n>>>  %s Menu  <<<\n\n", MODULE_NAME);
        puts("1. BC Test");
        puts("2. MM Test");
        puts("3. RT Test");
        puts("------------------");
        puts("M. Main menu");
        printf("\nEnter Selection: ");
        inbuf = pend_rx_line();
        if (!inbuf || (strlen(inbuf) > 1))
            continue;

        switch (tolower((int)*inbuf))
        {
            case '1':
                bc_test();
                break;
            case '2':
                mm_test();
                break;
            case '3':
                rt_test();
                break;
            case 'm':
                exit_menu = TRUE;
                break;
        }
    }
}

/****************************** Local functions *****************************/

static void bc_test(void)
{
    u16     val;
    XTime   time;

    printf("\nSending message... ");

    /* @@@ Dummy read - otherwise the writes don't go through after POR/reset */
    //val = Xil_In16(BRM1553D_REGS_BASE);

    /* Setup */
    //transmit "mw.w ff200002 0000^M"		;legacy BC, 
    Xil_Out16(0xff200002, 0x0000);
    //transmit "mw.w ff200004 0420^M"		;BC mode, 
    Xil_Out16(0xff200004, 0x0420);
    //transmit "mw.w ff200010 1060^M"		;expanded word, (unused RT stuff)
    Xil_Out16(0xff200010, 0x1060);
    //transmit "mw.w ff20001a 000A^M"		;comment
    Xil_Out16(0xff20001a, 0x000A);

    //transmit "mw.w ff220200 0000^M"		;SETUP: point at first block in stack
    Xil_Out16(0xff220200, 0x0000);
    //transmit "mw.w ff220202 FFFE^M"		;SETUP frame length (1's compl)
    Xil_Out16(0xff220202, 0xFFFE);

    /* STACK 1 */
    //transmit "mw.w ff220000 0000^M"		;STACK: reserved for response
    Xil_Out16(0xff220000, 0x0000);
    //transmit "mw.w ff220002 0000^M"		;STACK: time tag word will be inserted here
    Xil_Out16(0xff220002, 0x0000);
    //transmit "mw.w ff220004 00a0^M"		;STACK: Gap to next msg (in 100usecs)
    Xil_Out16(0xff220004, 0x00a0);
    //transmit "mw.w ff220006 0108^M"		;STACK: msg ptr
    Xil_Out16(0xff220006, 0x0108);

    /* MESSAGE 1 */
    //transmit "mw.w ff220210 FE80^M"		;MSG: BC ctrl word: use timetag, mask exceptions, use bus A
    Xil_Out16(0xff220210, 0xFE80);
    //transmit "mw.w ff220212 0024^M"		;MSG: cmd word (remote addr , TR, subaddr, woid count)
    Xil_Out16(0xff220212, 0x0024);
    //transmit "mw.w ff220214 1234^M"		;MSG: data
    Xil_Out16(0xff220214, 0x1234);
    //transmit "mw.w ff220216 5678^M"		;MSG: data
    Xil_Out16(0xff220216, 0x5678);
    //transmit "mw.w ff220218 9abc^M"		;MSG: data
    Xil_Out16(0xff220218, 0x9abc);
    //transmit "mw.w ff22021a def0^M"		;MSG: data
    Xil_Out16(0xff22021a, 0xdef0);

    //transmit "mw.w ff22021c 0000^M"		;MSG: clear for reply
    Xil_Out16(0xff22021c, 0x0000);
    //transmit "mw.w ff22021e 0000^M"
    Xil_Out16(0xff22021e, 0x0000);
    //transmit "mw.w ff220220 0000^M"
    Xil_Out16(0xff220220, 0x0000);
    //transmit "mw.w ff220222 0000^M"
    Xil_Out16(0xff220222, 0x0000);
    //transmit "mw.w ff220224 0000^M"
    Xil_Out16(0xff220224, 0x0000);
    //transmit "mw.w ff220226 0000^M"
    Xil_Out16(0xff220226, 0x0000);

    //transmit "mw.w ff200006 0042"		;trigger, stop on EOF
    Xil_Out16(0xff200006, 0x0042);

    /* Wait for EOM flag */
    time = get_timer_count(0);
	do {
        val = Xil_In16(0xff220000);
        // it takes 150us to send 5 words (CMD + 4 data words)
        // it should take 990us to send 33 words (CMD + 32 words)
		if (get_timer_count(time) > (unsigned long)(25000)) // 1ms
			break;
	} while ((val & 0x8000) == 0);
    time = get_timer_count(time);
    printf("%s (status=0x%04X, time=%lld ticks)\n", (val & 0x8000) ? "SENT" : "FAILED!", val, time);

    /* Good data block received by BC? */
    if (val & 0x10)
    {
        printf("\nResponse received: %04X %04X %04X %04X %04X %04X\n",
            Xil_In16(0xff22021c), Xil_In16(0xff22021e), Xil_In16(0xff220220),
            Xil_In16(0xff220222), Xil_In16(0xff220224), Xil_In16(0xff220226));
    }

    pause();
}


static void mm_test(void)
{
    u16 val, stack;

    printf("\nWaiting for messages... ");

    /* Setup */
    //w FF20000E 07D8 #SETUP: Set monitor command and data stack sizes (Config Reg. 3); Disable Illegalization
    Xil_Out16(0xFF20000E, 0x07D8);
    //w FF220204 0400 #MEMORY: Set initial command stack pointer to 0x400
    Xil_Out16(0xFF220204, 0x0300);
    //w FF220206 0500 #MEMORY: Set initial data stack pt to 0x500
    Xil_Out16(0xFF220206, 0x0400);

    //w FF220500 FFFF #MEMORY; Set to monitor all RTs and SAs
    //to FF2205FE
    for (val = 0; val < 0x80; ++val)
        Xil_Out16(0xFF220500 + (val * sizeof(u16)), 0xFFFF);

    //w FF200002 5C00 #SETUP: Set for MM only and enable MM; Unbusy (Config Reg. 1)
    Xil_Out16(0xff200030, 0x2030);

    /* Save the initial stack pointer */
    stack = Xil_In16(0xff220204);

    //r ff220204 #Command stack pointer. Will increment by 4 after message is processed. Poll on this.

    /* Wait for stack pointer to increment */
	do {
        val = Xil_In16(0xff220204);
		if (getchar() > 0)
		{
            puts("\nAborted");
            pause();
            return;
		}
	} while (val == stack);

    /* Got a message; Read block status word */
    val = Xil_In16(0xff220000 + (stack * sizeof(u16)));
    printf("RECEIVED (status=0x%04X)\n", val);

    /* EOM set? */
    if (val & 0x8000)
    {
        printf("\nData: %04X %04X %04X %04X %04X %04X\n",
            Xil_In16(0xff220800), Xil_In16(0xff220802), Xil_In16(0xff220804),
            Xil_In16(0xff220806), Xil_In16(0xff220808), Xil_In16(0xff22080A));
    }

    pause();
}


static void rt_test(void)
{
    u16 val, stack;

    printf("\nWaiting for messages... ");

    /* Setup */
    //w ff200002(0x001) 8800 #SETUP: Configuration Register 1 (Set Enhanced RT, MM disabled, busy bit set low)
    Xil_Out16(0xff200002, 0x8800);
    //w ff20000E(0x007) 8000 #SETUP: Configuration Register 3 (Always enhanced mode); Disable Illegalization
    Xil_Out16(0xff20000E, 0x8080);
    //w ff200012(0x009) 0002 #SETUP: Configuration Register 5 (Set RT Addr to 1)
    Xil_Out16(0xff200012, 0x0002);
    //w ff200030(0x018) 2030 #SETUP: Configuration Register 6 (RT Addr Source is from Config Register 5 and not hardware pins)
    Xil_Out16(0xff200030, 0x2030);

    //w ff220282(0x141) 0400 #MEMORY: set Rx SubAddress 1 Data Pointer to memory address ff220800
    Xil_Out16(0xff220282, 0x0400);
    //w ff220342(0x1A1) 0000 #MEMORY: set SA1 CW (single buffer mode)
    Xil_Out16(0xff220342, 0x0000);

    /* Save the initial stack pointer */
    stack = Xil_In16(0xff220200);

    //w ff200002(0x001) 8C00 #SETUP: Configuration Register 1 (busy bit set high)
    Xil_Out16(0xff200002, 0x8C00);

    //r ff220200(0x100)      #Command stack pointer. Will increment by 4 after Rx message is processed. Poll on this.

    /* Wait for stack pointer to increment */
	do {
        val = Xil_In16(0xff220200);
		if (getchar() > 0)
		{
            puts("\nAborted");
            pause();
            return;
		}
	} while (val == stack);

    /* Got a message; Read block status word */
    val = Xil_In16(0xff220000 | stack);
    printf("RECEIVED (status=0x%04X)\n", val);

    /* EOM set? */
    if (val & 0x8000)
    {
        printf("\nData: %04X %04X %04X %04X %04X %04X\n",
            Xil_In16(0xff220800), Xil_In16(0xff220802), Xil_In16(0xff220804),
            Xil_In16(0xff220806), Xil_In16(0xff220808), Xil_In16(0xff22080A));
    }

    pause();
}

#endif // #if SERIAL_DEBUG

