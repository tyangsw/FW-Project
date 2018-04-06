/*
 * CBx module test menu.
 *
 * Copyright (C) 2014 North Atlantic Industries, Inc.
 */

#if SERIAL_DEBUG

#include <includes.h>

/* External functions (see module.h) */
extern bool can_self_test(CHANNEL ch);
extern bool can_reset_channel_test(CHANNEL ch);
extern void can_force_interrupt_on_channel(CHANNEL ch);
extern bool can_set_baud_to_250(CHANNEL ch);
extern bool can_set_baud_to_500(CHANNEL ch);
extern void can_dump_bit_timing(CHANNEL ch);
extern void can_dump_error_status(CHANNEL ch);
extern void can_dump_filter_config(CHANNEL ch);

/* Local Common functions */
static void do_can_self_test(void);
static CHANNEL get_channel_choice(bool bRestrict);
static void do_reset_channels_test(void);
static void do_can_force_interrupt(CHANNEL ch);
static void do_can_set_baud_to_250(void);
static void do_can_set_baud_to_500(void);
static void do_can_dump_bit_timing(CHANNEL ch);
static void do_can_dump_error_status(CHANNEL ch);
static void do_can_dump_filter_config(CHANNEL ch);

#if (MODULE_CB2 || MODULE_CB3)
	/* External functions (see module.h) */
	extern bool canj1939_tx_test(CHANNEL ch);
	extern bool canj1939_tx_flood_test(CHANNEL ch, int nMsgCount);
	extern bool canj1939_rx_test(CHANNEL ch);
	extern bool can_j1939_filter_test(CHANNEL ch);

	/* Local J1939 functions */
	static void do_canj1939_tx_test(CHANNEL ch);
	static void do_canj1939_tx_flood_test(CHANNEL ch, int nMsgCount);
	static void do_canj1939_rx_test(void);
	static void do_canj1939_filter_test(CHANNEL ch);
#endif

#if (MODULE_CB1 || MODULE_CB3)
	/* External functions (see module.h) */
	extern bool can_set_baud_to_1Mb(CHANNEL ch);
	extern bool canA_host_tx_test(CHANNEL ch);
	extern bool canA_host_rx_test(CHANNEL ch);
	extern bool canB_host_tx_test(CHANNEL ch);
	extern bool canB_host_rx_test(CHANNEL ch);
	extern bool canB_host_tx_flood_test(CHANNEL ch, int nMsgCount);
	extern bool canAB_filter_test(CHANNEL ch);
	extern bool canAB_module_round_trip_test(CHANNEL ch, int nMsgCount);

	/* Local CAN AB functions */
	static void do_can_set_baud_to_1Mb(void);
	static void do_canA_host_tx_test(CHANNEL ch);
	static void do_canA_host_rx_test(void);
	static void do_canB_host_tx_test(CHANNEL ch);
	static void do_canB_host_rx_test(void);
	static void do_canB_host_tx_flood_test(CHANNEL ch, int nMsgCount);
	static void do_canAB_filter_test(CHANNEL ch);
	static void do_canAB_module_round_trip_test(CHANNEL ch, int nMsgCount);
#endif


/**************************** Exported functions ****************************/

void module_menu(void)
{
    bool    exit_menu = FALSE;
    char    *inbuf;
    CHANNEL ch;

    /* Disable all interrupts while we are in debug mode */
    can_disable_interrupts();

    /* Print menu choices */
    while (exit_menu == FALSE)
    {
    	puts("--------------------------------");
        printf("\n\n>>>  %s Menu  <<<\n\n", MODULE_NAME);
        puts("1. CAN Self Test");
#if (MODULE_CB2 || MODULE_CB3)
        puts("2. CAN-J1939 TX Test");
        puts("3. CAN-J1939 RX Test");
        puts("4. CAN-J1939 TX Flood Test");
        puts("5. CAN-J1939 Filter Test");
#endif
        puts("--------------------------------");
        puts("7. CAN Reset Channels Test");
        puts("8. CAN Force Interrupt");
        puts("--------------------------------");
        puts("a. Set Baud Rate to 250K");
        puts("b. Set Baud Rate to 500K");
#if (MODULE_CB1 || MODULE_CB3)
        puts("c. Set Baud Rate to 1Mb");
        puts("--------------------------------");
        puts("g. CAN-A Host TX Test");
        puts("h. CAN-A Host RX Test");
        puts("i. CAN-B Host TX Test");
        puts("j. CAN-B Host RX Test");
        puts("k. CAN-B Host TX Flood Test");
        puts("l. CAN-AB Filter Test");
        puts("m. CAN-AB On Module FIFO Test");
#endif
        puts("--------------------------------");
        puts("x. Dump Filter Config");
        puts("y. Dump Bit Timing");
        puts("z. Dump Error Status");
        puts("--------------------------------");
        puts("M. Main menu");
        puts("--------------------------------");
        printf("\nEnter Selection: ");
        inbuf = pend_rx_line();
        if (!inbuf || (strlen(inbuf) > 1))
            continue;

        switch (tolower((int)*inbuf))
        {
            case '1':
                do_can_self_test();
                break;
#if (MODULE_CB2 || MODULE_CB3)
            case '2':
            	ch = get_channel_choice(TRUE);
            	do_canj1939_tx_test(ch);
                break;
            case '3':
            	do_canj1939_rx_test();
                break;
            case '4':
            	ch = get_channel_choice(FALSE);

				printf("\nEnter Msg Count: ");
				inbuf = pend_rx_line();
				if (!inbuf || (strlen(inbuf) <= 0))
					printf("\nInvalid Msg Count Option");
				else
					do_canj1939_tx_flood_test(ch, atoi(inbuf));
            	break;
            case '5':
            	ch = get_channel_choice(TRUE);
            	do_canj1939_filter_test(ch);
            	break;
#endif
            case '7':
            	do_reset_channels_test();
            	break;
            case '8':
            	ch = get_channel_choice(TRUE);
            	do_can_force_interrupt(ch);
            	break;
            case 'a':
            	do_can_set_baud_to_250();
            	break;
            case 'b':
            	do_can_set_baud_to_500();
            	break;
#if (MODULE_CB1 || MODULE_CB3)
            case 'c':
            	do_can_set_baud_to_1Mb();
            	break;

            case 'g':
            	ch = get_channel_choice(TRUE);
                do_canA_host_tx_test(ch);
                break;
            case 'h':
                do_canA_host_rx_test();
                break;
            case 'i':
            	ch = get_channel_choice(FALSE);
                do_canB_host_tx_test(ch);
                break;
            case 'j':
               	do_canB_host_rx_test();
                break;
            case 'k':
            	ch = get_channel_choice(TRUE);

				printf("\nEnter Msg Count: ");
				inbuf = pend_rx_line();
				if (!inbuf || (strlen(inbuf) <= 0))
					printf("\nInvalid Msg Count Option");
				else
					do_canB_host_tx_flood_test(ch, atoi(inbuf));
            	break;
            case 'l':
            	ch = get_channel_choice(TRUE);
            	do_canAB_filter_test(ch);
            	break;
            case 'n':
            	ch = get_channel_choice(FALSE);
				printf("\nEnter Msg Count: ");
				inbuf = pend_rx_line();
				if (!inbuf || (strlen(inbuf) <= 0))
					printf("\nInvalid Msg Count Option");
				else
					do_canAB_module_round_trip_test(ch, atoi(inbuf));
            	break;
#endif
            case 'x':
            	ch = get_channel_choice(FALSE);
            	do_can_dump_filter_config(ch);
            	break;
            case 'y':
            	ch = get_channel_choice(FALSE);
            	do_can_dump_bit_timing(ch);
            	break;
            case 'z':
            	ch = get_channel_choice(FALSE);
            	do_can_dump_error_status(ch);
            	break;
            case 'm':
                exit_menu = TRUE;
                can_enable_interrupts(); /* Re-enable interrupts now that we are exiting debug menu */
                break;
        }
    }
}

static CHANNEL get_channel_choice(bool bRestrict)
{
    char *inbuf;
    int nTemp = 0;
    bool bDone = FALSE;
    CHANNEL ch = 0;

	do
	{
		printf("\nEnter Channel ( 1 - 8 ): ");
		if (!bRestrict)
			printf("\n   or enter 0 for ALL channels: ");

		inbuf = pend_rx_line();
		if (!inbuf || (strlen(inbuf) <= 0))
			printf("\nInvalid Channel Option");
		else
		{
			nTemp = atoi(inbuf);
			if (bRestrict && (nTemp < 1 || nTemp > NUM_CHANNELS))
				printf("\nInvalid Channel Option - must be a value between 1 - 8");
			else
				bDone = TRUE;
		}
	} while (!bDone);

	if (nTemp == 0)
		ch = ALL_CHANNELS;
	else
		ch = (CHANNEL)(nTemp - 1);
    return (ch);
}

/****************************** Local functions *****************************/

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
do_can_self_test is responsible for performing a simple CAN tranmit and receive loopback test on all channels.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void do_can_self_test(void)
{
	bool bSuccess = FALSE;
	int i = 0;

	for (i=0; i < NUM_CHANNELS; i++)
	{
		bSuccess = can_self_test(i);

		if (bSuccess)
			printf("Self Test for Channel %d PASSED\n", (i + 1));
		else
			printf("Self Test for Channel %d FAILED\n", (i + 1));
	}
//	pause();
}

#if (MODULE_CB2 || MODULE_CB3)
/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
do_canj1939_tx_test is responsible for performing a simple CAN-J1939 transmit test on the specified channel.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void do_canj1939_tx_test(CHANNEL ch)
{
	bool bSuccess = TRUE;

	bSuccess = canj1939_tx_test(ch);
	if (bSuccess)
		printf("CAN-J1939 TX Test for Channel %d PASSED\n", (ch + 1));
	else
		printf("CAN-J1939 TX Test for Channel %d FAILED\n", (ch + 1));
}

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
do_canj1939_tx_flood_test is responsible for performing J1939 TX test of a specified number of J1939 messages
being sent on the desired channel.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void do_canj1939_tx_flood_test(CHANNEL ch, int nMsgCount)
{
	bool bSuccess = TRUE;
	bSuccess = canj1939_tx_flood_test(ch, nMsgCount);
	if (bSuccess)
	{
		if (ch == ALL_CHANNELS)
			printf("CAN-J1939 TX Flood Test for ALL Channels PASSED\n");
		else
			printf("CAN-J1939 TX Flood Test for Channel %d PASSED\n", (ch + 1));
	}
	else
	{
		if (ch == ALL_CHANNELS)
			printf("CAN-J1939 TX Flood Test for ALL Channels FAILED\n");
		else
			printf("CAN-J1939 TX Flood Test for Channel %d FAILED\n", (ch + 1));
	}
}

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
do_canj1939_rx_test is responsible for performing a simple CAN-J1939 receive test on all channels.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void do_canj1939_rx_test(void)
{
	bool bSuccess = TRUE;
	int i = 0;

	for (i=0; i < NUM_CHANNELS; i++)
	{
		bSuccess = canj1939_rx_test(i);
		if (bSuccess)
			printf("CAN-J1939 RX Test for Channel %d PASSED\n", (i + 1));
		else
			printf("CAN-J1939 RX Test for Channel %d FAILED\n", (i + 1));
	}
}

static void do_canj1939_filter_test(CHANNEL ch)
{
	if (ch < 7)
		printf("NOTE: CAN J1939 Filter Test Assumes channel %d is wrapped to channel %d!\n", (ch + 1), (ch + 2));
	else /* Must be dealing with channel 8 */
		printf("NOTE: CAN J1939 Filter Test Assumes channel %d is wrapped to channel %d!\n", 8, 1);

	if (canj1939_filter_test(ch))
		printf("CAN J1939 Filter Test for Channel %d PASSED\n", (ch + 1));
	else
		printf("CAN J1939 Filter Test for Channel %d FAILED\n", (ch + 1));
}
#endif /* #if (MODULE_CB2 || MODULE_CB3)  */

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
do_reset_channels_test is responsible for performing a channel reset on all channels.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void do_reset_channels_test(void)
{
	bool bSuccess = TRUE;
	int i = 0;

	for (i=0; i < NUM_CHANNELS; i++)
	{
#ifdef _VERBOSE
		printf("------------------------------------------------------------\r\n");
#endif
		bSuccess = can_reset_channel_test(i);
		if (bSuccess)
			printf("CAN Reset of Channel %d PASSED\n", (i + 1));
		else
			printf("CAN Reset of Channel %d FAILED\n", (i + 1));
	}
#ifdef _VERBOSE
	printf("------------------------------------------------------------\r\n");
#endif
//	pause();
}

static void do_can_force_interrupt(CHANNEL ch)
{
	can_force_interrupt_on_channel(ch);
}
//#endif

static void do_can_dump_bit_timing(CHANNEL ch)
{
	can_dump_bit_timing(ch);
}

static void do_can_dump_error_status(CHANNEL ch)
{
	can_dump_error_status(ch);
}

static void do_can_set_baud_to_250()
{
	int i = 0;
	bool bSuccess = TRUE;

	for (i=0; i < NUM_CHANNELS; i++)
	{
		if (can_set_baud_to_250(i))
			printf("CAN Set Baud to 250 for Channel %d PASSED\n", (i + 1));
		else
		{
			printf("CAN Set Baud to 250 for Channel %d FAILED\n", (i + 1));
			bSuccess = FALSE;
		}
	}

	if (bSuccess)
		printf("CAN Set Baud to 250 Test PASSED\n");
	else
		printf("CAN Set Baud to 250 Test FAILED\n");
}

static void do_can_set_baud_to_500()
{
	int i = 0;
	bool bSuccess = TRUE;

	for (i=0; i < NUM_CHANNELS; i++)
	{
		if (can_set_baud_to_500(i))
			printf("CAN Set Baud to 500 for Channel %d PASSED\n", (i + 1));
		else
			printf("CAN Set Baud to 500 for Channel %d FAILED\n", (i + 1));
	}

	if (bSuccess)
		printf("CAN Set Baud to 500 Test PASSED\n");
	else
		printf("CAN Set Baud to 500 Test FAILED\n");
}


static void do_can_dump_filter_config(CHANNEL ch)
{
	int i = 0;

	if (ch == ALL_CHANNELS)
	{
		for (i=0; i < NUM_CHANNELS; i++)
			can_dump_filter_config(i);
	}
	else
		can_dump_filter_config(ch);
}


#if (MODULE_CB1 || MODULE_CB3)
static void do_can_set_baud_to_1Mb()
{
	int i = 0;
	bool bSuccess = TRUE;

	for (i=0; i < NUM_CHANNELS; i++)
	{
		if (can_set_baud_to_1Mb(i))
			printf("CAN Set Baud to 1Mb for Channel %d PASSED\n", (i + 1));
		else
			printf("CAN Set Baud to 1Mb for Channel %d FAILED\n", (i + 1));
	}

	if (bSuccess)
		printf("CAN Set Baud to 1Mb Test PASSED\n");
	else
		printf("CAN Set Baud to 1Mb Test FAILED\n");
}

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
do_canA_host_tx_test is responsible for performing a simple CAN-A transmit test on Channel 1.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void do_canA_host_tx_test(CHANNEL ch)
{
	bool bSuccess = TRUE;
	bSuccess = canA_host_tx_test(ch);
	if (bSuccess)
		printf("CAN-A TX Test for Channel %d PASSED\n", (ch + 1));
	else
		printf("CAN-A TX Test for Channel %d FAILED\n", (ch + 1));
}

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
do_canA_host_rx_test is responsible for performing a simple CAN-A receive test on all channels.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void do_canA_host_rx_test(void)
{
	bool bSuccess = TRUE;
	int i = 0;

	for (i=0; i < NUM_CHANNELS; i++)
	{
		bSuccess = canA_host_rx_test(i);
		if (bSuccess)
			printf("CAN-A RX Test for Channel %d PASSED\n", (i + 1));
		else
			printf("CAN-A RX Test for Channel %d FAILED\n", (i + 1));
	}
}

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
do_canB_host_tx_test is responsible for performing a simple CAN-B transmit test on Channel 1.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void do_canB_host_tx_test(CHANNEL ch)
{
	bool bSuccess = TRUE;

	bSuccess = canB_host_tx_test(ch);
	if (bSuccess)
		printf("CAN-B TX Test for Channel %d PASSED\n", (ch + 1));
	else
		printf("CAN-B TX Test for Channel %d FAILED\n", (ch + 1));
}

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
do_canB_host_rx_test is responsible for performing a simple CAN-B receive test on all channels.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void do_canB_host_rx_test(void)
{
	bool bSuccess = TRUE;
	int i = 0;

	for (i=0; i < NUM_CHANNELS; i++)
	{
		bSuccess = canB_host_rx_test(i);
		if (bSuccess)
			printf("CAN-B RX Test for Channel %d PASSED\n", (i + 1));
		else
			printf("CAN-B RX Test for Channel %d FAILED\n", (i + 1));
	}
}

static void do_canB_host_tx_flood_test(CHANNEL ch, int nMsgCount)
{
	bool bSuccess = TRUE;
	bSuccess = canB_host_tx_flood_test(ch, nMsgCount);
	if (bSuccess)
		printf("CAN-B TX Flood Test for Channel %d PASSED\n", (ch + 1));
	else
		printf("CAN-B TX Flood Test for Channel %d FAILED\n", (ch + 1));
}

static void do_canAB_filter_test(CHANNEL ch)
{
	printf("NOTE: CAN AB Filter Test Assumes channel %d is wrapped to channel %d!", (ch + 1), (ch + 2));
	if (canAB_filter_test(ch))
		printf("CAN AB Filter Test for Channel %d PASSED\n", (ch + 1));
	else
		printf("CAN AB Filter Test for Channel %d FAILED\n", (ch + 1));
}

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
do_canAB_module_round_trip_test is responsible for checking to see if the tx fifo, rx fifo and CAN interrupts are
all working from the module perspective. A set of pre-defined CAN messages will be sent out on the specified
channel (via the TX FIFO), routed through the CAN and the CAN interrupts will force the data into the receiving
channels RX FIFO. NOTE: test expects that Channel 1 is looped to Channel 2, Channel 3 is looped to Channel 4,
Channel 5 is looped to Channel 6 and Channel 7 is looped to Channel 8.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void do_canAB_module_round_trip_test(CHANNEL ch, int nMsgCount)
{
	if (canAB_module_round_trip_test(ch, nMsgCount))
		printf("CAN AB Module Round Trip Test for Channel %d PASSED\n", (ch + 1));
	else
		printf("CAN AB Module Round Trip Test for Channel %d FAILED\n", (ch + 1));
}
#endif /* #if (MODULE_CB1 || MODULE_CB3) */
#endif // #if SERIAL_DEBUG
