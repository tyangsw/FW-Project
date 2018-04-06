/*
 * CBx module main code.
 *
 * Copyright (C) 2014 North Atlantic Industries, Inc.
 */

#include <includes.h>
#include <string.h>

/* CAN specific header files */
#include "bits.h"
#include "can.h"
#include "dpram_addresses.h"
#include "fpga_addresses.h"
#include "module.h"

#if (MODULE_CB2 || MODULE_CB3)
	#include "ssJ1939\j1939.h"
	#include "ssJ1939\j1939app.h"
#endif

//*****************************************************************************
#define _ENABLE_INTERRUPTS 1
//#define _TRY_TO_AVOID_SURGE 1
//#define _INTERRUPT_VERBOSE 1
//#define _FILTER_VERBOSE 1

#define _MAX_NODES 255
//*****************************************************************************

/* Local functions */
static void init(CHANNEL ch);
static int bist(CHANNEL ch);
static void load_params(CHANNEL ch);

/* Local functions for CAN module */
#ifdef _ENABLE_INTERRUPTS
static int init_module_interrupts(void);
#endif

static void update_ctrl_register_with_protocol_enable_state(CHANNEL ch);
static void can_init_baud(CHANNEL ch);
static void check_diag_mode(void);
static void check_for_config_changes(CHANNEL ch);
static void check_for_port_configuration_change(CHANNEL ch);
static void check_for_self_test(CHANNEL ch);
static void check_for_baud_change(CHANNEL ch);
static void check_for_reset(CHANNEL ch);
static void check_for_acceptance_filter_change(CHANNEL ch);
static void check_for_tx(CHANNEL ch);
static void	reset_channel(CHANNEL ch, uint8_t ucPrescaler, uint8_t ucSJW, uint8_t ucTimeSeg2, uint8_t ucTimeSeg1, bool bClearStatus);
static uint32_t getXilinxAcceptanceFilterMaskFromFilterControl(CHANNEL ch, uint32_t filter, uint32_t filterControlRegValue);
static uint32_t getXilinxAcceptanceFilterIDFromFilterControl(CHANNEL ch, uint32_t filter, uint32_t filterControlRegValue);
static bool changes_made_to_filter_mask_or_code(CHANNEL ch, uint32_t filterControlRegValue);

#ifdef _ENABLE_INTERRUPTS
static uint8_t can_intrHandler(CHANNEL ch);
static void can_chan1_irq_handler(void *context);
static void can_chan2_irq_handler(void *context);
#ifdef _ALL_CHANNELS
static void can_chan3_irq_handler(void *context);
static void can_chan4_irq_handler(void *context);
static void can_chan5_irq_handler(void *context);
static void can_chan6_irq_handler(void *context);
static void can_chan7_irq_handler(void *context);
static void can_chan8_irq_handler(void *context);
#endif /* _ALL_CHANNELS */
#endif /* _ENABLE_INTERRUPTS */


int   fill_AB_RX_UIF(can_t *rx_m, CHANNEL ch);
int   fill_AB_TX_UIF(can_t *tx_m, CHANNEL ch);
void  wait_microsecond(unsigned int microseconds);
void  print_AB_packet(can_t *pCAN_pkt);
void  mirror_status_register(CHANNEL ch);
void  clear_all_reported_status(CHANNEL ch);

/* Global variables */
static uint32_t g_InterruptMask = (CAN_IXR_ERROR_MASK | CAN_IXR_RXNEMP_MASK); /* Only interested in Error interrupts and RX! */
static bool ch_init[NUM_CHANNELS] = { FALSE };
struct RX_CONTROL RX_UIF[NUM_CHANNELS];
static bool Slf_Test[NUM_CHANNELS];
static bool Slf_Test_passed[NUM_CHANNELS];


/* Each channel could be configured for different CAN protocols */
static uint8_t protocol_can_ab = 0x01;
static uint8_t protocol_can_j1939 = 0x02;
static uint8_t port_protocol_type[NUM_CHANNELS];
static uint32_t LastPendingIntr[NUM_CHANNELS] = {0,0,0,0,0,0,0,0};
static uint32_t PendingIntrCount[NUM_CHANNELS] = {0,0,0,0,0,0,0,0};
static XTime ErrorTiming[NUM_CHANNELS] = {0,0,0,0,0,0,0,0};


#if (MODULE_CB2 || MODULE_CB3)
static uint8_t Src_Address[NUM_CHANNELS];
static uint8_t SA_Attempts[J1939CFG_PORTS_NUM] = {0,0,0,0,0,0,0,0};

static void check_for_address_change(CHANNEL ch);
int   fill_J1939_RX_UIF(j1939_t *rx_m, CHANNEL ch);
int   fill_J1939_TX_UIF(j1939_t *tx_m, CHANNEL ch);
void  print_J1939_packet(j1939_t *pCAN_pkt);
#endif


/**************************** Exported functions ****************************/

void can_enable_interrupts()
{
	init_module_interrupts();
}

void can_disable_interrupts()
{
	can_interruptDisableAll(CAN_IXR_ALL);
}


/**************************************************************************************************************/
/**
\ingroup CANInitialization
<summary>
update_ctrl_register_with_protocol_enable_state is responsible for determining which CAN functionality should be "unlocked".
</summary>
<returns>uint8_t : 1 = J1939 capable; 0 = NOT capable</returns>
*/
/**************************************************************************************************************/
static void update_ctrl_register_with_protocol_enable_state(CHANNEL ch)
{
	/* Update the control register to reflect what CAN protocol the specified channel is currently configured to */
	volatile unsigned int *pDPRAM_CONFIG  = (volatile unsigned int *)(DPRAM_CH1_CONTROL   + (ch * CHANNEL_DEDICATED_SIZE));
	uint8_t protocol_config = 0x00;

	if (port_protocol_type[ch] == protocol_can_j1939)
		protocol_config = CTRL_CONFIG_ENABLE_J1939;
	else if (port_protocol_type[ch] == protocol_can_ab)
		protocol_config = CTRL_CONFIG_ENABLE_AB;

	*pDPRAM_CONFIG |= protocol_config;
}

/**************************************************************************************************************/
/**
\ingroup CANInitialization
<summary>
module_init is responsible for initializing this CAN module (including the initialization of all available
channels.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
void module_init(void)
{
    CHANNEL ch;

#ifdef _FPGA_IMPLEMENTED
    volatile unsigned int *pFPGA_FIFO_CONTROL = (volatile unsigned int *)(FPGA_FIFO_CONTROL);
    volatile unsigned int *pFPGA_HPS_CONTROL = (volatile unsigned int *)(FPGA_HPS_CONTROL);
    volatile unsigned int *pFPGA_PS_ENABLE = (volatile unsigned int *)(FPGA_PS_ENABLE);
#endif  /* _FPGA_IMPLEMENTED */

#ifdef _VERBOSE
    printf("In module_init() \r\n");
#endif /*_VERBOSE */

	memset(&ch_init, 0, NUM_CHANNELS);
	memset(&Slf_Test, 0 , NUM_CHANNELS);
	memset(&Slf_Test_passed, 0, NUM_CHANNELS);

#ifdef _FPGA_IMPLEMENTED
    /* Initialize the FIFOs */
    *pFPGA_FIFO_CONTROL = 0x2;
    *pFPGA_FIFO_CONTROL = 0x0;
    *pFPGA_FIFO_CONTROL = 0x1;
    *pFPGA_HPS_CONTROL  = 0xFFFF;

	/* Turn on the 1st set of Channels */
#ifdef _TRY_TO_AVOID_SURGE
	for (ch = CHANNEL_1; ch < (NUM_CHANNELS/2); ++ch)
	{
		*pFPGA_PS_ENABLE |= (0x0001 << ch);
		wait_microsecond(1000); /* Wait so we do not cause a power surge */
	}
#else
	/* Turn on all 8 channels */
	*pFPGA_PS_ENABLE =0x000F;
#endif /* _TRY_TO_AVOID_SURGE */
#endif /* _FPGA_IMPLEMENTED */

    /* Initialize module for J1939 (OK to do this even if no channels configured for J1939!) */
#if (MODULE_CB2 || MODULE_CB3)
	j1939_init();
	j1939_enable_all_ports(1); /* By default we enable all ports for J1939 if this is a CB2..else we don't enable any ports for J1939 */
	memset(port_protocol_type, protocol_can_j1939, (sizeof(port_protocol_type)/sizeof(uint8_t)));
#else
   	memset(port_protocol_type, protocol_can_ab, (sizeof(port_protocol_type)/sizeof(uint8_t)));
#endif

    /* Initialize all channels */
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
        init(ch);

#ifdef _FPGA_IMPLEMENTED
    *pFPGA_HPS_CONTROL = 0xFFFF;
#endif /* _FPGA_IMPLEMENTED */

#ifdef _ENABLE_INTERRUPTS
    init_module_interrupts();
#endif /* _ENABLE_INTERRUPTS */
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
module_main is responsible for the main CAN functionality loop. Each channel that has been initialized is
checked for updated baud rates, checked for CAN msgs awaiting to be transmitted and checked for CAN msgs
awaiting to be received.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
void module_main(void)
{
#if (MODULE_CB2 || MODULE_CB3)
    static XTime time;
    static bool  time_to_update = TRUE;

	if (j1939_is_any_port_enabled())
	{
		/* We need to call J1939 at a fixed periodic interval (even when in menu mode) .. 25ms or less is recommended */
		if (time_to_update)
		{
			j1939_update();

			/* Restart timer */
			time_to_update = FALSE;
			time = get_timer(0);
		}

		/* Time to read? (every 5 ms) */
		if (get_timer(time) > 5)
			time_to_update = TRUE;
	}
#endif

    CHANNEL ch;
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
        if (ch_init[ch])
        {
#ifdef _FPGA_IMPLEMENTED
        	/* Check/update all CAN configuration parameters */
        	check_for_config_changes(ch);

            /* Check for tx request */
            check_for_tx(ch);

            /* mirror status register to outside world */
            mirror_status_register(ch);

            /* Check for diagnostic mode */
            check_diag_mode();
#endif /* _FPGA_IMPLEMENTED */
        }
    }
}

/****************************** Local functions *****************************/

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
check_for_self_test is responsible for checking to see if user requested a self test on the given channel.
If so, a loopback self test is performed on the given channel and results are posted to the bit status register.
</summary>
<param name="ch"> : (Input) Channel index to check to see if self test should be run.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void check_for_self_test(CHANNEL ch)
{
	volatile unsigned int *pDPRAM_CONFIG  = (volatile unsigned int *)(DPRAM_CH1_CONTROL   + (ch * CHANNEL_DEDICATED_SIZE));

	if (*pDPRAM_CONFIG & CTRL_CONFIG_BIT)
	{
#ifdef _VERBOSE
		printf("BIT was requested for channel %d\n", (ch+1));
#endif
		/* Run the self test for the specified channel */
		can_self_test(ch);

		/* Make sure to clear the BIT bit since user may be waiting for BIT completion (clearing the bit signifies BIT has completed) */
		*pDPRAM_CONFIG &= ~CTRL_CONFIG_BIT;
	}
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
init is responsible for initializing the specified channel.
</summary>
<param name="ch"> : (Input) Channel index to be initialized (zero based index).</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void init(CHANNEL ch)
{
#ifdef _VERBOSE
	printf("In init(ch) - channel = %x\r\n", ch);
#endif /* _VERBOSE */

#ifdef _FPGA_IMPLEMENTED
	/* Initialize the FIFOs for the specified channel */
    volatile unsigned int *pDDR_TX_FIFO_START_ADDR = (volatile unsigned int *)(FPGA_CH1_DDR_TX_FIFO_START_ADDR + (ch * FPGA_CHANNEL_DEDICATED_SIZE));
    volatile unsigned int *pDDR_RX_FIFO_START_ADDR = (volatile unsigned int *)(FPGA_CH1_DDR_RX_FIFO_START_ADDR + (ch * FPGA_CHANNEL_DEDICATED_SIZE));
    volatile unsigned int *pDDR_TX_FIFO_FULL_COUNT = (volatile unsigned int *)(FPGA_CH1_DDR_TX_FIFO_FULL_COUNT + (ch * FPGA_CHANNEL_DEDICATED_SIZE));
    volatile unsigned int *pDDR_RX_FIFO_FULL_COUNT = (volatile unsigned int *)(FPGA_CH1_DDR_RX_FIFO_FULL_COUNT + (ch * FPGA_CHANNEL_DEDICATED_SIZE));

    /* NOTE: In the future...it is expected the start and end FIFO addresses along with full counts will be set externally */
    switch (ch)
    {
    case CHANNEL_1 :
    	*pDDR_TX_FIFO_START_ADDR = 0x0010;
    	*pDDR_RX_FIFO_START_ADDR = 0x0011;
    	break;
    case CHANNEL_2 :
    	*pDDR_TX_FIFO_START_ADDR = 0x0012;
    	*pDDR_RX_FIFO_START_ADDR = 0x0013;
    	break;
#ifdef _ALL_CHANNELS
    case CHANNEL_3 :
    	*pDDR_TX_FIFO_START_ADDR = 0x0014;
    	*pDDR_RX_FIFO_START_ADDR = 0x0015;
    	break;
    case CHANNEL_4 :
    	*pDDR_TX_FIFO_START_ADDR = 0x0016;
    	*pDDR_RX_FIFO_START_ADDR = 0x0017;
    	break;
    case CHANNEL_5 :
    	*pDDR_TX_FIFO_START_ADDR = 0x0018;
    	*pDDR_RX_FIFO_START_ADDR = 0x0019;
    	break;
    case CHANNEL_6 :
    	*pDDR_TX_FIFO_START_ADDR = 0x001A;
    	*pDDR_RX_FIFO_START_ADDR = 0x001B;
    	break;
    case CHANNEL_7 :
    	*pDDR_TX_FIFO_START_ADDR = 0x001C;
    	*pDDR_RX_FIFO_START_ADDR = 0x001D;
    	break;
    case CHANNEL_8 :
    	*pDDR_TX_FIFO_START_ADDR = 0x001E;
    	*pDDR_RX_FIFO_START_ADDR = 0x001F;
    	break;
#endif /* _ALL_CHANNELS */
    default :
    	break;
    }

    *pDDR_TX_FIFO_FULL_COUNT = 0xFFFFF;
    *pDDR_RX_FIFO_FULL_COUNT = 0xFFFFF;
#endif /* _FPGA_IMPLEMENTED */

#ifdef _VERBOSE
	printf("BEFORE can_init_baud(ch) \r\n");
#endif /* _VERBOSE */

	/* Call init with desired initial baud rate.*/
	can_init_baud(ch);

#ifdef _VERBOSE
	printf("AFTER can_init_baud(ch) \r\n");
#endif /*VERBOSE */

    /* Run BIST */
    if (bist(ch) == CAN_SUCCESS)
    {
        /* Load configuration parameters */
        load_params(ch);

        /* Channel is now initialized */
        ch_init[ch] = TRUE;
    }

    /* Start out making sure all status registers are clear */
    clear_all_reported_status(ch);

    /* Make sure control register reflects each channel's current protocol configuration */
    update_ctrl_register_with_protocol_enable_state(ch);

#ifdef _VERBOSE
        printf("END In int(ch) \r\n");
#endif /* _VERBOSE */
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
can_init_baud is responsible for initializing the baud rate for the specified channel.
</summary>
<param name="ch"> : (Input) Channel index to be initialized (zero based index).</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
void can_init_baud(CHANNEL ch)
{
#if (MODULE_CB2 || MODULE_CB3)
	/* If the port is enabled for J1939...we use J1939 timings..else we use CAN A/B timings. */
	if(port_protocol_type[ch] == protocol_can_j1939)
	{
		/* Call init with desired initial baud rate.  (For J1939 this is 500K at a sample point of 87.5% */
		can_init_with_baud(ch, CAN_J1939_BRP_500K, CAN_J1939_SJW_500K, CAN_J1939_TSEG2_500K, CAN_J1939_TSEG1_500K);
	}
	/* Check if normal CAN AB enabled */
	else if(port_protocol_type[ch] == protocol_can_ab)
	{
		can_init_with_baud(ch, CAN_BRP_500K, CAN_SJW_500K, CAN_TSEG2_500K, CAN_TSEG1_500K);
	}
#else
	can_init_with_baud(ch, CAN_BRP_500K, CAN_SJW_500K, CAN_TSEG2_500K, CAN_TSEG1_500K);
#endif
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
bist is responsible for executing a quick CAN transmit and receive sequence for the given CAN channel.
</summary>
<param name="ch"> : (Input) Channel index (zero based index) to be tested.</param>
<returns>int : Status
	- 0  : SUCCESS
	- Non-Zero : ERROR
</returns>
*/
/**************************************************************************************************************/
static int bist(CHANNEL ch)
{
	int nStatus = CAN_SUCCESS;

    /* Perform simple loopback to make sure CAN channel is working... */
	if (!can_self_test(ch))
		nStatus = CAN_LOOPBACK_TEST_ERROR;

    return(nStatus);
}

#ifdef _ENABLE_INTERRUPTS
/**************************************************************************************************************/
/**
\ingroup CAN_Interrupts
<summary>
init_module_interrupts is responsible for initializing all CAN interrupts by assigning a unique IRQ to each.
</summary>
<returns>int : Status
	- 0  : SUCCESS
	- Non-Zero : ERROR
</returns>
*/
/**************************************************************************************************************/
static int init_module_interrupts(void)
{
	int nStatus = CAN_SUCCESS;
	CHANNEL ch;

	/* Make sure interrupts are disabled to start */
	nStatus = can_interruptDisableAll(CAN_IXR_ALL);
	if (nStatus != CAN_SUCCESS)
		printf("Interrupt Clear All Request Failed!\n");

	/* Make sure interrupts are all cleared to start */
	nStatus = can_interruptClearAll(CAN_IXR_ALL);
    if (nStatus != CAN_SUCCESS)
    	printf("Interrupt Clear All Request Failed!\n");

    if (nStatus != CAN_SUCCESS)
    	return nStatus;

    XStatus rc;
	if (ch_init[CHANNEL_1] == TRUE)
	{
		/* Register CAN Channel 1 Msg IRQ handler */
		rc = irq_register(XPAR_FABRIC_SYSTEM_IRQ_F2P0_INTR, can_chan1_irq_handler, NULL);
		if (rc)
		{
			printf("IRQ0 - irq_register() failed with rc = %d\n", (int)rc);
			nStatus = CAN_IRQ_REGISTER_ERROR;
		}
	}

	if (ch_init[CHANNEL_2] == TRUE)
	{
		/* Register CAN Channel 2 Msg IRQ handler */
		rc = irq_register(XPAR_FABRIC_SYSTEM_IRQ_F2P1_INTR, can_chan2_irq_handler, NULL);
		if (rc)
		{
			printf("IRQ1 - irq_register() failed with rc = %d\n", (int)rc);
			nStatus = CAN_IRQ_REGISTER_ERROR;
		}
	}

#ifdef _ALL_CHANNELS
	if (ch_init[CHANNEL_3] == TRUE)
	{
		/* Register CAN Channel 3 Msg IRQ handler */
		rc = irq_register(XPAR_FABRIC_SYSTEM_IRQ_F2P2_INTR, can_chan3_irq_handler, NULL);
		if (rc)
		{
			printf("IRQ2 - irq_register() failed with rc = %d\n", (int)rc);
			nStatus = CAN_IRQ_REGISTER_ERROR;
		}
	}

	if (ch_init[CHANNEL_4] == TRUE)
	{
		/* Register CAN Channel 4 Msg IRQ handler */
		rc = irq_register(XPAR_FABRIC_SYSTEM_IRQ_F2P3_INTR, can_chan4_irq_handler, NULL);
		if (rc)
		{
			printf("IRQ3 - irq_register() failed with rc = %d\n", (int)rc);
			nStatus = CAN_IRQ_REGISTER_ERROR;
		}
	}

	if (ch_init[CHANNEL_5] == TRUE)
	{
		/* Register CAN Channel 5 Msg IRQ handler */
		rc = irq_register(XPAR_FABRIC_SYSTEM_IRQ_F2P4_INTR, can_chan5_irq_handler, NULL);
		if (rc)
		{
			printf("IRQ4 - irq_register() failed with rc = %d\n", (int)rc);
			nStatus = CAN_IRQ_REGISTER_ERROR;
		}
	}

	if (ch_init[CHANNEL_6] == TRUE)
	{
		/* Register CAN Channel 6 Msg IRQ handler */
		rc = irq_register(XPAR_FABRIC_SYSTEM_IRQ_F2P5_INTR, can_chan6_irq_handler, NULL);
		if (rc)
		{
			printf("IRQ5 - irq_register() failed with rc = %d\n", (int)rc);
			nStatus = CAN_IRQ_REGISTER_ERROR;
		}
	}

	if (ch_init[CHANNEL_7] == TRUE)
	{
		/* Register CAN Channel 7 Msg IRQ handler */
		rc = irq_register(XPAR_FABRIC_SYSTEM_IRQ_F2P6_INTR, can_chan7_irq_handler, NULL);
		if (rc)
		{
			printf("IRQ6 - irq_register() failed with rc = %d\n", (int)rc);
			nStatus = CAN_IRQ_REGISTER_ERROR;
		}
	}

	if (ch_init[CHANNEL_8] == TRUE)
	{
		/* Register CAN Channel 8 Msg IRQ handler */
		rc = irq_register(XPAR_FABRIC_SYSTEM_IRQ_F2P7_INTR, can_chan8_irq_handler, NULL);
		if (rc)
		{
			printf("IRQ7 - irq_register() failed with rc = %d\n", (int)rc);
			nStatus = CAN_IRQ_REGISTER_ERROR;
		}
	}
#endif /* _ALL_CHANNELS */

	/* Now enable interrupts on CANs that have been initialized */
    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
    {
    	if (ch_init[ch] == TRUE)
    	{
    		if (can_interruptEnable(ch, g_InterruptMask) != CAN_SUCCESS)
    			printf("Interrupt Enable All Request Failed for CHANNEL: %d!\n", ch+1);
    	}
    }

    return nStatus;
}
#endif /* _ENABLE_INTERRUPTS */

#ifdef _ENABLE_INTERRUPTS
/**************************************************************************************************************/
/**
\ingroup CAN_Interrupts
<summary>
can_chan1_msg_irq_handler is responsible for handling the specified channel's msg interrupt.
</summary>
<param name="int_id"> : (Input)Interrupt id.</param>
<param name="context"> : (Input)Pointer to context if available.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void can_chan1_irq_handler(void *context)
{
#ifdef _VERBOSE
	printf("Got Chan1 Interrupt\r\n");
#endif /* _VERBOSE */
	can_intrHandler(CHANNEL_1);
}


/**************************************************************************************************************/
/**
\ingroup CAN_Interrupts
<summary>
can_chan2_msg_irq_handler is responsible for handling the specified channel's msg interrupt.
</summary>
<param name="int_id"> : (Input)Interrupt id.</param>
<param name="context"> : (Input)Pointer to context if available.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void can_chan2_irq_handler(void *context)
{
#ifdef _VERBOSE
	printf("Got Chan2 Interrupt\r\n");
#endif /* _VERBOSE */
	can_intrHandler(CHANNEL_2);
}

#ifdef _ALL_CHANNELS
/**************************************************************************************************************/
/**
\ingroup CAN_Interrupts
<summary>
can_chan3_msg_irq_handler is responsible for handling the specified channel's msg interrupt.
</summary>
<param name="int_id"> : (Input)Interrupt id.</param>
<param name="context"> : (Input)Pointer to context if available.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void can_chan3_irq_handler(void *context)
{
#ifdef _VERBOSE
	printf("Got Chan3 Interrupt\r\n");
#endif /* _VERBOSE */
	can_intrHandler(CHANNEL_3);
}

/**************************************************************************************************************/
/**
\ingroup CAN_Interrupts
<summary>
can_chan4_msg_irq_handler is responsible for handling the specified channel's msg interrupt.
</summary>
<param name="int_id"> : (Input)Interrupt id.</param>
<param name="context"> : (Input)Pointer to context if available.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void can_chan4_irq_handler(void *context)
{
#ifdef _VERBOSE
	printf("Got Chan4 Interrupt\r\n");
#endif /* _VERBOSE */
	can_intrHandler(CHANNEL_4);
}

/**************************************************************************************************************/
/**
\ingroup CAN_Interrupts
<summary>
can_chan5_msg_irq_handler is responsible for handling the specified channel's msg interrupt.
</summary>
<param name="int_id"> : (Input)Interrupt id.</param>
<param name="context"> : (Input)Pointer to context if available.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void can_chan5_irq_handler(void *context)
{
#ifdef _VERBOSE
	printf("Got Chan5 Interrupt\r\n");
#endif /* _VERBOSE */
	can_intrHandler(CHANNEL_5);
}


/**************************************************************************************************************/
/**
\ingroup CAN_Interrupts
<summary>
can_chan6_msg_irq_handler is responsible for handling the specified channel's msg interrupt.
</summary>
<param name="int_id"> : (Input)Interrupt id.</param>
<param name="context"> : (Input)Pointer to context if available.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void can_chan6_irq_handler(void *context)
{
#ifdef _VERBOSE
	printf("Got Chan6 Interrupt\r\n");
#endif /* _VERBOSE */
	can_intrHandler(CHANNEL_6);
}

/**************************************************************************************************************/
/**
\ingroup CAN_Interrupts
<summary>
can_chan7_msg_irq_handler is responsible for handling the specified channel's msg interrupt.
</summary>
<param name="int_id"> : (Input)Interrupt id.</param>
<param name="context"> : (Input)Pointer to context if available.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void can_chan7_irq_handler(void *context)
{
#ifdef _VERBOSE
	printf("Got Chan7 Interrupt\r\n");
#endif /* _VERBOSE */
	can_intrHandler(CHANNEL_7);
}

/**************************************************************************************************************/
/**
\ingroup CAN_Interrupts
<summary>
can_chan8_msg_irq_handler is responsible for handling the specified channel's msg interrupt.
</summary>
<param name="int_id"> : (Input)Interrupt id.</param>
<param name="context"> : (Input)Pointer to context if available.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void can_chan8_irq_handler(void *context)
{
#ifdef _VERBOSE
	printf("Got Chan8 Interrupt\r\n");
#endif /* _VERBOSE */
	can_intrHandler(CHANNEL_8);
}

#endif /* _ALL_CHANNELS */
#endif /* _ENABLE_INTERRUPTS */

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
load_params is responsible for loading default parameters from flash for the desired channel index (zero based).
</summary>
<param name="ch"> : (Input) Channel index (zero based) for which to load parameters.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
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

#ifdef _ENABLE_INTERRUPTS
/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_intrHandler is responsible for reading the interrupt status from the ISR, determines the source of
the interrupts, calls according callbacks, and finally clears the interrupts.
</summary>
<param name="p"> : (Input) CAN port.</param>
<returns>uint8_t : Status
	- 0 : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
<seealso cref="can_interruptGetStatus">
<seealso cref="can_interruptGetEnabled">
<seealso cref="can_getBusErrorStatus">
<seealso cref="can_clearBusErrorStatus">
<seealso cref="can_interruptClear">
*/
/**************************************************************************************************************/
uint8_t can_intrHandler(CHANNEL ch)
{
#if (MODULE_CB2 || MODULE_CB3)
	j1939_t J1939_RX_pkt;
	uint8_t DataBuf[8] = {0,0,0,0,0,0,0,0};
#endif
	can_t   AB_RX_pkt;

	uint32_t PendingIntr = 0;
	uint32_t EnabledIntr = 0;

	/* Make sure port is within range */
	uint8_t status = can_checkPort(ch);
	if (status != CAN_SUCCESS)
		return status;

	volatile unsigned int *pDPRAM_LAST_ERR_CODE = (volatile unsigned int *)(DPRAM_CH1_LAST_ERR_CODE + (ch * CHANNEL_DEDICATED_SIZE));

	/* Get pending interrupts */
	status = can_interruptGetStatus(ch, &PendingIntr);
	if (status != CAN_SUCCESS)
		return status;

	/* Get interrupts that have been enabled */
	status = can_interruptGetEnabled(ch, &EnabledIntr);
	if (status != CAN_SUCCESS)
		return status;

	/* Only interested in status of interrupts that are currently enabled...safety */
	PendingIntr &= EnabledIntr;

	if (PendingIntr & CAN_IXR_ERROR_MASK)
	{
		uint32_t unRegStatus = 0;
		uint32_t unErrorCounter = 0;

		volatile unsigned int *pDPRAM_CERC = (volatile unsigned int *)(DPRAM_CH1_CERC + (ch * CHANNEL_DEDICATED_SIZE));
		volatile unsigned int *pDPRAM_BUS_STATUS = (volatile unsigned int *)(DPRAM_CH1_BUS_STATUS + (ch * CHANNEL_DEDICATED_SIZE));

		PendingIntrCount[ch]++;

		/*****************************************************************/
		/* Bus Status 			 										 */
		/*****************************************************************/
		/* Fetch the Bus Error Status and expose it to the outside world */
		can_getBusErrorStatus( ch, &unRegStatus );
		*pDPRAM_BUS_STATUS = unRegStatus;

//#if defined(_VERBOSE) || defined (_INTERRUPT_VERBOSE)
		/* Here we only print out a message to console if detected error has changed or if repeated occurrence of same
		 * interrupt exceeds 1000 */
		if (PendingIntr != LastPendingIntr[ch] || PendingIntrCount[ch] > 1000)
		{
			if (PendingIntrCount[ch] <= 1000)
			{
				printf("Detected Reportable Error Interrupt! 1st occurrence on Channel = %d - ISR = 0x%x\n", (ch+1), (unsigned int)PendingIntr);
				printf("Bus Error Status (ESR) = 0x%x\n\n",(unsigned int)unRegStatus);
				ErrorTiming[ch] = get_timer(0);
			}
			else
			{
				uint8_t ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1 = 0;

				printf("Same Reportable Error Interrupt detected > 1000 times. Current Channel = %d - ISR = 0x%x\n", (ch+1), (unsigned int)PendingIntr);
				printf("Bus Error Status (ESR) = 0x%x\n",(unsigned int)unRegStatus);

				/* If we get 1000 errors in less than 3 seconds of the same kind of error on the same channel...let's reset the channel! */
				if (get_timer(ErrorTiming[ch]) < 3000)
				{
					printf("Resetting Channel %d due to detection of > 1000 ISRs\n", (ch+1));

					/* Get current baud rate..so we can re-initialize CAN with same when done */
					can_getBaudRatePrescaler(ch, &ucPrescaler);
					can_getBitTiming(ch, &ucSJW, &ucTimeSeg2, &ucTimeSeg1);

					/* Reset current channel and clear all pending interrupts for channel and return */
					reset_channel(ch, ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1, FALSE);
					can_interruptClear(ch, PendingIntr);
					ErrorTiming[ch] = 0;
					return status;
				}
				else /*1000 errors were not received within 3 seconds so start the clock over for this channel */
					ErrorTiming[ch] = get_timer(0);
			}
			LastPendingIntr[ch] = PendingIntr;
			PendingIntrCount[ch] = 0;
		}
//#endif /* _VERBOSE || _INTERRUPT_VERBOSE */

		/* Let outside world know to look at Bus Status register for more info */
		if (unRegStatus != 0)
			*pDPRAM_LAST_ERR_CODE |= CAN_BUS_STATUS_TYPE;
		else
			*pDPRAM_LAST_ERR_CODE &= ~CAN_BUS_STATUS_TYPE;

		/* Clear the interrupt so we do not get flooded */
		can_clearBusErrorStatus( ch, unRegStatus );

		/* Expose error count for RX and TX to the outside world */
		status = can_getBusErrorCounterCombined(ch, &unErrorCounter);
		if (status == CAN_SUCCESS)
			*pDPRAM_CERC = unErrorCounter;
	}

#if defined(_VERBOSE) || defined(_INTERRUPT_VERBOSE)
	/* A frame was transmitted successfully */
	if ((PendingIntr & CAN_IXR_TXOK_MASK))
		printf("Detected Transmit Interrupt for CHANNEL: %d\n", ch+1);
#endif /* _VERBOSE || _INTERRUPT_VERBOSE */

	 /* A frame was received and is sitting in RX FIFO. CAN_IXR_RXOK_MASK is not used because
	    the bit is set just once even if there are multiple frames sitting in RX FIFO.
	    CAN_IXR_RXNEMP_MASK is used because the bit can be set again and again automatically
	    as long as there is at least one frame in RX FIFO. */
	if ((PendingIntr & CAN_IXR_RXNEMP_MASK))
	{
#if defined(_VERBOSE) || defined(_INTERRUPT_VERBOSE)
		printf("Detected Receive Interrupt for CHANNEL: %d\n", ch+1);
#endif /* _VERBOSE || _INTERRUPT_VERBOSE */

#if (MODULE_CB2 || MODULE_CB3)
		if (port_protocol_type[ch] == protocol_can_j1939)
		{
			J1939_RX_pkt.buf = &DataBuf[0];
			status = j1939_can_rx(ch, &J1939_RX_pkt);
		}
		else if (port_protocol_type[ch] == protocol_can_ab)
			status = can_rx(ch, &AB_RX_pkt);
#elif (MODULE_CB1)
		status = can_rx(ch, &AB_RX_pkt);
#endif
		if (status == CAN_SUCCESS)
		{
#if defined(_VERBOSE) || defined(_INTERRUPT_VERBOSE)
#if (MODULE_CB2 || MODULE_CB3)
			if (port_protocol_type[ch] == protocol_can_j1939)
				print_J1939_packet(&J1939_RX_pkt);
			else if (port_protocol_type[ch] == protocol_can_ab)
				print_AB_packet(&AB_RX_pkt);
#elif (MODULE_CB1)
			print_AB_packet(&AB_RX_pkt);
#endif

#endif 		/* _VERBOSE || _INTERRUPT_VERBOSE */

#if (MODULE_CB2 || MODULE_CB3)
			if (port_protocol_type[ch] == protocol_can_j1939)
				fill_J1939_RX_UIF(&J1939_RX_pkt, ch);
			else if (port_protocol_type[ch] == protocol_can_ab)
				fill_AB_RX_UIF(&AB_RX_pkt, ch);
#elif (MODULE_CB1)
			fill_AB_RX_UIF(&AB_RX_pkt, ch);
#endif
		}
		else if (status != CAN_MSG_UNINTENDED_RECIPIENT)
		{
			if (port_protocol_type[ch] == protocol_can_j1939)
				printf("Error receiving J1939 packet! (j1939_can_rx) - Status = %d\n", status);
			else if (port_protocol_type[ch] == protocol_can_ab)
				printf("Error receiving CAN AB packet! (can_rx) - Status = %d\n", status);
		}
#if defined(_VERBOSE) || defined(_INTERRUPT_VERBOSE)
		else
			printf("Message being ignored for CHANNEL: %d due to channel being UNINTENDED RECIPIENT\n", ch+1);
#endif
	}

	/* Clear all pending interrupts */
	can_interruptClear(ch, PendingIntr);

	return status;
}
#endif /* _ENABLE_INTERRUPTS */

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
check_diag_mode is responsible for checking to see if we should go into factory diagnostic mode.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void check_diag_mode(void)
{
    CHANNEL ch;
    volatile unsigned short * pDPRAM_FACTORY_DIAG = (volatile unsigned short *)DPRAM_FACTORY_DIAG;
    unsigned short bit_flg;
//	unsigned int cctrl;
	static unsigned short factory_debug_last = 0;

	if (factory_debug_last != *pDPRAM_FACTORY_DIAG)
	{   // --- There Was a Change ---
		factory_debug_last = *pDPRAM_FACTORY_DIAG;                      // update the static variable

	    for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
	    {
    	    bit_flg = 0x0001 << ch;
    	    if (*pDPRAM_FACTORY_DIAG & bit_flg)
    	    {
/*ORIG_CODE - WHAT IS EQUIVALENT TO THIS IN AXI????
        	    cctrl = CAN_R32(ADR_D_CAN_CCTRL, ch);           // read the current state
        	    cctrl |= 0x0080;                                // 'or' in the test enable bit
                CAN_W32(ADR_D_CAN_CCTRL, ch, cctrl);	        // put into test mode
    	        CAN_W16(ADR_D_CAN_CTR,   dh, 0x0040);           // drive a dominant ('0') on the bus
*/
    	    }
    	    else
    	    {
/*ORIG_CODE - WHAT IS EQUIVALENT TO THIS IN AXI????
        	    cctrl = CAN_R32(ADR_D_CAN_CCTRL, ch);           // read the current state
        	    cctrl &= ~0x0080;                               // 'and' out the test enable bit
                CAN_W32(ADR_D_CAN_CCTRL, ch, cctrl);	        // put into test mode
*/
    	    }
    	}
	}
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
check_for_port_configuration_change is responsible for updating what the given port is being configured for.
Currently either J1939 or CAN AB.
</summary>
<param name="ch"> : (Input) Channel index (zero based) to be reset.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void check_for_port_configuration_change(CHANNEL ch)
{
	volatile unsigned int *pDPRAM_CONFIG  = (volatile unsigned int *)(DPRAM_CH1_CONTROL   + (ch * CHANNEL_DEDICATED_SIZE));
	uint8_t can_protocol = (uint8_t)(*pDPRAM_CONFIG & CTRL_CONFIG_PROTOCOL_MASK);
	uint8_t finished = 0;

	/* If, due to licensing, this CAN module is not CAN AB capable..we need to ignore CAN AB requests */
#if (MODULE_CB1 || MODULE_CB3)
	if (can_protocol & CTRL_CONFIG_ENABLE_AB)
	{
		port_protocol_type[ch] = protocol_can_ab;
		finished = 1;
	}
#endif

#if (MODULE_CB2 || MODULE_CB3)
	/* We do not want customers paying for CB1 to have J1939 capability */
	if (!finished)
	{
		if (can_protocol & CTRL_CONFIG_ENABLE_J1939)
		{
			if (!j1939_is_port_enabled(ch))
				j1939_enable_p(ch, 1);
			port_protocol_type[ch] = protocol_can_j1939;
			finished = 1;
		}
		else /* Port is configured for a different CAN protocol ... so make sure we disable J1939 capability if it was enabled */
		{
			if (j1939_is_port_enabled(ch))
				j1939_enable_p(ch, 0);
		}
	}
#endif

	if (finished)
	{
		/* Let's Reset Channel and set Baud Rate to default baud rate for the provided protocol */
		if (port_protocol_type[ch] == protocol_can_ab)
			reset_channel(ch, CAN_BRP_500K, CAN_SJW_500K, CAN_TSEG2_500K, CAN_TSEG1_500K, TRUE);
#if (MODULE_CB2 || MODULE_CB3)
		else if (port_protocol_type[ch] == protocol_can_j1939)
			reset_channel(ch, CAN_J1939_BRP_500K, CAN_J1939_SJW_500K, CAN_J1939_TSEG2_500K, CAN_J1939_TSEG1_500K, TRUE);
#endif
	}
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
check_for_baud_change is responsible for updating the baud rate for the specified channel.
</summary>
<param name="ch"> : (Input) Channel index (zero based) to be reset.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void check_for_baud_change(CHANNEL ch)
{
   unsigned short usBaudRate = 0;
   unsigned short usBaudPrescalerExt = 0;
   unsigned int unBaudTiming = 0;

   volatile unsigned int * pDPRAM_BAUD_TIMING;
   pDPRAM_BAUD_TIMING = (volatile unsigned int *)(DPRAM_CH1_BAUD_TIMING + (ch * CHANNEL_DEDICATED_SIZE));

   /* Prescaler Ext is in lower 16 bits...Baud rate can be found in the upper 16 bits */
   usBaudPrescalerExt= (unsigned short)((*pDPRAM_BAUD_TIMING) & 0x0000FFFF);
   usBaudRate = (unsigned short)(((*pDPRAM_BAUD_TIMING) >> 16) & 0x0000FFFF);

   if (usBaudRate & 0x8000)
   {
#if defined(_VERBOSE)
	   printf("*****************************\r\n");
	   printf("Baud Rate Change Request!\r\n");
	   printf("*****************************\r\n");
#endif
	   // --- User wants to change the Timing Settings ---
	   usBaudRate &= 0x7FFF;						// mask out the control bit

	   *pDPRAM_BAUD_TIMING &= 0x7FFFFFFF;			// mask out the control bit in actual register as well

	   if (can_enterMode(ch, CAN_MODE_CONFIG) == CAN_SUCCESS)
	   {
#if defined(_VERBOSE)
		   printf("Requested Baud Prescaler = %d\r\n", (usBaudRate & 0x3F));
		   printf("Requested Baud SJW = %d\r\n", ((usBaudRate >> 6) & 0x03));
		   printf("Requested Baud TSEG1 = %d\r\n", ((usBaudRate >> 8) & 0x0F));
		   printf("Requested Baud TSEG2 = %d\r\n", ((usBaudRate >> 12) & 0x07));
#endif
		   can_setBaudRatePrescaler(ch, (usBaudRate & 0x3F));
		   can_setBitTiming(ch, ((usBaudRate >> 6) & 0x03)/*SJW*/, ((usBaudRate >> 12) & 0x07)/*TSeg2*/, ((usBaudRate >> 8) & 0x0F)/*TSeg1*/);
		   can_enterMode(ch, CAN_MODE_NORMAL);
	   }
	   else
	   {
		   printf("Error trying to enter config mode for configuring Baud Rate\r\n");
		   volatile unsigned int * pDPRAM_COMM_STATUS = (volatile unsigned int *)(DPRAM_CH1_COMM_STATUS  + (ch * CHANNEL_DEDICATED_SIZE));
		   volatile unsigned int * pDPRAM_LAST_ERR_CODE = (volatile unsigned int *)(DPRAM_CH1_LAST_ERR_CODE  + (ch * CHANNEL_DEDICATED_SIZE));

		   /* Report that we were unable to get into config mode */
		   *pDPRAM_COMM_STATUS = CAN_NAI_NOT_IN_CONFIG_MODE;

			/* Set the COMM STATUS bit of the LEC so caller knows to look at COMM STATUS register for more error information */
			*pDPRAM_LAST_ERR_CODE |= CAN_COMM_STATUS_TYPE;
	   }
   }

   /* Now make sure current configured Baud Rate is reflected in the Baud Timing Register.
      This is only done if the BaudRate register has not been initialized */
   if (usBaudRate == 0)
   {
	   uint8_t ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1 = 0;
	   can_getBaudRatePrescaler(ch, &ucPrescaler);
	   can_getBitTiming(ch, &ucSJW, &ucTimeSeg2, &ucTimeSeg1);

	   usBaudRate   = (((ucTimeSeg2 & 0x07) << 12) | ((ucTimeSeg1 & 0x0F) << 8) | ((ucSJW & 0x03) << 6) | (ucPrescaler & 0x3F));
	   usBaudPrescalerExt = 0;
	   unBaudTiming = (unsigned int)(usBaudRate << 16 | usBaudPrescalerExt);
	   *pDPRAM_BAUD_TIMING = unBaudTiming;
   }
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
can_set_baud_to_250 is responsible for updating the baud rate for the specified channel to 250 Kb/sec.
</summary>
<param name="ch"> : (Input) Channel index (zero based).</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
bool can_set_baud_to_250(CHANNEL ch)
{
	bool status = FALSE;
	unsigned short usBaudRate = 0x1C;   /* Based on 8Mhz clock and 87.5% sample point */
	unsigned short usBaudPrescaler = 1;

	if (can_enterMode(ch, CAN_MODE_CONFIG) == CAN_SUCCESS)
	{
	   can_setBaudRatePrescaler(ch, usBaudPrescaler);
	   can_setBitTiming(ch, ((usBaudRate >> CAN_BTR_SJW_SHIFT) & 0x03)/*SJW*/, ((usBaudRate >> CAN_BTR_TS2_SHIFT) & 0x07)/*TSeg2*/, (usBaudRate & 0x0F)/*TSeg1*/);
	   can_enterMode(ch, CAN_MODE_NORMAL);
	   status = TRUE;
	}
	return status;
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
can_set_baud_to_250 is responsible for updating the baud rate for the specified channel to 500 Kb/sec.
</summary>
<param name="ch"> : (Input) Channel index (zero based).</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
bool can_set_baud_to_500(CHANNEL ch)
{
	bool status = FALSE;
	unsigned short usBaudRate = 0x1C;   /* Based on 8Mhz clock and 87.5% sample point */
	unsigned short usBaudPrescaler = 0;

	if (can_enterMode(ch, CAN_MODE_CONFIG) == CAN_SUCCESS)
	{
	   can_setBaudRatePrescaler(ch, usBaudPrescaler);
	   can_setBitTiming(ch, ((usBaudRate >> CAN_BTR_SJW_SHIFT) & 0x03)/*SJW*/, ((usBaudRate >> CAN_BTR_TS2_SHIFT) & 0x07)/*TSeg2*/, (usBaudRate & 0x0F)/*TSeg1*/);
	   can_enterMode(ch, CAN_MODE_NORMAL);
	   status = TRUE;
	}
	return status;
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
check_for_config_changes is responsible for checking to see if there has been changes to the CAN configuration
for the specified channel. Buad Rate, Acceptance Filters, Reset...
</summary>
<param name="ch"> : (Input) Channel index (zero based)</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void check_for_config_changes(CHANNEL ch)
{
	volatile unsigned int *pDPRAM_CONFIG  = (volatile unsigned int *)(DPRAM_CH1_CONTROL   + (ch * CHANNEL_DEDICATED_SIZE));
#if (MODULE_CB2 || MODULE_CB3)
	unsigned int i = 0;
#endif

	/* Check to see if caller has made request to change the address for the specific channel/port */
	check_for_port_configuration_change(ch);
#if (MODULE_CB2 || MODULE_CB3)
	check_for_address_change(ch);
#endif
	check_for_self_test(ch);
	check_for_baud_change(ch);
	check_for_acceptance_filter_change(ch);
	check_for_reset(ch);

#if (MODULE_CB2 || MODULE_CB3)
	/* If this channel is being configured to enable TX or RX and it is enabled for J1939,
	   we need to make sure an address has been claimed */
	if ((*pDPRAM_CONFIG & CTRL_CONFIG_TX) || (*pDPRAM_CONFIG & CTRL_CONFIG_RX))
	{
		/* If an address has not yet been claimed for the specified port...let's attempt to assign one now */
		if (port_protocol_type[ch] == protocol_can_j1939 &&  j1939_assigned_sa_claim_passed(ch) == 0)
		{
			j1939_init_p(ch);

			/* May not be able to acquire address on first try so we provide ample tries for the acquire. */
			for (i=0; i < 500; i++)
			{
				j1939_update_address_claim(ch);
				if (j1939_assigned_sa_claim_passed(ch) > 0)
					break;
			}
		}
	}
#endif

	/* If Both TX and RX are not to be enabled...we make sure interrupts are disabled */
	if (0 == (*pDPRAM_CONFIG & (CTRL_CONFIG_TX | CTRL_CONFIG_RX)))
		can_interruptDisable(ch, CAN_IXR_ALL);
	/* If RX is to be disabled...make sure we disable the RX interrupt. NOTE: we do not interrupt on TX regardless*/
	else if (0 == (*pDPRAM_CONFIG & CTRL_CONFIG_RX))
		can_interruptDisable(ch, CAN_IXR_RXNEMP_MASK);
	/* Enable all desired interrupts */
	else
		can_interruptEnable(ch, g_InterruptMask);

	/* Let's make sure buffered control variable is up-to-date */
	RX_UIF[ch].ctrl = *pDPRAM_CONFIG;
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
check_for_reset is responsible for checking to see if a reset request for the given channel has been made.
</summary>
<param name="ch"> : (Input) Channel index (zero based)</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void check_for_reset(CHANNEL ch)
{
	volatile unsigned int *pDPRAM_CONFIG  = (volatile unsigned int *)(DPRAM_CH1_CONTROL   + (ch * CHANNEL_DEDICATED_SIZE));

	if (*pDPRAM_CONFIG & CTRL_RESET)
	{
		uint8_t ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1 = 0;

		/* Get current baud rate..so we can re-initialize CAN with same when done */
		can_getBaudRatePrescaler(ch, &ucPrescaler);
		can_getBitTiming(ch, &ucSJW, &ucTimeSeg2, &ucTimeSeg1);

		reset_channel(ch, ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1, TRUE);
		*pDPRAM_CONFIG &= 0x7FFF;
		RX_UIF[ch].ctrl = *pDPRAM_CONFIG;
	}
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
check_for_acceptance_filter_change is responsible for checking to see if there has been changes to the
acceptance filters. (NOTE: AC_CODE maps to what is now Acceptance Filter ID of Xilinx Axi CAN.
</summary>
<param name="ch"> : (Input) Channel index (zero based)</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void check_for_acceptance_filter_change(CHANNEL ch)
{
	volatile unsigned int *pDPRAM_FILTER_CONFIG = (volatile unsigned int *)(DPRAM_CH1_FILTER_CONTROL  + (ch * CHANNEL_DEDICATED_SIZE));
	volatile unsigned int *pDPRAM_AC_MASK;
	volatile unsigned int *pDPRAM_AC_ID;

	uint8_t status = CAN_SUCCESS;
	uint32_t acceptanceFilterEnabled = 0;
	uint32_t numFiltersToEnable = 0;
	uint32_t acceptanceFilterMask = 0;
	uint32_t acceptanceFilterID   = 0;
	uint8_t ucAcceptFilterBusy = 0;
	uint16_t i = 0;
	int32_t nBusyCount = 0;
	const int32_t MAX_BUSY_RETRIES = 1000;

	uint32_t filterControlRegValue = *pDPRAM_FILTER_CONFIG;

	/* If the "Edit Filter" bit is set (1), do not attempt to interpret any of the Filter Sets for this channel until the "Edit Filter" bit is set back to a zero */
	if ((filterControlRegValue & FILTCTRL_EDIT_FILTER_MASK) > 0)
		return;

	/* If something changed regarding the number of filters or which filters are enabled...we need to pass those changes along to
	 * the core. If no filter enable changes were made..we need to make sure there were also no changes in the enabled filters mask
	 * and code */
	if ((RX_UIF[ch].filterCtrl != filterControlRegValue) || changes_made_to_filter_mask_or_code(ch, filterControlRegValue))
	{
#ifdef _FILTER_VERBOSE
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		printf("Detected Filter Changes are Required for channel %d - FilterControlRegValue = 0x%x\n", (ch+1), (unsigned int)filterControlRegValue);
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif

		/* Store the filter control register value so we can determine if changes were made next time this check is called */
		RX_UIF[ch].filterCtrl = filterControlRegValue;

		/* Determine number of filters that should be enabled for this channel - This governs which of the Acceptance Mask/Code pairs
		   will be active. NOTE: A value of 1 indicates only the first pair (AF1) will be active, 2 indicates AF1 and AF2 pairs will
		   be active, 3 indicates AF1, AF2 and AF3 pairs will be active and 4 indicates AF1, AF2, AF3 and AF4 pairs will be active */
		numFiltersToEnable = (filterControlRegValue & FILTCTRL_ENABLE_FILTER_MASK);

	#ifdef _FILTER_VERBOSE
		printf("Number of Filters to Enable = %d\n", (int)numFiltersToEnable);
	#endif

		/* No filters enabled...make sure we disable all filters for channel */
		if (numFiltersToEnable == 0)
		{
			can_acceptFilterDisable(ch, CAN_AFR_UAF_ALL_MASK);
			return;
		}

		/* Make sure we are within bounds of capability...else force within bounds */
		if (numFiltersToEnable > MAX_CHANNEL_FILTER_COUNT)
			numFiltersToEnable = MAX_CHANNEL_FILTER_COUNT;

		/* Get current state of affairs of what acceptance filters are currently enabled */
		can_acceptFilterGetEnabled( ch, &acceptanceFilterEnabled );

		for (i=1 ; i <= numFiltersToEnable; i++)
		{
			pDPRAM_AC_MASK = (volatile unsigned int *)(DPRAM_CH1_ACPT_MASK + (ch * CHANNEL_DEDICATED_SIZE) + ((i-1) * CHANNEL_FILTER_PAIR_ADDRESS_SKIP));
			pDPRAM_AC_ID   = (volatile unsigned int *)(DPRAM_CH1_ACPT_CODE + (ch * CHANNEL_DEDICATED_SIZE) + ((i-1) * CHANNEL_FILTER_PAIR_ADDRESS_SKIP));

			acceptanceFilterMask = getXilinxAcceptanceFilterMaskFromFilterControl(ch, i, filterControlRegValue);
			acceptanceFilterID   = getXilinxAcceptanceFilterIDFromFilterControl(ch, i, filterControlRegValue);

			/* If current filter is enabled...we need to disable it before we can make changes */
			if ((acceptanceFilterEnabled & CAN_AFR_MASK(i)) > 0)
			{
#ifdef _FILTER_VERBOSE
				printf("Disabling Filter for Chan: %d Filter: %u\n", (ch+1), i);
#endif
				can_acceptFilterDisable(ch, CAN_AFR_MASK(i));
			}

			/* Store the passed in ID Mask and Code so we can reference later if needed */
			/* NOTE: We store these values even if the Mask and ID enable bit is not currently set. This way if
			         user just enables Mask and ID filtering later...we will use the desired values. */
			RX_UIF[ch].ac_msk[i-1]   = *pDPRAM_AC_MASK;
			RX_UIF[ch].ac_code[i-1]  = *pDPRAM_AC_ID;

#ifdef _FILTER_VERBOSE
			if ((filterControlRegValue & FILTCTRL_ENABLE_ACC_CODE_AND_MASK(i)) > 0)
			{
				printf("Acceptance ID Mask: 0x%x for Filter: %d\n", (unsigned int)RX_UIF[ch].ac_msk[i-1], i);
				printf("Acceptance ID: 0x%x for Filter: %d\n", (unsigned int)RX_UIF[ch].ac_code[i-1], i);
			}
			else
				printf("Not filtering by ID Mask!\n");
			printf("Xilinx Acceptance Mask: 0x%x for Filter: %d\n", (unsigned int)acceptanceFilterMask, i);
			printf("Xilinx Acceptance ID: 0x%x for Filter: %d\n", (unsigned int)acceptanceFilterID, i);
#endif

			/* Make sure acceptance filter is not busy prior to attempting to change mask or ID. */
			do
			{
				if (can_isAcceptFilterBusy(ch, &ucAcceptFilterBusy) != CAN_SUCCESS)
					return;

				if (ucAcceptFilterBusy)
				{
					nBusyCount++;
					wait_microsecond(500);
					if (nBusyCount > MAX_BUSY_RETRIES)
						break;
				}
			} while (ucAcceptFilterBusy);

			if (nBusyCount <= MAX_BUSY_RETRIES)
			{
				status = can_acceptFilterSet(ch, CAN_AFR_MASK(i), acceptanceFilterMask, acceptanceFilterID);
				/* Now change the filter to the desired value */
				if (status == CAN_SUCCESS)
				{
#ifdef _FILTER_VERBOSE
				    printf("About to enable Filter: %d\n", i);
#endif
				    /* Now enable the filter! */
				    status = can_acceptFilterEnable(ch, CAN_AFR_MASK(i));
				    if (status != CAN_SUCCESS)
				    	printf("can_acceptFilterEnable call FAILED! - status = %d\n", status);
				}
				else
					printf("can_acceptFilterSet call FAILED! - status = %d\n", status);
			}
		}

		/* Make sure rest of filter pairs are disabled */
		for ( ; i <= MAX_CHANNEL_FILTER_COUNT; i++)
		{
			can_acceptFilterDisable(ch, CAN_AFR_MASK(i));

#ifdef _FILTER_VERBOSE
			printf("Ensuring disable of Filter: %d\n", i);
#endif
		}
	}
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
check_for_tx is responsible for checking the TX Fifo to see if a request has been made to transmit a CAN message.
If so, and a full CAN frame exists, the data is transmitted.
</summary>
<param name="ch"> : (Input) Channel index (zero based) of channel to check for possible transmission.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void check_for_tx(CHANNEL ch)
{
	int frame_count;
	int ret_val = 0;
	uint8_t status = CAN_SUCCESS;
	can_t ABFrame;

#if (MODULE_CB2 || MODULE_CB3)
	j1939_t J1939Frame;
	uint8_t j1939TP_status = 0;
#endif

	volatile unsigned int *pDPRAM_CONFIG 		= (volatile unsigned int *)(DPRAM_CH1_CONTROL      	+ (ch * CHANNEL_DEDICATED_SIZE));
	volatile unsigned int *pDPRAM_TX_FRAMECNT 	= (volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE));

	frame_count = *pDPRAM_TX_FRAMECNT;

	if ( (*pDPRAM_CONFIG & CTRL_CONFIG_TX) &&   // Is TX enabled?
		 (frame_count != 0) &&             // Is there Data to go out?
		 (!can_isTxFifoFull(ch)) )
	{
		volatile unsigned int * pDPRAM_LAST_ERR_CODE = (volatile unsigned int *)(DPRAM_CH1_LAST_ERR_CODE  + (ch * CHANNEL_DEDICATED_SIZE));

#if (MODULE_CB2 || MODULE_CB3)
		if (port_protocol_type[ch] == protocol_can_j1939)
			ret_val = fill_J1939_TX_UIF(&J1939Frame, ch);
		else if (port_protocol_type[ch] == protocol_can_ab)
			ret_val = fill_AB_TX_UIF(&ABFrame, ch);
#elif (MODULE_CB1)
		ret_val = fill_AB_TX_UIF(&ABFrame, ch);
#endif

		if (ret_val ==  CAN_SUCCESS)
		{
#if (MODULE_CB2 || MODULE_CB3)
			if (port_protocol_type[ch] == protocol_can_j1939)
				status = j1939_tx_app(&J1939Frame, &j1939TP_status);
			else if (port_protocol_type[ch] == protocol_can_ab)
				status =can_tx(ch, &ABFrame);
#elif (MODULE_CB1)
			status =can_tx(ch, &ABFrame);
#endif

			if (status != CAN_SUCCESS)
			{
#ifdef _VERBOSE
				printf("can tx returned error status %d\n", status);
#endif
				volatile unsigned int * pDPRAM_COMM_STATUS = (volatile unsigned int *)(DPRAM_CH1_COMM_STATUS  + (ch * CHANNEL_DEDICATED_SIZE));
				*pDPRAM_COMM_STATUS = CAN_NAI_TX_FAILURE;
				*pDPRAM_LAST_ERR_CODE |= CAN_COMM_STATUS_TYPE;
			}

			/*Need to wait between TXs else CORE becomes overwhelmed */
			wait_microsecond(350);
		}
		else
		{
			printf("Frame Count Before Clear = %d\n", *((volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE))));

			// --- Clear out the TX FIFO ---
			volatile unsigned int *pFPGA_HPS_CONTROL = (volatile unsigned int *)(FPGA_HPS_CONTROL);
			*pFPGA_HPS_CONTROL = (0x0100 << ch);  /* Clear the TX FIFO for desired channel */

			// --- Error Happened when reading the TX FIFO ---
			// --- Report the Error ---
			volatile unsigned int * pDPRAM_COMM_STATUS = (volatile unsigned int *)(DPRAM_CH1_COMM_STATUS  + (ch * CHANNEL_DEDICATED_SIZE));

			if (ret_val == 1)
				*pDPRAM_COMM_STATUS = CAN_NAI_FIFO_EMPTY;   // FIFO is empty
			else if (ret_val == 2)
				*pDPRAM_COMM_STATUS = CAN_NAI_FIFO_NOT_ENOUGH_DATA;   // FIFO doesn't have enough data for a full message
			else if (ret_val == 3)
				*pDPRAM_COMM_STATUS = CAN_NAI_FIFO_NO_START_FLAG;   // FIFO doesn't contain a start flag
			else if (ret_val == 4)
				*pDPRAM_COMM_STATUS = CAN_NAI_TX_DATA_COUNT_NULL;   // tx_m->data_count = NULL when there is data to transmit.
			else if (ret_val == 5)
				*pDPRAM_COMM_STATUS = CAN_NAI_FIFO_END_FLAG_NOT_FOUND;   // END_FLAG wasn't where it was expected
			else if (ret_val == 6)
				*pDPRAM_COMM_STATUS = CAN_NAI_BAD_SIZE;   // Bad Size.  Size over 255 payload limit.

			/* Set the COMM STATUS bit of the LEC so caller knows to look at COMM STATUS register for more error information */
			*pDPRAM_LAST_ERR_CODE |= CAN_COMM_STATUS_TYPE;

			printf("ERROR %d DETECTED IN check_for_tx - FIFO was emptied!\n", ret_val);
			printf("Frame Count After Clear = %d\n", *((volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE))));
		}
	}
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
reset_channel is responsible for resetting everything to do with a CAN channel. (Flush the user FIFOs, Clear
errors and start Bus_Off recovery)
</summary>
<param name="ch"> : (Input) Channel index (zero based) to be reset.</param>
<param name="ucPrescaler"> : (Input) Baud Prescaler value.</param>
<param name="ucSJW"> : (Input) Baud SJW value.</param>
<param name="ucTimeSeg2"> : (Input) Baud Time Segment 2 value.</param>
<param name="ucTimeSeg1"> : (Input) Baud Time Segment 1 value.</param>
<param name="bClearStatus"> : (Input) Determines if status is to be cleared (true) or not (false).</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void reset_channel(CHANNEL ch, uint8_t ucPrescaler, uint8_t ucSJW, uint8_t ucTimeSeg2, uint8_t ucTimeSeg1, bool bClearStatus)
{
	volatile unsigned int *pFPGA_HPS_CONTROL = (volatile unsigned int *)(FPGA_HPS_CONTROL);

	/* Request specific CAN to be reset */
	can_reset(ch);

	/* Clear FIFOs */
	*pFPGA_HPS_CONTROL = (0x0101 << ch);  /* Clear FIFOs for desired channel (RX and TX) */

	// ----------------------------
	// ---   Clear RAM Values   ---
	// ----------------------------
	memset(&RX_UIF[ch], 0, sizeof(struct RX_CONTROL));

	/* Re-initialize CAN port (using last configured baud rate info) */
	can_init_with_baud(ch, ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1);

	/* If caller wants to also clear status...clear all status registers */
	if (bClearStatus)
		clear_all_reported_status(ch);

#ifdef _ENABLE_INTERRUPTS
	/*Re-enable interrupts now that we reset the channel */
	init_module_interrupts();
#endif /* _ENABLE_INTERRUPTS */
}


#if (MODULE_CB2 || MODULE_CB3)
/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
check_for_address_change is responsible for updating the specified channel/port with the desired address.
</summary>
<param name="ch"> : (Input) Channel index (zero based) to be reset.</param>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
static void check_for_address_change(CHANNEL ch)
{
	volatile unsigned int *pDPRAM_CONFIG  = (volatile unsigned int *)(DPRAM_CH1_CONTROL   + (ch * CHANNEL_DEDICATED_SIZE));
	volatile unsigned int *pDPRAM_SOURCE_ADDRESS  = (volatile unsigned int *)(DPRAM_CH1_SOURCE_ADDRESS   + (ch * CHANNEL_DEDICATED_SIZE));

	/* Check to see if user requested to update the address of the specified channel. */
	/* If so, make the request to the J1939 subsystem. */
	if (*pDPRAM_CONFIG & CTRL_CONFIG_ASSIGN_ADDRESS)
	{
		Src_Address[ch] = (uint8_t)(*pDPRAM_SOURCE_ADDRESS & 0x00FF);
		SA_Attempts[ch] = 0; /*Zero out the attempts global variable for given channel*/

#ifdef _VERBOSE
		printf("Detected request to assign Source Address to 0x%x\n", (unsigned int)Src_Address[ch]);
#endif
		j1939_init_p(ch);

		/* Reset flag since we attempted to assign desired source address */
		*pDPRAM_CONFIG &= ~CTRL_CONFIG_ASSIGN_ADDRESS;
	}
	else if (*pDPRAM_CONFIG & CTRL_CONFIG_RETRIEVE_ADDRESS)
	{
		*pDPRAM_SOURCE_ADDRESS = (unsigned int)j1939_get_assigned_sa(ch);

		/* Now retrieve current status */
		if (j1939_assigned_sa_claim_passed(ch))
			*pDPRAM_SOURCE_ADDRESS |= CAN_ADDR_CLAIM_OK;
		else if (j1939_assigned_sa_claim_in_progress(ch))
			*pDPRAM_SOURCE_ADDRESS |= CAN_ADDR_CLAIM;
		else if (j1939_assigned_sa_claim_failed(ch))
			*pDPRAM_SOURCE_ADDRESS |= CAN_ADDR_CLAIM_FAIL;

#ifdef _VERBOSE
		printf("Returning Source Address 0x%x\n", (unsigned int)*pDPRAM_SOURCE_ADDRESS);
#endif

		/* Reset flag since we attempted to assign desired source address */
		*pDPRAM_CONFIG &= ~CTRL_CONFIG_RETRIEVE_ADDRESS;
	}
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
fill_J1939_RX_UIF is responsible for filling the receive (RX) FIFO with information that was placed in the j1939_t
struct as a result of processing a D-CAN interrupt. The message is placded in the specified channel's receive FIFO.
               CAN_isr()
                   |
                   V
              fill_J1939_RX_UIF()
</summary>
<param name="rx_m"> : (Input) Receive struct containing CAN message information.</param>
<param name="ch"> : (Input) Channel index (zero based).</param>
<returns>int : Status
	- 0  : SUCCESS
	- Non-Zero : ERROR
</returns>
*/
/**************************************************************************************************************/
int fill_J1939_RX_UIF(j1939_t *rx_m, CHANNEL ch)
{
   int i;

#ifdef _VERBOSE
   unsigned int unTemp = 0;
   printf("BEGIN fill_J1939_RX_UIF - channel %d\r\n", ch+1);
#endif /* _VERBOSE */

   volatile unsigned int *pInterrupt = (volatile unsigned int *)FPGA_INTERRUPT_STATUS;
   volatile unsigned int *pDATA = (volatile unsigned int *)(DPRAM_CH1_RX_DATA + (ch * CHANNEL_DEDICATED_SIZE));
   volatile unsigned int *pRX_SIZE = (volatile unsigned int *)(DPRAM_CH1_RX_USED  + (ch * CHANNEL_DEDICATED_SIZE));
   volatile unsigned int *pDPRAM_CH1_MSG_DROP_COUNT = (volatile unsigned int *)(DPRAM_CH1_MSG_DROP_COUNT + (ch * CHANNEL_DEDICATED_SIZE));
   volatile unsigned int *pDDR_RX_FIFO_FULL_COUNT = (volatile unsigned int *)(FPGA_CH1_DDR_RX_FIFO_FULL_COUNT + (ch * FPGA_CHANNEL_DEDICATED_SIZE));

   /* Now clear entire interrupt since pulsing it should have caused a latch to take place! */
   *pInterrupt = 0;


   /* Check to see if the new CAN Msg will fit on the FIFO. If not..we bump the Msg Drop Count */
   if ((*pRX_SIZE + (MIN_FRAME_LENGTH + rx_m->buf_len)) > *pDDR_RX_FIFO_FULL_COUNT)
   {
	   *pDPRAM_CH1_MSG_DROP_COUNT++;
	   return CAN_NO_FIFO_ROOM;
   }

   *pDATA = (rx_m->pgn >> 9) | START_FLAG; 		// Write PGN_HI, set MSB to signify beginning of message
   *pDATA = (rx_m->pgn & 0x1FF);				// Write PGN_LO
   *pDATA = rx_m->src;							// Write Source Address
   *pDATA = rx_m->dst;							// Write Destination Address

   //If no payload... set the flag to signify this is the end of the message.
   if (rx_m->buf_len == 0)
       *pDATA = (unsigned int)(rx_m->buf_len | END_FLAG);
   else
   {
       *pDATA = (unsigned int)rx_m->buf_len;

	    for (i=0;i<(rx_m->buf_len - 1);i++)
	        *pDATA = (unsigned int)rx_m->buf[i];

	    // Set the flag to signify this is the end of the message.
	    *pDATA = (unsigned int)(rx_m->buf[i] | END_FLAG);
   }

#ifdef _VERBOSE
   unTemp = *pRX_SIZE;
   printf("RX_SIZE = %d\r\n", unTemp);
#endif

   // -------------------------------------------------------
   // --- Interrupt to User Bus (e.g. VME, PCI, Ethernet) ---
   // -------------------------------------------------------
   *pInterrupt = (0x0001 << ch);


#ifdef _VERBOSE
   printf("END fill_J1939_RX_UIF\r\n");
#endif /* _VERBOSE */

   return CAN_SUCCESS;
}


/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
fill_J1939_TX_UIF is responsible for taking data off of the transmit (TX) FIFO and filling out a j1939_t struct for the
given channel.
</summary>
<param name="tx_m"> : (Input) TX struct containing CAN message information.</param>
<param name="ch"> : (Input) Channel index (zero based).</param>
<returns>int : Status
	- 0  : SUCCESS
    - 1  : FIFO is empty
    - 2  : FIFO doesn't have enough data for a full message
    - 3  : FIFO doesn't contain a start flag
    - 4  : tx_m->data_count = NULL when there is data to transmit.
    - 5  : END_FLAG wasn't where it was expected
    - 6  : Bad Size.  Size over 255 payload limit.
</returns>
*/
/**************************************************************************************************************/
int fill_J1939_TX_UIF(j1939_t *tx_m, CHANNEL ch)
{
	int i;
	int nFifo_used;
	int nData_size;
	unsigned short usTemp;
	unsigned short usPGNhi, usPGNlo;

	volatile unsigned int *pDATA    = (volatile unsigned int *)(DPRAM_CH1_TX_DATA  + (ch * CHANNEL_DEDICATED_SIZE));
	volatile unsigned int *pTX_SIZE = (volatile unsigned int *)(DPRAM_CH1_TX_USED  + (ch * CHANNEL_DEDICATED_SIZE));

	nFifo_used = *pTX_SIZE;

//printf("FIFO Used for Channel %d = %d\n", ch+1, nFifo_used);

	// Check to make sure there is data to Transmit!!!
	if (nFifo_used == 0)
	    return CAN_NO_DATA_TO_TX;

	// Scan for a start flag
    for (i=0;i<nFifo_used;i++)
    {
    	usPGNhi = (unsigned short)(*pDATA & 0x0000FFFF);
        if (usPGNhi & START_FLAG)
        	break;              // found beginning
    }

    // If we got to the end of FIFO and didn't find a flag
    if (i == nFifo_used)
        return CAN_TX_START_NOT_FOUND;

    // FIFO must contain at least 5 entries in order to be a valid CAN Frame
	if (nFifo_used < MIN_FRAME_LENGTH)
	    return CAN_TX_MIN_FRAME_VIOLATION;

	usPGNhi &= 0x01FF;
	usPGNlo = *pDATA;
	tx_m->port = (uint8_t)ch;
	tx_m->pgn = (usPGNhi << 9) | usPGNlo;
	tx_m->pri = *pDATA;
	tx_m->dst = *pDATA;
	nData_size = (*pDATA & 0x00FF);

    // --- Check for Bad Data Size ---
	if (nData_size > 255)
        return CAN_BAD_DATA_SIZE;           // The payload can't be more thant 8 bytes.

	tx_m->buf_len = nData_size;

    if (tx_m->buf_len == 0)
        return CAN_SUCCESS;  			// If there's no data, then we're done.

    if (tx_m->buf == NULL)
    	return CAN_TX_NULL_DATA_BUFF;  // Problem: we have data, but no place to write it.

    for (i=0;i<nData_size;i++)
    {
        usTemp = (unsigned short)(*pDATA & 0x0000FFFF);
        tx_m->buf[i] = (usTemp & 0xFF);
        if (usTemp & END_FLAG)
            break;
    }

    if (nData_size != (i + 1))
    {
    	printf("TX END NOT FOUND - Data Size = %d, i+1 = %d\n", nData_size, (i+1));
    	printf("Frame Count = %u\n", *((volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE))));

    	for (i=0; i < 8; i++)
    		printf("buf[%d] = 0x%x\n", i, tx_m->buf[i]);

    	printf("usTemp = 0x%x\n", usTemp);

    	return CAN_TX_END_NOT_FOUND;  // --- END_FLAG wasn't where it was expected ---
    }

#ifdef _VERBOSE
    printf("**********************************\n");
	printf("PGN = %x\n", (unsigned int)tx_m->pgn);
	for (i=0; i < 8; i++)
		printf("buf[%d] = 0x%x\n", i, tx_m->buf[i]);
	printf("**********************************\n");
#endif /* _VERBOSE */

    return CAN_SUCCESS;       // Success
}


/**************************************************************************************************************/
/**
\ingroup CAN_Utils
<summary>
print_J1939_packet is responsible for printing the contents of the j1939_t packet.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
void print_J1939_packet(j1939_t *pCAN_pkt)
{
	int k = 0;
	printf("CAN pkt.pgn = %x\n", (unsigned int)pCAN_pkt->pgn);
	for (k=0; k<pCAN_pkt->buf_len; k++)
	   printf("CAN pkt.buf[%d] = %x\n", k, pCAN_pkt->buf[k]);
}


/**************************************************************************************************************/
/**
\ingroup J1939App
<summary>
j1939app_sa_get - During initialization and when the J1939 module fails to claim an address on this port,
it calls this function to get another address to attempt.  If there are no more addresses, this function
should return 255.
</summary>
<returns>uint8_t - claimed address.  255 if no more addresses. </returns>
*/
/**************************************************************************************************************/
uint8_t j1939app_sa_get ( uint8_t p )
{
  /* source address and attempts made */
  uint8_t sa = 255; /* Initialize to No More Addresses */

  sa = Src_Address[p];

  if (sa == 0 || SA_Attempts[p] > 0)
  {
	  /* keep track of number of attempts per network */
	  SA_Attempts[p]++;

	  /** DANT
	   *  CURRENTLY WE START AT 128 (Global Dynamic Preferred Address Range)
	   *  AND ATTEMPT TO USE ADDRESSES IN ORDER.
	   *  NOTE: THESE ADDRESSES DO CONVEY MEANING BASED UPON THE INDUSTRY GROUP.
	   *  SEE "A COMPREHENSIVE GUIDE TO J1939" PAGE 93. FOR ALL INDUSTRIES IT APPEARS
	   *  THE DYNAMIC PREFERRED ADDRESS RANGE STARTS AT 128..SO THAT IS WHAT WE WILL DO
	   *  HERE! */
	  if (sa == 0)
	  {
		  if ( SA_Attempts[p] == 1 )
			  sa = 128 + p;
		  else
			  sa = 128 + (((p+1) * SA_Attempts[p] * 8));
	  }
	  else /* We want to use a specific Src Address */
	  {
		  /* If attempts > 1 then we have been unsuccessful using desired Source Address so we increment the Source Address by 1 */
		  if ( SA_Attempts[p] > 1 )
			  sa += 1;
	  }

	  /* Most industries dynamic address range ends at 207 so we will use this value..if we still have not obtained
	   * an address and we compute a value above 207, we return 255 to indicate we could not obtain an address.
	   */
	  if (sa > 207)
		  sa = 255;
  }

#ifdef _ORIG
  for (i=0; i < _MAX_NODES; i++)
  {
	  if ( SA_Attempts[p] == 1 )
		  sa = p;
	  else
		  sa = ((p+1) * SA_Attempts[p] * 8);
  }
#endif
  return sa;
}

/**************************************************************************************************************/
/**
\ingroup J1939App
<summary>
j1939app_init is responsible for initializing the application layer.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
void j1939app_init ( void )
{
  uint8_t p;

  /* setup all j1939 networks */
  for( p = 0; p < J1939CFG_PORTS_NUM; p++ )
	  j1939app_init_p(p);
}

/**************************************************************************************************************/
/**
\ingroup J1939App
<summary>
j1939app_init_p is responsible for initializing the application layer for the specified port / channel.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
void j1939app_init_p ( uint8_t p )
{
  j1939_name_t name;

  /* table B1 of j1939 */
  name.aac = 1;             /* 1-bit Arbitrary Address Capable */
  name.ind_grp = 0;         /* 3-bit Industry Group (0=Global, 1=on-highway, 2=Agricultural, 3=Construction, 4=Marine, 5=Industrial-Process Control-Stationary (Generator Sets)) */
  name.veh_sys_inst = 0;    /* 4-bit Vehicle System Instance  */
  name.veh_sys = 1;         /* 7-bit Vehicle System (1=tractor - for ind_grp 1(on-highway)) */
  name.func = 0;          	/* 8-bit Function (130 = data logger - for ind_grp 1(on-highway))*/
  name.func_inst = 0;       /* 5-bit Function Instance */
  name.ecu_inst = 0;        /* 3-bit ECU Instance */
  name.mfg_code = 0;        /* 11-bit Manufacturer Code */
  name.identy_num = 0;      /* 21-bit Identity Number (see below) */

#ifdef _ORIG_SAMPLE_CODE
  name.aac = 1;             /* 1-bit Arbitrary Address Capable */
  name.ind_grp = 1;         /* 3-bit Industry Group (1 = on-highway) */
  name.veh_sys_inst = 0;    /* 4-bit Vehicle System Instance  */
  name.veh_sys = 1;         /* 7-bit Vehicle System (1 = tractor) */
  name.func = 130;          /* 8-bit Function (130 = data logger)*/
  name.func_inst = 0;       /* 5-bit Function Instance */
  name.ecu_inst = 0;        /* 3-bit ECU Instance */
  name.mfg_code = 402;      /* 11-bit Manufacturer Code (402 = SIMMA) */
  name.identy_num = 0;      /* 21-bit Identity Number (see below) */
#endif /*_ORIG_SAMPLE_CODE*/

  if (p < J1939CFG_PORTS_NUM && (j1939_is_port_enabled(p)))
  {
	/* get serial number from eeprom */
	//name.identy_num = ee_serial_num( p );

	/* example to use different NAMES per network */
	j1939_name_set( p, &name );

	/* set babbling-idiot protection to 25% */
    j1939_bip_tx_rate_allowed_set( p, 25 );
//  j1939_bip_tx_rate_allowed_set( p, 100 ); /* DANT - TEMPORARILY TURN OFF BABBLING-IDIOT PROTECTION */

    /* Since we are initializing the current port as a J1939 capable port, we need to set an initial baud rate -
       NOTE: Caller should specify a specific Baud on subsequent call! */
    can_init_baud(p);
  }
}

/**************************************************************************************************************/
/**
\ingroup J1939App
<summary>
j1939app_tx_request is responsible for transmitting a J1939 request message.  Use this for retrieving request
based PGNs (e.g. Vehicle Identification).  To send the request to all ECUs on the network, set dst to 255.
</summary>
<returns>uint8_t - Status - 0 : Success; 1 : Failure</returns>
*/
/**************************************************************************************************************/
uint8_t j1939app_tx_request ( uint8_t p, uint32_t pgn, uint8_t dst )
{
  j1939_t msg;
  uint8_t buf[8];

  msg.pri = 6;
  msg.port = p;
  msg.buf = buf;
  msg.dst = dst;
  msg.buf_len = 3;
  msg.buf[0] = (uint8_t)pgn;
  msg.buf[1] = (uint8_t)(pgn >> 8);
  msg.buf[2] = (uint8_t)(pgn >> 16);
  msg.pgn = J1939_PGN_REQUEST;

  return j1939_tx_app( &msg, 0 );
}

/**************************************************************************************************************/
/**
\ingroup J1939App
<summary>
j1939app_process is responsible for printing the contents of the can_t packet (if currently in verbose mode)
as well as filling the RX FIFO with the J1939 message data that gets passed in.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
void j1939app_process ( j1939_t *msg )
{
//#if defined(_VERBOSE) || defined(_INTERRUPT_VERBOSE)
//	print_J1939_packet(msg);
//#endif /* _VERBOSE || _INTERRUPT_VERBOSE */
/*DANT TO_DO!!!  COMMENT OUT FOR NOW AS WE DO NOT WANT TO OVERFILL OUR FIFO
	fill_J1939_RX_UIF(msg, msg->port);
*/
}

/**************************************************************************************************************/
/**
\ingroup J1939App
<summary>
j1939app_update function is called by the j1939.c at the system tick rate and is used to provide a time
base to the application layer code.  This time base can be used to periodically broadcast PGNs or to
identify when a received PGN has timed out. Currently in NAI module this functionality is a no-op.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
void j1939app_update ( void )
{
  return;
}

#endif

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
fill_AB_RX_UIF is responsible for filling the receive (RX) FIFO with information that was placed in the can_t
struct as a result of processing a D-CAN interrupt. The message is placded in the specified channel's receive FIFO.
               CAN_isr()
                   |
                   V
              fill_AB_RX_UIF()
</summary>
<param name="rx_m"> : (Input) Receive struct containing CAN message information.</param>
<param name="ch"> : (Input) Channel index (zero based).</param>
<returns>int : Status
	- 0  : SUCCESS
	- Non-Zero : ERROR
</returns>
*/
/**************************************************************************************************************/
int fill_AB_RX_UIF(can_t *rx_m, CHANNEL ch)
{
   int i;

#ifdef _VERBOSE
   unsigned int unTemp = 0;
   printf("BEGIN fill_AB_RX_UIF - channel %d\r\n", ch+1);
#endif /* _VERBOSE */

   union Ident_type
   {
	    unsigned char u8[4];
	    unsigned int  u32;
   }uIdent;

   volatile unsigned int *pInterrupt = (volatile unsigned int *)FPGA_INTERRUPT_STATUS;
   volatile unsigned int *pDATA = (volatile unsigned int *)(DPRAM_CH1_RX_DATA + (ch * CHANNEL_DEDICATED_SIZE));
   volatile unsigned int *pRX_SIZE = (volatile unsigned int *)(DPRAM_CH1_RX_USED  + (ch * CHANNEL_DEDICATED_SIZE));
   volatile unsigned int *pDPRAM_CH1_MSG_DROP_COUNT = (volatile unsigned int *)(DPRAM_CH1_MSG_DROP_COUNT + (ch * CHANNEL_DEDICATED_SIZE));
   volatile unsigned int *pDDR_RX_FIFO_FULL_COUNT = (volatile unsigned int *)(FPGA_CH1_DDR_RX_FIFO_FULL_COUNT + (ch * FPGA_CHANNEL_DEDICATED_SIZE));

   /* Now clear entire interrupt since pulsing it should have caused a latch to take place! */
   *pInterrupt = 0;


   /* Check to see if the new CAN Msg will fit on the FIFO. If not..we bump the Msg Drop Count */
   if ((*pRX_SIZE + (MIN_FRAME_LENGTH + rx_m->buf_len)) > *pDDR_RX_FIFO_FULL_COUNT)
   {
	   *pDPRAM_CH1_MSG_DROP_COUNT++;
	   return CAN_NO_FIFO_ROOM;
   }

   uIdent.u32 = (rx_m->id & 0x1FFFFFFF);

   if (!(rx_m->id & B31)) /* CAN A - BIT 31 is not set (0) - CAN B - BIT 31 is set (1) */
	   *pDATA = (unsigned int)(uIdent.u8[3] | START_FLAG | 0x2000);  // High Byte of Message Identifier and flag for CAN-A message.
   else
	   *pDATA = (unsigned int)(uIdent.u8[3] | START_FLAG);           // High Byte of Message Identifier

   *pDATA =  (unsigned int)uIdent.u8[2];
   *pDATA =  (unsigned int)uIdent.u8[1];
   *pDATA =  (unsigned int)uIdent.u8[0];  // Low Byte of Message Identifier

   //If no payload... set the flag to signify this is the end of the message.
   if (rx_m->buf_len == 0)
       *pDATA = (unsigned int)(rx_m->buf_len | END_FLAG);
   else
   {
       *pDATA = (unsigned int)rx_m->buf_len;

	    for (i=0;i<(rx_m->buf_len - 1);i++)
	        *pDATA = (unsigned int)rx_m->buf[i];

	    // Set the flag to signify this is the end of the message.
	    *pDATA = (unsigned int)(rx_m->buf[i] | END_FLAG);
   }

#ifdef _VERBOSE
   unTemp = *pRX_SIZE;
   printf("RX_SIZE = %d\r\n", unTemp);
#endif

   // -------------------------------------------------------
   // --- Interrupt to User Bus (e.g. VME, PCI, Ethernet) ---
   // -------------------------------------------------------
   *pInterrupt = (0x0001 << ch);


#ifdef _VERBOSE
   printf("END fill_AB_RX_UIF\r\n");
#endif /* _VERBOSE */

   return CAN_SUCCESS;
}




/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
fill_AB_TX_UIF is responsible for taking data off of the transmit (TX) FIFO and filling out a can_t struct for the
given channel.
</summary>
<param name="rx_m"> : (Input) Receive struct containing CAN message information.</param>
<param name="ch"> : (Input) Channel index (zero based).</param>
<returns>int : Status
	- 0  : SUCCESS
    - 1  : FIFO is empty
    - 2  : FIFO doesn't have enough data for a full message
    - 3  : FIFO doesn't contain a start flag
    - 4  : tx_m->data_count = NULL when there is data to transmit.
    - 5  : END_FLAG wasn't where it was expected
    - 6  : Bad Size.  Size over 255 payload limit.
</returns>
*/
/**************************************************************************************************************/
int fill_AB_TX_UIF(can_t *tx_m, CHANNEL ch)
{
	int i;
	int nFifo_used;
	int nData_size;
	unsigned short usTemp;

	union Ident_type
	{
	    unsigned char u8[4];
	    unsigned int  u32;
	}uIdent;

	volatile unsigned int *pDATA    = (volatile unsigned int *)(DPRAM_CH1_TX_DATA  + (ch * CHANNEL_DEDICATED_SIZE));
	volatile unsigned int *pTX_SIZE = (volatile unsigned int *)(DPRAM_CH1_TX_USED  + (ch * CHANNEL_DEDICATED_SIZE));

	nFifo_used = *pTX_SIZE;

	// Check to make sure there is data to Transmit!!!
	if (nFifo_used == 0)
	    return CAN_NO_DATA_TO_TX;

	// Scan for a start flag
    for (i=0;i<nFifo_used;i++)
    {
        usTemp = (unsigned short)(*pDATA & 0x0000FFFF);
        if (usTemp & START_FLAG)
        	break;              // found beginning
    }

    // If we got to the end of FIFO and didn't find a flag
    if (i == nFifo_used)
        return CAN_TX_START_NOT_FOUND;

    // FIFO must contain at least 5 entries in order to be a valid CAN Frame
	if (nFifo_used < MIN_FRAME_LENGTH)
	    return CAN_TX_MIN_FRAME_VIOLATION;

    uIdent.u8[3]  = 0x1F & usTemp;		  // mask out control bits and the top 3 bits because this is a 29-bit number
    uIdent.u8[2]  = 0xFF & *pDATA;		  // mask out control bits
    uIdent.u8[1]  = 0xFF & *pDATA;		  // mask out control bits
    uIdent.u8[0]  = 0xFF & *pDATA;		  // mask out control bits
    nData_size = (*pDATA & 0x00FF);  	  // Mask off control bits

    // --- Check for Bad Data Size ---
    if (nData_size > 8)
        return CAN_BAD_DATA_SIZE;           // The payload can't be more thant 8 bytes.

    // --- Put Packet Info Where Calling Function Expects It ---
    tx_m->id = uIdent.u32;
	if ((usTemp & 0x2000) != 0x2000)
		tx_m->id = uIdent.u32 | B31; /*CAN B*/

    tx_m->buf_len = nData_size;

    if (tx_m->buf_len == 0)
        return CAN_SUCCESS;  			// If there's no data, then we're done.

    if (tx_m->buf == NULL)
    	return CAN_TX_NULL_DATA_BUFF;  // Problem: we have data, but no place to write it.

    for (i=0;i<nData_size;i++)
    {
        usTemp = (unsigned short)(*pDATA & 0x0000FFFF);
        tx_m->buf[i] = (usTemp & 0xFF);
        if (usTemp & END_FLAG)
            break;
    }

    if (nData_size != (i + 1))
    {
    	printf("TX END NOT FOUND - Data Size = %d, i+1 = %d\n", nData_size, (i+1));
    	printf("Frame Count = %u\n", *((volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE))));

    	printf("ID = %x\n", (unsigned int)tx_m->id);
    	for (i=0; i < 8; i++)
    		printf("buf[%d] = 0x%x\n", i, tx_m->buf[i]);

    	printf("usTemp = 0x%x\n", usTemp);

    	return CAN_TX_END_NOT_FOUND;  // --- END_FLAG wasn't where it was expected ---
    }

#ifdef _VERBOSE
    printf("**********************************\n");
	printf("ID = %x\n", (unsigned int)tx_m->id);
	for (i=0; i < 8; i++)
		printf("buf[%d] = 0x%x\n", i, tx_m->buf[i]);
	printf("**********************************\n");
#endif /* _VERBOSE */

    return CAN_SUCCESS;       // Success
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
mirror_status_register is responsible for mirroring status register to the outside world
</summary>
<param name="ch"> : (Input) Channel index (zero based).</param>
<returns>void</returns>
*/
/**************************************************************************************************************/
void mirror_status_register(CHANNEL ch)
{
	uint32_t unRegStatus = 0;
	volatile unsigned int *pDPRAM_CORE_STATUS = (volatile unsigned int *)(DPRAM_CH1_CORE_STATUS + (ch * CHANNEL_DEDICATED_SIZE));

	/*****************************************************************/
	/* Core Status 			 										 */
	/*****************************************************************/
	if (can_getCoreStatusRegisterContents(ch, &unRegStatus) == CAN_SUCCESS)
		*pDPRAM_CORE_STATUS = unRegStatus;
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
clear_all_reported_status is responsible for clearing all of the status registers that are used to convey
error information to the caller. NOTE: currently this method does not attempt to clear any errors stored in the
CAN registers themselves. It merely clears those registers we use to report to the outside world.
</summary>
<param name="ch"> : (Input) Channel index (zero based).</param>
<returns>void</returns>
*/
/**************************************************************************************************************/
void clear_all_reported_status(CHANNEL ch)
{
	volatile unsigned int *pDPRAM_LAST_ERR_CODE = (volatile unsigned int *)(DPRAM_CH1_LAST_ERR_CODE + (ch * CHANNEL_DEDICATED_SIZE));
	volatile unsigned int *pDPRAM_CERC = (volatile unsigned int *)(DPRAM_CH1_CERC + (ch * CHANNEL_DEDICATED_SIZE));
	volatile unsigned int *pDPRAM_BUS_STATUS = (volatile unsigned int *)(DPRAM_CH1_BUS_STATUS + (ch * CHANNEL_DEDICATED_SIZE));
	volatile unsigned int *pDPRAM_CORE_STATUS = (volatile unsigned int *)(DPRAM_CH1_CORE_STATUS + (ch * CHANNEL_DEDICATED_SIZE));
	volatile unsigned int *pDPRAM_COMM_STATUS = (volatile unsigned int *)(DPRAM_CH1_COMM_STATUS + (ch * CHANNEL_DEDICATED_SIZE));

	*pDPRAM_LAST_ERR_CODE = 0;
	*pDPRAM_CERC = 0;
	*pDPRAM_BUS_STATUS = 0;
	*pDPRAM_CORE_STATUS = 0;
	*pDPRAM_COMM_STATUS = 0;

	/* Reset bookkeeping of last pending interrupt per channel */
	LastPendingIntr[ch] = 0;
	PendingIntrCount[ch] = 0;
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
getXilinxAcceptanceFilterMaskFromFilterControl is responsible for forming the acceptance mask filter in the
format that Xilinx expects.
</summary>
<param name="ch"> : (Input) Channel index (zero based).</param>
<param name="filter"> : (Input) filter - filter index of desired filter (0 - 3)</param>
<param name="filterControlRegValue"> : (Input) filterControlRegValue - filter control register indicating what should be used for filtering </param>
<returns>uint32_t - Filter Mask 32 bit value that Xilinx understands</returns>
*/
/**************************************************************************************************************/
static uint32_t getXilinxAcceptanceFilterMaskFromFilterControl(CHANNEL ch, uint32_t filter, uint32_t filterControlRegValue)
{
	uint32_t acceptanceFilterMask = 0;
	uint32_t dpramAcceptanceMask = 0;
	uint8_t  subRTR = 0;
	uint8_t  rtr = 0;
	uint8_t  idExtension = 0;
	uint32_t extendedID = 0;
	uint32_t standardID = 0;

	volatile unsigned int *pDPRAM_AC_MASK = (volatile unsigned int *)(DPRAM_CH1_ACPT_MASK + (ch * CHANNEL_DEDICATED_SIZE) + ((filter-1) * CHANNEL_FILTER_PAIR_ADDRESS_SKIP));

	if ((filterControlRegValue & FILTCTRL_ENABLE_ACC_CODE_AND_MASK(filter)) > 0)
	{
		dpramAcceptanceMask = *pDPRAM_AC_MASK;

		/* Extended portion of 29bit mask is the 1st 18 bits of the mask register */
		extendedID = (dpramAcceptanceMask & 0x0003FFFF);
		standardID = ((dpramAcceptanceMask >> 18) & 0x000007FF);
	}

	subRTR = ((filterControlRegValue & FILTCTRL_ENABLE_SRR_FILT(filter)) == FILTCTRL_ENABLE_SRR_FILT(filter));
	rtr = ((filterControlRegValue & FILTCTRL_ENABLE_RTR_FILT(filter)) == FILTCTRL_ENABLE_RTR_FILT(filter));
	idExtension = ((filterControlRegValue & FILTCTRL_ENABLE_AB_FILT(filter)) == FILTCTRL_ENABLE_AB_FILT(filter));

	/* DANT - Added - apparently needed in order for filter mask to be applied to CAN B (29 bit identifiers) */
	if (!idExtension && extendedID != 0x00000000)
		idExtension = 1;

	acceptanceFilterMask = Can_CreateIdValue(standardID, subRTR, idExtension, extendedID, rtr);

	return acceptanceFilterMask;
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
getXilinxAcceptanceFilterIDFromFilterControl is responsible for forming the acceptance ID filter in the
format that Xilinx expects.
</summary>
<param name="ch"> : (Input) Channel index (zero based).</param>
<param name="filter"> : (Input) filter - filter index of desired filter (0 - 3)</param>
<param name="filterControlRegValue"> : (Input) filterControlRegValue - filter control register indicating what should be used for filtering </param>
<returns>uint32_t - Filter ID 32 bit value that Xilinx understands</returns>
*/
/**************************************************************************************************************/
static uint32_t getXilinxAcceptanceFilterIDFromFilterControl(CHANNEL ch, uint32_t filter, uint32_t filterControlRegValue)
{
	uint32_t acceptanceFilterID = 0;
	uint32_t dpramAcceptanceID = 0;
	uint8_t  subRTR = 0;
	uint8_t  rtr = 0;
	uint8_t  idExtension = 0;
	uint8_t  specifyABFilter = 0;
	uint32_t extendedID = 0;
	uint32_t standardID = 0;

	volatile unsigned int *pDPRAM_AC_CODE = (volatile unsigned int *)(DPRAM_CH1_ACPT_CODE + (ch * CHANNEL_DEDICATED_SIZE) + ((filter-1) * CHANNEL_FILTER_PAIR_ADDRESS_SKIP));


	if ((filterControlRegValue & FILTCTRL_ENABLE_ACC_CODE_AND_MASK(filter)) > 0)
	{
		dpramAcceptanceID = *pDPRAM_AC_CODE;

		/* Extended portion of 29bit mask is the 1st 18 bits of the mask register */
		extendedID = (dpramAcceptanceID & 0x0003FFFF);
		standardID = ((dpramAcceptanceID >> 18) & 0x000007FF);
	}

	subRTR = ((filterControlRegValue & FILTCTRL_ENABLE_SRR_FILT(filter)) == FILTCTRL_ENABLE_SRR_FILT(filter));
	rtr = ((filterControlRegValue & FILTCTRL_ENABLE_RTR_FILT(filter)) == FILTCTRL_ENABLE_RTR_FILT(filter));
	specifyABFilter = ((filterControlRegValue & FILTCTRL_ENABLE_AB_FILT(filter)) == FILTCTRL_ENABLE_AB_FILT(filter));

	/* If specifying AB filter... let's consult user request */
	if (specifyABFilter)
	{
		if ((filterControlRegValue & FILTCTRL_STDEXT_FILT(filter)) == 0)
			idExtension = 1;
	}
	else /* If user not specifying a specific CAN type (standard CAN A or extended CAN B), but an extended ID is being used for the acceptance filter, then we need to specify the extendedId bit */
	{
		if ( extendedID != 0x00000000)
			idExtension = 1;
	}

	acceptanceFilterID = Can_CreateIdValue(standardID, subRTR, idExtension, extendedID, rtr);


	return acceptanceFilterID;
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
changes_made_to_filter_mask_or_code is responsible for determining if any changes were made to the user supplied
mask or filter for the enabled filters.
</summary>
<param name="ch"> : (Input) Channel index (zero based).</param>
<param name="filterControlRegValue"> : (Input) filterControlRegValue - filter control register indicating what should be used for filtering </param>
<returns>bool - TRUE if changes were made; FALSE otherwise</returns>
*/
/**************************************************************************************************************/
static bool changes_made_to_filter_mask_or_code(CHANNEL ch, uint32_t filterControlRegValue)
{
	bool changesMade = FALSE;
	uint32_t numFiltersToEnable = 0;
	uint32_t enabledFilters = 0;
	uint32_t i = 0;
	volatile unsigned int *pDPRAM_AC_MASK;
	volatile unsigned int *pDPRAM_AC_CODE;

	/* Check to see how many filters were enabled */
	enabledFilters = (RX_UIF[ch].filterCtrl & FILTCTRL_ENABLE_FILTER_MASK);

	/* Only interested in checking those filter pairs that are actually enabled (or being told to be enabled) */
	numFiltersToEnable = (filterControlRegValue & FILTCTRL_ENABLE_FILTER_MASK);

	/* Make sure we are within bounds of capability...else force within bounds */
	if (numFiltersToEnable > MAX_CHANNEL_FILTER_COUNT)
		numFiltersToEnable = MAX_CHANNEL_FILTER_COUNT;

	if (enabledFilters != numFiltersToEnable)
	{
		changesMade = TRUE;
//		printf("Num Filters Different!!!!\n");
	}

	/* Only need to continue checking if we have not yet found a change */
	if (!changesMade)
	{
		for (i=0 ; i < numFiltersToEnable; i++)
		{
			pDPRAM_AC_MASK = (volatile unsigned int *)(DPRAM_CH1_ACPT_MASK + (ch * CHANNEL_DEDICATED_SIZE) + (i * CHANNEL_FILTER_PAIR_ADDRESS_SKIP));
			pDPRAM_AC_CODE = (volatile unsigned int *)(DPRAM_CH1_ACPT_CODE + (ch * CHANNEL_DEDICATED_SIZE) + (i * CHANNEL_FILTER_PAIR_ADDRESS_SKIP));

			if (*pDPRAM_AC_MASK != RX_UIF[ch].ac_msk[i] ||
				*pDPRAM_AC_CODE != RX_UIF[ch].ac_code[i])
			{
				changesMade = TRUE;
				break;
			}
		}
	}

	return changesMade;
}

/**************************************************************************************************************/
/**
\ingroup CAN_Utils
<summary>
wait_microsecond is responsible for waiting the desired number of microseconds prior to continuing.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
void wait_microsecond(unsigned int microseconds)
{
	usleep(microseconds);
}

/**************************************************************************************************************/
/**
\ingroup CAN_Utils
<summary>
print_AB_packet is responsible for printing the contents of the can_t packet.
</summary>
<returns>VOID</returns>
*/
/**************************************************************************************************************/
void print_AB_packet(can_t *pCAN_pkt)
{
	int k = 0;
/*
	if (pCAN_pkt == NULL)
		return;
*/
	printf("CAN pkt.id = %x\n", (unsigned int)pCAN_pkt->id);
	for (k=0; k<pCAN_pkt->buf_len; k++)
	   printf("CAN pkt.buf[%d] = %x\n", k, pCAN_pkt->buf[k]);
}




#if (MODULE_CB2 || MODULE_CB3)

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
canj1939_tx_test is responsible for performing a simple CAN J1939 transmit.
</summary>
<param name="ch"> : (Input) Channel index (zero based) to tx over.</param>
<returns>bool : true if success; false otherwise</returns>
*/
/**************************************************************************************************************/
bool canj1939_tx_test(CHANNEL ch)
{
	bool bSuccess = FALSE;
	j1939_t msg;
	uint8_t buf[16] = {0,1,2,3,4,5,6,7};

	/* load message */
	msg.pgn = 65215;  /*J1939 Wheel Speed Info*/
	msg.buf = buf;
	msg.buf_len = 8;
	msg.dst = 255;
	msg.pri = 7;
	msg.port = ch;

	/* transmit message */
	if (j1939_tx_app(&msg, 0) == CAN_SUCCESS)
	{
		printf("Message transmitted\n");
		bSuccess = TRUE;
	}
	else
	{
		printf("Message not transmitted\n");
		bSuccess = FALSE;
	}
	return bSuccess;
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
do_canj1939_tx_flood_test is responsible for performing J1939 TX test of a specified number of J1939 messages
being sent on the desired channel.
</summary>
<param name="ch"> : (Input) Channel index (zero based) to tx over.</param>
<param name="nMsgCount"> : (Input) Number of messages to transmit.</param>
<returns>bool : true if success; false otherwise</returns>
*/
/**************************************************************************************************************/
bool canj1939_tx_flood_test(CHANNEL ch, int nMsgCount)
{
	int i = 0;
	int nFailureCount = 0;
	CHANNEL startChannel = ch;
	CHANNEL endChannel = (ch+1);
	CHANNEL currentChannel = ch;
	uint8_t status = CAN_SUCCESS;
	uint32_t txCounter = 0;

	j1939_t msg;
	uint8_t buf[16] = {0,1,2,3,4,5,6,7};

	j1939_disable_bip_all_ports();

	/* If -1 was passed in as ch...caller wants to flood msgs on all channels */
	if (ch == ALL_CHANNELS)
	{
		startChannel = CHANNEL_1;
		endChannel = NUM_CHANNELS;
	}

	/* load message */
	msg.pgn = 65215;  /*J1939 Wheel Speed Info*/
	msg.buf = buf;
	msg.buf_len = 8;
	msg.dst = 255;
	msg.pri = 7;

	for (i=0; i < nMsgCount; i++)
	{
		for (currentChannel = startChannel; currentChannel < endChannel; currentChannel++)
		{
			/* transmit message */
			msg.port = currentChannel;

			status = j1939_tx_app(&msg, 0);
			txCounter++;

			if (status != CAN_SUCCESS)
			{
				nFailureCount++;
				printf("Error: 0x%x TX on Channel %d\r\n", status, currentChannel);
			}

			wait_microsecond(750);

			/* force j1939_update on every n TXs - this is so we don't run out of FIFO space */
			if ((txCounter % 100) == 0)
				j1939_update();
		}
	}

	j1939_enable_bip_all_ports(25);

	return ((nFailureCount == 0)?TRUE:FALSE);
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
canj1939_rx_test is responsible for performing a simple CAN J1939 receive.
</summary>
<param name="ch"> : (Input) Channel index (zero based) to rx on.</param>
<returns>bool : true if success; false otherwise</returns>
*/
/**************************************************************************************************************/
bool canj1939_rx_test(CHANNEL ch)
{
	bool bMsgReceived = FALSE;
	j1939_t msg;
	uint8_t DataBuf[8] = {0,0,0,0,0,0,0,0};

	msg.buf = &DataBuf[0];
	while (j1939_can_rx(ch, &msg) == CAN_SUCCESS)
	{
		bMsgReceived = TRUE;
		printf("Message received on channel %d\n", ch+1);

		/* Check to see if received message matches what should have been sent */
		print_J1939_packet(&msg);
	}

	return bMsgReceived;
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
canj1939_filter_test is responsible for enabling a filter on the specified channel such that only certain CAN
messages get passed through.
</summary>
<param name="ch"> : (Input) Channel index (zero based) to be reset.</param>
<returns>bool TRUE if success; FALSE otherwise</returns>
*/
/**************************************************************************************************************/
bool canj1939_filter_test(CHANNEL ch)
{
	bool bSuccess = FALSE;
	int nBusyCount = 0;
	const int MAX_BUSY_RETRIES = 1000;
	j1939_t tst_pkt1;
	j1939_t tst_pkt2;
	uint8_t ucAcceptFilterBusy = 0;
	uint8_t status;
	uint8_t DataBuf1[8] = {0,0,0,0,0,0,0,0};
	uint8_t DataBuf2[8] = {0,0,0,0,0,0,0,0};
	uint8_t ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1 = 0;
	CHANNEL tx_ch = CHANNEL_1;

	volatile unsigned int *pRX_FRAME_CNT = (volatile unsigned int *)(DPRAM_CH1_RX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE));

	/* If channel is less than 8, than we tx on the channel one above the channel we are testing the filter on */
	if (ch < (NUM_CHANNELS - 1))
		tx_ch = (ch + 1);

	/* Provide buffers to J1939 buffer pointers */
	tst_pkt1.buf = &DataBuf1[0];
	tst_pkt2.buf = &DataBuf2[0];

	/* Get current baud rate..so we can re-initialize CAN with same when done */
	can_getBaudRatePrescaler(ch, &ucPrescaler);
	can_getBitTiming(ch, &ucSJW, &ucTimeSeg2, &ucTimeSeg1);

	/* First lets make sure we start with a cleared CAN channel */
	reset_channel(ch, ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1, TRUE);


	if (can_acceptFilterDisable(ch, CAN_AFR_UAF1_MASK) == CAN_SUCCESS)
	{
		do
		{
			if (can_isAcceptFilterBusy(ch, &ucAcceptFilterBusy) != CAN_SUCCESS)
				return FALSE;

			if (ucAcceptFilterBusy)
			{
				nBusyCount++;
				wait_microsecond(500);
				if (nBusyCount > MAX_BUSY_RETRIES)
					break;
			}
		} while (ucAcceptFilterBusy);

		if (nBusyCount <= MAX_BUSY_RETRIES)
		{
			uint32_t acceptFilterID = (0xFEBF<<1); //ACCEPT ONLY FEBF
			if (can_acceptFilterSet(ch, CAN_AFR_UAF1_MASK, CAN_IDR_ID2_MASK, acceptFilterID) == CAN_SUCCESS)
			{
				if (can_acceptFilterEnable(ch, CAN_AFR_UAF1_MASK) == CAN_SUCCESS)
				{
					/* Make sure acceptance filter enable had enough time. (delay just to be sure) */
					wait_microsecond(1000);

					/* Now transmit one CAN-A message (should be accepted) and 1 CAN-B message (should be rejected) */
					tst_pkt1.pgn = 0xFEBF;  /*J1939 Wheel Speed Info*/
					tst_pkt1.buf[0] = 0xA;
					tst_pkt1.buf[1] = 0xC;
					tst_pkt1.buf[2] = 0xE;
					tst_pkt1.buf[3] = 0xD;
					tst_pkt1.buf[4] = 0xA;
					tst_pkt1.buf[5] = 0xC;
					tst_pkt1.buf[6] = 0xE;
					tst_pkt1.buf[7] = 0xD;
					tst_pkt1.buf_len = 8;
					tst_pkt1.pri = 7;
					tst_pkt1.port = tx_ch;
					tst_pkt1.dst = 255;
					j1939_tx_app(&tst_pkt1, &status);

					wait_microsecond(1000);
					tst_pkt2.pgn = 0xAABB;
					tst_pkt2.buf[0] = 0xF;
					tst_pkt2.buf[1] = 0xE;
					tst_pkt2.buf[2] = 0xD;
					tst_pkt2.buf[3] = 0xC;
					tst_pkt2.buf[4] = 0xB;
					tst_pkt2.buf[5] = 0xA;
					tst_pkt2.buf[6] = 0x9;
					tst_pkt2.buf[7] = 0x8;
					tst_pkt2.buf_len = 8;
					tst_pkt2.pri = 7;
					tst_pkt2.dst = 255;
					tst_pkt2.port = tx_ch;
					j1939_tx_app(&tst_pkt2, &status);

					wait_microsecond(1000);

					/* NOTE: CAN Interrupt will take care of rx CAN messages that are not filtered...so here we check
					 	     the frame count to see if we received the desired number of frames - if all 8 channels are
					 	     looped to one another...we should receive a total of 1 frame per FIFO for all channels except
					 	     channel that performed the TX. The 2nd message should have been filtered out on the specified
					 	     channel but received on all other channels */
					if (*pRX_FRAME_CNT == 1)
						bSuccess = TRUE;
					else
						printf("RX Frame Count of %d did not match expected count of 1\n", *pRX_FRAME_CNT);
				}
			}
		}
	}

	return bSuccess;
}


#endif /*#if (MODULE_CB2 || MODULE_CB3) */


#if (MODULE_CB1 || MODULE_CB3)
/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
canA_host_tx_test is responsible for performing a simple CAN-A transmit (TX) test.
</summary>
<param name="ch"> : (Input) Channel index (zero based) of channel to check for possible transmission.</param>
<returns>bool: true if success; false otherwise</returns>
*/
/**************************************************************************************************************/
bool canA_host_tx_test(CHANNEL ch)
{
	can_t tst_pkt;

	tst_pkt.id = 0x123;
	tst_pkt.buf[0] = 1;
	tst_pkt.buf[1] = 2;
	tst_pkt.buf[2] = 3;
	tst_pkt.buf[3] = 4;
	tst_pkt.buf[4] = 5;
	tst_pkt.buf[5] = 6;
	tst_pkt.buf[6] = 7;
	tst_pkt.buf[7] = 8;
	tst_pkt.buf_len = 8;

	if (can_tx(ch, &tst_pkt) != 0)
	{
	   printf("Error in canA_host_tx_test():can_tx call\n");
	   return FALSE;
	}

	return TRUE;
}

//#ifdef _XXX
/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
canA_host_rx_test is responsible for performing a simple CAN-A receive (RX) test. A specified data pattern of
1, 2, 3, 4, 5, 6, 7, 8 is expected to be sent with an 11-Bit CAN-A ID of 0x123.
</summary>
<param name="ch"> : (Input) Channel index (zero based) of channel to check for possible transmission.</param>
<returns>bool: true if success; false otherwise</returns>
*/
/**************************************************************************************************************/
bool canA_host_rx_test(CHANNEL ch)
{
	int i = 0;
	bool bSuccess = TRUE;
	can_t RXtst_pkt;

/*DANT 6-10-14
	can_rx_isr(ch);
*/
/*DANT_XXX
//NEW DANT 6-10-14
#ifdef _ENABLE_INTERRUPTS
	can_intrHandler(ch);
#endif
//END
 */
	if (can_rx(ch, &RXtst_pkt) != 0)
	   printf("Error in canA_host_rx_test():can_rx call\n");

	//DEBUG PRINTOUT
	printf("CAN-A pkt.id = %x\n", (unsigned int)RXtst_pkt.id);
	for (i=0; i<8; i++)
	   printf("CAN-A pkt.buf[%d] = %x\n", i, RXtst_pkt.buf[i]);
	//END DEBUG

	if ((RXtst_pkt.id & 0x1FFFFFFF) == 0x123 &&
		RXtst_pkt.buf_len == 8 &&
		RXtst_pkt.buf[0] == 1 &&
		RXtst_pkt.buf[1] == 2 &&
		RXtst_pkt.buf[2] == 3 &&
		RXtst_pkt.buf[3] == 4 &&
		RXtst_pkt.buf[4] == 5 &&
		RXtst_pkt.buf[5] == 6 &&
		RXtst_pkt.buf[6] == 7 &&
		RXtst_pkt.buf[7] == 8)
	  bSuccess = TRUE;
	else
	  bSuccess = FALSE;

	fill_AB_RX_UIF(&RXtst_pkt, ch);

	return bSuccess;
}

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
canB_host_tx_test is responsible for performing a simple CAN-B transmit (TX) test.
</summary>
<param name="ch"> : (Input) Channel index (zero based) of channel to check for possible transmission.</param>
<returns>bool: true if success; false otherwise</returns>
*/
/**************************************************************************************************************/
bool canB_host_tx_test(CHANNEL ch)
{
	can_t tst_pkt;

	tst_pkt.id = 0x12345678;

	/* Identify CAN message as a 29-Bit CAN-B message */
	tst_pkt.id |= B31; /*CAN B*/

	tst_pkt.buf[0] = 0xF;
	tst_pkt.buf[1] = 0xE;
	tst_pkt.buf[2] = 0xD;
	tst_pkt.buf[3] = 0xC;
	tst_pkt.buf[4] = 0xB;
	tst_pkt.buf[5] = 0xA;
	tst_pkt.buf[6] = 0x9;
	tst_pkt.buf[7] = 0x8;
	tst_pkt.buf_len = 8;

	if (can_tx(ch, &tst_pkt) != 0)
	{
	   printf("Error in canB_host_tx_test():can_tx call\n");
	   return FALSE;
	}

	return TRUE;
}


bool canB_host_tx_flood_test(CHANNEL ch, int nMsgCount)
{
	bool bStatus = TRUE;
	can_t tst_pkt;
	int i = 0;
	int nFailureCount = 0;

	tst_pkt.id = 0x00000001;

	/* Identify CAN message as a 29-Bit CAN-B message */
	tst_pkt.id |= B31; /*CAN B*/

	tst_pkt.buf[0] = 0xD;
	tst_pkt.buf[1] = 0xE;
	tst_pkt.buf[2] = 0xA;
	tst_pkt.buf[3] = 0xD;
	tst_pkt.buf[4] = 0xB;
	tst_pkt.buf[5] = 0xE;
	tst_pkt.buf[6] = 0xE;
	tst_pkt.buf[7] = 0xF;
	tst_pkt.buf_len = 8;

	for (i=0; i < nMsgCount; i++)
	{
//		printf("can_tx Msg %d of %d\n", i+1, nMsgCount);

		if (can_tx(ch, &tst_pkt) != CAN_SUCCESS)
		{
		   bStatus = FALSE;
		   nFailureCount++;
		}
		tst_pkt.id++;

		wait_microsecond(350);
	}

	if (nFailureCount > 0)
		printf("can_tx FailureCount = %d\r\n", nFailureCount);

	return bStatus;
}

/**************************************************************************************************************/
/**
\ingroup CAN_MenuTests
<summary>
canB_host_rx_test is responsible for performing a simple CAN-B receive (RX) test. A specified data pattern of
F, E, D, C, B, A, 9, 8 is expected to be sent with an 29-Bit CAN-A ID of 0x12345678.
</summary>
<param name="ch"> : (Input) Channel index (zero based) of channel to check for possible transmission.</param>
<returns>bool: true if success; false otherwise</returns>
*/
/**************************************************************************************************************/
bool canB_host_rx_test(CHANNEL ch)
{
	int i = 0;
	bool bSuccess = TRUE;
	can_t RXtst_pkt;

/* NEW DANT 6-10-14
	can_rx_isr(ch);
*/
/*DANT_XXX
	//NEW DANT 6-10-14
#ifdef _ENABLE_INTERRUPTS
		can_intrHandler(ch);
#endif
*/
	//END

	if (can_rx(ch, &RXtst_pkt) != 0)
	   printf("Error in canB_host_rx_tst():can_rx call\n");

	//DEBUG PRINTOUT
	printf("CAN-B pkt.id = %x\n", (unsigned int)(RXtst_pkt.id & 0x1FFFFFFF));
	for (i=0; i<8; i++)
	   printf("CAN-B pkt.buf[%d] = %x\n", i, RXtst_pkt.buf[i]);
	//END DEBUG

	if ((RXtst_pkt.id & 0x1FFFFFFF) == 0x12345678 &&
		RXtst_pkt.buf_len == 8 &&
		RXtst_pkt.buf[0] == 0xF &&
		RXtst_pkt.buf[1] == 0xE &&
		RXtst_pkt.buf[2] == 0xD &&
		RXtst_pkt.buf[3] == 0xC &&
		RXtst_pkt.buf[4] == 0xB &&
		RXtst_pkt.buf[5] == 0xA &&
		RXtst_pkt.buf[6] == 0x9 &&
		RXtst_pkt.buf[7] == 0x8)
	  bSuccess = TRUE;
	else
	  bSuccess = FALSE;

	fill_AB_RX_UIF(&RXtst_pkt, ch);

	return bSuccess;
}


#ifdef _NEED_MORE_DEBUG_INFO
void can_fifo_tx_test(CHANNEL ch)
{
#ifdef _DANT
	int nMsgSentCount = 0;
	int nMsgFrameSize = 13;
	int i = 0;
	int nErrorCnt = 0;
	unsigned short usFrameData[13];

	volatile unsigned int *pDATA = (volatile unsigned int *)(DPRAM_CH1_TX_DATA  + (ch * CHANNEL_DEDICATED_SIZE));

	while (nMsgSentCount < 4000)
	{
		nMsgSentCount++;

		memset(usFrameData, 0, 13*sizeof(unsigned short));

		for (i=0; i < nMsgFrameSize; i++)
			usFrameData[i] = (unsigned short)(*pDATA);

		if ((usFrameData[0] != 0x8000) ||
		   (usFrameData[1] != 0x0000) ||
//		   (usFrameData[2] != ((nMsgSentCount << 16) & 0x00FF)) ||
//		   (usFrameData[3] != ((nMsgSentCount << 8) & 0x00FF)) ||
		   (usFrameData[4] != 0x0008) ||
		   (usFrameData[5] != 0x000D) ||
		   (usFrameData[6] != 0x000E) ||
		   (usFrameData[7] != 0x000A) ||
		   (usFrameData[8] != 0x000D) ||
		   (usFrameData[9] != 0x000B) ||
		   (usFrameData[10] != 0x000E) ||
		   (usFrameData[11] != 0x000E) ||
		   (usFrameData[12] != 0x400F))
	    {
	    	nErrorCnt++;
	    	printf("************************************************\n");
	    	printf("Msg Error - TX Read Index = %d\n", nMsgSentCount);
	    	for (i=0; i < nMsgFrameSize; i++)
	    		printf("usFrameData[%d] = 0x%4x\n", i, usFrameData[i]);
	    	printf("************************************************\n");

	    	break;
		}
	}
#endif /* _DANT */
#ifdef _DANT
	int nMsgSentCount = 0;
	int nMsgFrameSize = 13;
	int i = 0;
	int nErrorCnt = 0;
	unsigned short usFrameData[13];
	int frame_count;

	volatile unsigned int *pDATA = (volatile unsigned int *)(DPRAM_CH1_TX_DATA  + (ch * CHANNEL_DEDICATED_SIZE));

	do
	{
		for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
		{
//			volatile unsigned int *pDPRAM_CONFIG 		= (volatile unsigned int *)(DPRAM_CH1_CONTROL      	+ (ch * CHANNEL_DEDICATED_SIZE));
			volatile unsigned int *pDPRAM_TX_FRAMECNT 	= (volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE));

			memset(usFrameData, 0, 13*sizeof(unsigned short));

			frame_count = *pDPRAM_TX_FRAMECNT;
			if (frame_count > 0)
			{
				nMsgSentCount++;

				for (i=0; i < nMsgFrameSize; i++)
				{
					usFrameData[i] = (unsigned short)(*pDATA);
				}

				if ((usFrameData[0] != 0x8000) ||
				   (usFrameData[1] != 0x0000) ||
		//		   (usFrameData[2] != ((nMsgSentCount << 16) & 0x00FF)) ||
		//		   (usFrameData[3] != ((nMsgSentCount << 8) & 0x00FF)) ||
				   (usFrameData[4] != 0x0008) ||
				   (usFrameData[5] != 0x000D) ||
				   (usFrameData[6] != 0x000E) ||
				   (usFrameData[7] != 0x000A) ||
				   (usFrameData[8] != 0x000D) ||
				   (usFrameData[9] != 0x000B) ||
				   (usFrameData[10] != 0x000E) ||
				   (usFrameData[11] != 0x000E) ||
				   (usFrameData[12] != 0x400F))
				{
					nErrorCnt++;
					printf("************************************************\n");
					printf("Msg Error - TX Read Index = %d\n", nMsgSentCount);
					for (i=0; i < nMsgFrameSize; i++)
						printf("usFrameData[%d] = 0x%4x\n", i, usFrameData[i]);
					printf("************************************************\n");

					break;
				}
			}
		}

		//Channel 1 Frame_count
		frame_count = *((volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT));

	} while (frame_count != 0);

	printf("ErrorCnt = %d\n", nErrorCnt);
#endif /* _DANT */

#ifdef _DANT
	int nErrorCnt = 0;
	unsigned short usFrameData[13];
	int frame_count;

	do
	{
		for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
		{
//			volatile unsigned int *pDPRAM_CONFIG 		= (volatile unsigned int *)(DPRAM_CH1_CONTROL      	+ (ch * CHANNEL_DEDICATED_SIZE));
			volatile unsigned int *pDPRAM_TX_FRAMECNT 	= (volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE));

			memset(usFrameData, 0, 13*sizeof(unsigned short));

			frame_count = *pDPRAM_TX_FRAMECNT;
			if (frame_count > 0)
			{
				fill_test(ch);
			}
		}

		//Channel 1 Frame_count
		frame_count = *((volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT));

	} while (frame_count != 0);

	printf("ErrorCnt = %d\n", nErrorCnt);
#endif /* _DANT */

#ifdef _DANT
	int frame_count;
	int ret_val;
	can_t frame;

	do
	{
		for (ch = CHANNEL_1; ch < NUM_CHANNELS; ++ch)
		{
			volatile unsigned int *pDPRAM_CONFIG 		= (volatile unsigned int *)(DPRAM_CH1_CONTROL      	+ (ch * CHANNEL_DEDICATED_SIZE));
			volatile unsigned int *pDPRAM_TX_FRAMECNT 	= (volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE));

			frame_count = *pDPRAM_TX_FRAMECNT;

			if ( (*pDPRAM_CONFIG & CONFIG_TX) &&   // Is TX enabled?
				 (frame_count != 0) &&             // Is there Data to go out?
				 (can_txIsEmpty(ch) == true) )
			{
				ret_val = fill_TX_UIF(&frame,ch);
				if (ret_val != CAN_SUCCESS)
				{
					printf("Frame Count Before Clear = %d\n", *((volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE))));
					// --- Clear out the TX FIFO ---
					volatile unsigned int *pFPGA_HPS_CONTROL = (volatile unsigned int *)(FPGA_HPS_CONTROL);
					*pFPGA_HPS_CONTROL = (0x0100 << ch);  /* Clear the TX FIFO for desired channel */
					udelay(100); 		    //TEMP
					*pFPGA_HPS_CONTROL = 0; //TEMP

					printf("ERROR %d DETECTED IN check_for_tx - FIFO was emptied!\n", ret_val);
					printf("Frame Count After Clear = %d\n", *((volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT  + (ch * CHANNEL_DEDICATED_SIZE))));
				}
			}

			//Channel 1 Frame_count
			frame_count = *((volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT));
		}
	}while (frame_count != 0);
#endif /* _DANT */

//#ifdef _DANT
	int nMsgSentCount = 0;
	int nErrorCnt = 0;
	int ret_val;
	can_t frame;

	while (nMsgSentCount < 4000)
	{
		nMsgSentCount++;

		ret_val = fill_AB_TX_UIF(&frame,ch);
		if (ret_val != CAN_SUCCESS)
		{
			nErrorCnt++;
			printf("ERROR\n");
		}
	}
	if (nErrorCnt > 0)
		printf("TOTAL ERROR COUNT = %d\n", nErrorCnt);
	else
		printf("SUCCESS\n");
//#endif /* _DANT */
}
#endif

int fill_IncomingTXFifo(can_t *rx_m, CHANNEL ch)
{
   int i;

#ifdef _VERBOSE
   printf("BEGIN fill_IncomingTXFifo - channel %d\r\n", ch+1);
#endif /* _VERBOSE */

   union Ident_type
   {
	    unsigned char u8[4];
	    unsigned int  u32;
   }uIdent;

   volatile unsigned int *pDATA = (volatile unsigned int *)(0x50001010 + (ch * CHANNEL_DEDICATED_SIZE));
   volatile unsigned int *pDPRAM_CONFIG = (volatile unsigned int *)(0x50001000 + (ch * CHANNEL_DEDICATED_SIZE));

   *pDPRAM_CONFIG |= CTRL_CONFIG_TX;

   uIdent.u32 = (rx_m->id & 0x1FFFFFFF);

   if (!(rx_m->id & B31)) /* CAN A - BIT 31 is not set (0) - CAN B - BIT 31 is set (1) */
	   *pDATA = (unsigned int)(uIdent.u8[3] | START_FLAG | 0x2000);  // High Byte of Message Identifier and flag for CAN-A message.
   else
	   *pDATA = (unsigned int)(uIdent.u8[3] | START_FLAG);           // High Byte of Message Identifier

   *pDATA =  (unsigned int)uIdent.u8[2];
   *pDATA =  (unsigned int)uIdent.u8[1];
   *pDATA =  (unsigned int)uIdent.u8[0];  // Low Byte of Message Identifier

   //If no payload... set the flag to signify this is the end of the message.
   if (rx_m->buf_len == 0)
       *pDATA = (unsigned int)(rx_m->buf_len | END_FLAG);
   else
   {
       *pDATA = (unsigned int)rx_m->buf_len;

	    for (i=0;i<(rx_m->buf_len - 1);i++)
	        *pDATA = (unsigned int)rx_m->buf[i];

	    // Set the flag to signify this is the end of the message.
	    *pDATA = (unsigned int)(rx_m->buf[i] | END_FLAG);
   }

#ifdef _VERBOSE
   printf("END fill_IncomingTXFifo\r\n");
#endif /* _VERBOSE */

   return CAN_SUCCESS;
}


/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
canAB_module_round_trip_test is responsible for checking to see if the tx fifo, rx fifo and CAN interrupts are
all working from the module perspective. A set of pre-defined CAN messages will be sent out on the specified
channel (via the TX FIFO), routed through the CAN and the CAN interrupts will force the data into the receiving
channels RX FIFO. NOTE: test expects that Channel 1 is looped to Channel 2, Channel 3 is looped to Channel 4,
Channel 5 is looped to Channel 6 and Channel 7 is looped to Channel 8.
</summary>
<param name="ch"> : (Input) Channel index (zero based) to be reset.</param>
<returns>bool TRUE if success; FALSE otherwise</returns>
*/
/**************************************************************************************************************/
bool canAB_module_round_trip_test(CHANNEL ch, int nMsgCount)
{
	bool bSuccess = FALSE;
#ifdef _ALL_CHANNELS
	int i = 0;
	can_t tst_pkt;
	can_t RXtst_pkt;
	CHANNEL rxCH = CHANNEL_2;

#ifdef _VERBOSE
	int k = 0;
#endif /* _VERBOSE */

	switch (ch)
	{
	case CHANNEL_1 :
		rxCH = CHANNEL_2;
		break;
	case CHANNEL_2 :
		rxCH = CHANNEL_1;
		break;
	case CHANNEL_3 :
		rxCH = CHANNEL_4;
		break;
	case CHANNEL_4 :
		rxCH = CHANNEL_3;
		break;
	case CHANNEL_5 :
		rxCH = CHANNEL_6;
		break;
	case CHANNEL_6 :
		rxCH = CHANNEL_5;
		break;
	case CHANNEL_7 :
		rxCH = CHANNEL_8;
		break;
	case CHANNEL_8 :
		rxCH = CHANNEL_7;
		break;
	default :
		rxCH = CHANNEL_1;
		break;
	}

	tst_pkt.id = 0x727;
	tst_pkt.buf[0] = 0xA;
	tst_pkt.buf[1] = 0xC;
	tst_pkt.buf[2] = 0xE;
	tst_pkt.buf[3] = 0xD;
	tst_pkt.buf[4] = 0xA;
	tst_pkt.buf[5] = 0xC;
	tst_pkt.buf[6] = 0xE;
	tst_pkt.buf[7] = 0xD;
	tst_pkt.buf_len = 8;

	for (i=0; i < nMsgCount; i++)
	{
		memset(&RXtst_pkt, 0, sizeof(can_t));

		/* Here we fill in the incoming TX Fifo to simulate data coming across SERDES. */
		fill_IncomingTXFifo(&tst_pkt, ch);

		/* Now we mimmick our main loop by checking to see if there is data waiting to be transmitted for the specified channel.
		   The data in the TX Fifo should now be transmitted using the specified CAN channel. */
		check_for_tx(ch);

		wait_microsecond(1000);

#ifdef _DANT
#ifdef _ENABLE_INTERRUPTS
		can_intrHandler(ch);
		wait_microsecond(1000);
		can_intrHandler(rxCH);
		wait_microsecond(1000);
#endif /* _ENABLE_INTERRUPTS */
#endif /* _DANT */

		if (can_rx(rxCH, &RXtst_pkt) != 0)
		   printf("Error in can_module_round_trip_test():can_rx call\n");

#ifdef _VERBOSE
		//DEBUG PRINTOUT
		printf("Loop Index = %d\n", i);
		printf("CAN-A pkt.id = %x\n", (unsigned int)RXtst_pkt.id);
		for (k=0; k<8; k++)
		   printf("CAN-A pkt.buf[%d] = %x\n", k, RXtst_pkt.buf[k]);
		//END DEBUG
#endif /* _VERBOSE */

		if ((RXtst_pkt.id & 0x1FFFFFFF) == 0x727 &&
			RXtst_pkt.buf_len == 8 &&
			RXtst_pkt.buf[0] == 0xA &&
			RXtst_pkt.buf[1] == 0xC &&
			RXtst_pkt.buf[2] == 0xE &&
			RXtst_pkt.buf[3] == 0xD &&
			RXtst_pkt.buf[4] == 0xA &&
			RXtst_pkt.buf[5] == 0xC &&
			RXtst_pkt.buf[6] == 0xE &&
			RXtst_pkt.buf[7] == 0xD)
		  bSuccess = TRUE;
		else
		{
		  bSuccess = FALSE;
		  break;
		}
	}

/*NOW PUT received data on the RX FIFO (optional for this test)
	fill_RX_UIF(&RXtst_pkt, ch);
*/
#endif /* _ALL_CHANNELS */
	return bSuccess;
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
canAB_filter_test is responsible for enabling a filter on the specified channel such that only certain CAN
messages get passed through.
</summary>
<param name="ch"> : (Input) Channel index (zero based) to be reset.</param>
<returns>bool TRUE if success; FALSE otherwise</returns>
*/
/**************************************************************************************************************/
bool canAB_filter_test(CHANNEL ch)
{
	bool bSuccess = FALSE;
	int nBusyCount = 0;
	const int MAX_BUSY_RETRIES = 1000;
	can_t tst_pkt;
	uint8_t ucAcceptFilterBusy = 0;

	volatile unsigned int *pDPRAM_TX_FRAME_CNT = (volatile unsigned int *)(DPRAM_CH1_TX_FRAME_CNT + (ch * CHANNEL_DEDICATED_SIZE));

	uint8_t ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1 = 0;

	/* Get current baud rate..so we can re-initialize CAN with same when done */
	can_getBaudRatePrescaler(ch, &ucPrescaler);
	can_getBitTiming(ch, &ucSJW, &ucTimeSeg2, &ucTimeSeg1);

	/* First lets make sure we start with a cleared CAN channel */
	reset_channel(ch, ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1, TRUE);

	if (can_acceptFilterDisable(ch, CAN_AFR_UAF1_MASK) == CAN_SUCCESS)
	{
		do
		{
			if (can_isAcceptFilterBusy(ch, &ucAcceptFilterBusy) != CAN_SUCCESS)
				return FALSE;

			if (ucAcceptFilterBusy)
			{
				nBusyCount++;
				wait_microsecond(500);
				if (nBusyCount > MAX_BUSY_RETRIES)
					break;
			}
		} while (ucAcceptFilterBusy);

		if (nBusyCount <= MAX_BUSY_RETRIES)
		{
			uint32_t acceptFilterID = (0x727 << 21);
			if (can_acceptFilterSet(ch, CAN_AFR_UAF1_MASK, CAN_IDR_ID1_MASK, acceptFilterID) == CAN_SUCCESS)
			{
				if (can_acceptFilterEnable(ch, CAN_AFR_UAF1_MASK) == CAN_SUCCESS)
				{
					/* Now transmit one CAN-A message (should be accepted) and 1 CAN-B message (should be rejected) */
					tst_pkt.id = 0x727;
					tst_pkt.buf[0] = 0xA;
					tst_pkt.buf[1] = 0xC;
					tst_pkt.buf[2] = 0xE;
					tst_pkt.buf[3] = 0xD;
					tst_pkt.buf[4] = 0xA;
					tst_pkt.buf[5] = 0xC;
					tst_pkt.buf[6] = 0xE;
					tst_pkt.buf[7] = 0xD;
					tst_pkt.buf_len = 8;

					can_tx(ch+1, &tst_pkt);

					tst_pkt.id = 0x12345678;
					/* Identify CAN message as a 29-Bit CAN-B message */
					tst_pkt.id |= B31; /*CAN B*/
					tst_pkt.buf[0] = 0xF;
					tst_pkt.buf[1] = 0xE;
					tst_pkt.buf[2] = 0xD;
					tst_pkt.buf[3] = 0xC;
					tst_pkt.buf[4] = 0xB;
					tst_pkt.buf[5] = 0xA;
					tst_pkt.buf[6] = 0x9;
					tst_pkt.buf[7] = 0x8;
					tst_pkt.buf_len = 8;

					can_tx(ch+1, &tst_pkt);

					/* Only CAN-A message should be transmitted (Matches Filter) */
					if (*pDPRAM_TX_FRAME_CNT == 0)
						bSuccess = TRUE;
					else
						printf("TX Frame Count of %d did not match expected count of 0\n", *pDPRAM_TX_FRAME_CNT);
				}
			}
		}
	}

	return bSuccess;
}

bool can_set_baud_to_1Mb(CHANNEL ch)
{
	bool status = FALSE;
	unsigned short usBaudRate = 0xBA;
	unsigned short usBaudPrescaler = 0;

	if (can_enterMode(ch, CAN_MODE_CONFIG) == CAN_SUCCESS)
	{
	   can_setBaudRatePrescaler(ch, usBaudPrescaler);
	   can_setBitTiming(ch, ((usBaudRate >> CAN_BTR_SJW_SHIFT) & 0x03)/*SJW*/, ((usBaudRate >> CAN_BTR_TS2_SHIFT) & 0x07)/*TSeg2*/, (usBaudRate & 0x0F)/*TSeg1*/);
	   can_enterMode(ch, CAN_MODE_NORMAL);

	   uint8_t ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1 = 0;

	   /* Get current baud rate..so we can re-initialize CAN with same when done */
	   can_getBaudRatePrescaler(ch, &ucPrescaler);
	   can_getBitTiming(ch, &ucSJW, &ucTimeSeg2, &ucTimeSeg1);
	   printf("Requested Baud Prescaler = %d\r\n", ucPrescaler);
	   printf("Requested Baud SJW = %d\r\n", ucSJW);
	   printf("Requested Baud TSEG1 = %d\r\n", ucTimeSeg1);
	   printf("Requested Baud TSEG2 = %d\r\n", ucTimeSeg2);

	   status = TRUE;
	}
	return status;
}
#endif /*#if (MODULE_CB1 || MODULE_CB3) */

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
can_self_test is responsible for performing a simple CAN transmit and receive loopback test.
</summary>
<param name="ch"> : (Input) Channel index (zero based) to be reset.</param>
<returns>bool : true if success; false otherwise</returns>
*/
/**************************************************************************************************************/
bool can_self_test(CHANNEL ch)
{
	volatile unsigned int * pBIT_STATUS = (volatile unsigned int *)FPGA_BIT_STATUS;
	uint8_t ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1 = 0;

	/* Get current baud rate..so we can re-initialize CAN with same when done */
	can_getBaudRatePrescaler(ch, &ucPrescaler);
	can_getBitTiming(ch, &ucSJW, &ucTimeSeg2, &ucTimeSeg1);

	Slf_Test[ch] = TRUE;
	Slf_Test_passed[ch] = FALSE;

	if (can_selfTest(ch) == CAN_SUCCESS)
	{
		Slf_Test_passed[ch] = TRUE;
#ifdef _VERBOSE
		printf("BIT PASSED for channel %d\n",(ch+1));
#endif
	}
	else
		printf("BIT FAILED for channel %d\n",(ch+1));

	/* --- Post The Results --- */
	unsigned short bit_flag;
	bit_flag = 0x0001 << ch;

	if (Slf_Test_passed[ch] == TRUE)
	  *pBIT_STATUS &= ~bit_flag;
	else
	  *pBIT_STATUS |=  bit_flag;

	/* Seems like only way to re-enable Interrupts from here is to reset the channel...so let's do it */
	reset_channel(ch, ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1, TRUE);

	return (Slf_Test_passed[ch]);
}

/**************************************************************************************************************/
/**
\ingroup CAN
<summary>
can_reset_channel_test is responsible for resetting the specified channel. The TX and RX Fifo counts are
checked after the reset and if they do not reflect zero the test is considered to have FAILED.
</summary>
<returns>bool: true if success; false otherwise</returns>
*/
/**************************************************************************************************************/
bool can_reset_channel_test(CHANNEL ch)
{
	volatile unsigned int *pTX_SIZE = (volatile unsigned int *)(DPRAM_CH1_TX_USED  + (ch * CHANNEL_DEDICATED_SIZE));
	volatile unsigned int *pRX_SIZE = (volatile unsigned int *)(DPRAM_CH1_RX_USED  + (ch * CHANNEL_DEDICATED_SIZE));
#ifdef _VERBOSE
	printf("Channel %d TX SIZE = %x\r\n",ch+1, *pTX_SIZE);
	printf("Channel %d RX SIZE = %x\r\n",ch+1, *pRX_SIZE);
#endif /* _VERBOSE */

	uint8_t ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1 = 0;

	/* Get current baud rate..so we can re-initialize CAN with same when done */
	can_getBaudRatePrescaler(ch, &ucPrescaler);
	can_getBitTiming(ch, &ucSJW, &ucTimeSeg2, &ucTimeSeg1);

	reset_channel(ch, ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1, TRUE);

#ifdef _VERBOSE
	printf("AFTER RESET: Channel %d TX SIZE = %x\r\n",ch+1, *pTX_SIZE);
	printf("AFTER RESET: Channel %d RX SIZE = %x\r\n",ch+1, *pRX_SIZE);
#endif /* _VERBOSE */
	return (*pTX_SIZE == 0 && *pRX_SIZE == 0);
}


void can_force_interrupt_on_channel(CHANNEL ch)
{
	volatile unsigned int *pInterrupt = (volatile unsigned int *)FPGA_INTERRUPT_STATUS;

	*pInterrupt = 0;
	wait_microsecond(100000);
	*pInterrupt = (0x0001 << ch);
}

void can_dump_bit_timing(CHANNEL ch)
{
	CHANNEL startChannel = ch;
	CHANNEL endChannel = (ch + 1);
	CHANNEL currentChannel = ch;
	uint8_t ucPrescaler, ucSJW, ucTimeSeg2, ucTimeSeg1 = 0;

	if (ch == ALL_CHANNELS)
	{
		startChannel = CHANNEL_1;
		endChannel = NUM_CHANNELS;
	}

	printf("................................\n");
	for (currentChannel = startChannel; currentChannel < endChannel; currentChannel++)
	{
		/* Get current baud rate..so we can re-initialize CAN with same when done */
		can_getBaudRatePrescaler(currentChannel, &ucPrescaler);
		can_getBitTiming(currentChannel, &ucSJW, &ucTimeSeg2, &ucTimeSeg1);

		if (ch == ALL_CHANNELS && currentChannel > 0)
			printf("\n");

		printf("CHANNEL %d BIT TIMING\n", currentChannel+1);
		printf("BPR   = %d\n", ucPrescaler);
		printf("SJW   = %d\n", ucSJW);
		printf("TSEG1 = %d\n", ucTimeSeg1);
		printf("TSEG2 = %d\n", ucTimeSeg2);
	}
	printf("................................\n");
}

void can_dump_error_status(CHANNEL ch)
{
	volatile unsigned int *pDPRAM_LAST_ERR_CODE;
	volatile unsigned int *pDPRAM_CERC;
	volatile unsigned int *pDPRAM_BUS_STATUS;
	volatile unsigned int *pDPRAM_CORE_STATUS;
	volatile unsigned int *pDPRAM_COMM_STATUS;

	CHANNEL startChannel = ch;
	CHANNEL endChannel = (ch + 1);
	CHANNEL currentChannel = ch;

	if (ch == ALL_CHANNELS)
	{
		startChannel = CHANNEL_1;
		endChannel = NUM_CHANNELS;
	}

	printf("................................\n");
	for (currentChannel = startChannel; currentChannel < endChannel; currentChannel++)
	{
		pDPRAM_LAST_ERR_CODE = (volatile unsigned int *)(DPRAM_CH1_LAST_ERR_CODE + (currentChannel * CHANNEL_DEDICATED_SIZE));
		pDPRAM_CERC = (volatile unsigned int *)(DPRAM_CH1_CERC + (currentChannel * CHANNEL_DEDICATED_SIZE));
		pDPRAM_BUS_STATUS = (volatile unsigned int *)(DPRAM_CH1_BUS_STATUS + (currentChannel * CHANNEL_DEDICATED_SIZE));
		pDPRAM_CORE_STATUS = (volatile unsigned int *)(DPRAM_CH1_CORE_STATUS + (currentChannel * CHANNEL_DEDICATED_SIZE));
		pDPRAM_COMM_STATUS = (volatile unsigned int *)(DPRAM_CH1_COMM_STATUS + (currentChannel * CHANNEL_DEDICATED_SIZE));

		if (ch == ALL_CHANNELS && currentChannel > 0)
			printf("\n");

		printf("CHANNEL %d ERROR STATUS\n", currentChannel+1);
		printf("Last Error Code: 0x%x\n", *pDPRAM_LAST_ERR_CODE);
		printf("Error Counts - RX: 0x%x TX:0x%x\n", ((*pDPRAM_CERC & 0xFF00) >> 8), (*pDPRAM_CERC & 0x00FF));
		printf("Bus Status: 0x%x\n", *pDPRAM_BUS_STATUS);
		printf("Core Status: 0x%x\n", *pDPRAM_CORE_STATUS);
		printf("Comm Status: 0x%x\n", *pDPRAM_COMM_STATUS);
	}
	printf("................................\n");
}


void can_dump_filter_config(CHANNEL ch)
{
	uint32_t acceptFilterEnabled = 0;
	uint8_t enabledFilters[MAX_CHANNEL_FILTER_COUNT] = {0,0,0,0};
	uint32_t i = 0;
	uint32_t maskValue = 0;
	uint32_t idValue = 0;

	printf ("Dumping filter config for channel %d\n", ch+1);

	can_acceptFilterGetEnabled( ch, &acceptFilterEnabled );

	if ((acceptFilterEnabled & CAN_AFR_UAF_ALL_MASK) == CAN_AFR_UAF_ALL_MASK)
	{
		printf("All filters are enabled for channel %d\n", ch+1);
		enabledFilters[0] = 1;
		enabledFilters[1] = 1;
		enabledFilters[2] = 1;
		enabledFilters[3] = 1;
	}
	else
	{
		if ((acceptFilterEnabled & CAN_AFR_UAF1_MASK) == CAN_AFR_UAF1_MASK)
		{
			printf("Filter 1 is enabled for channel %d\n", ch+1);
			enabledFilters[0] = 1;
		}
		if ((acceptFilterEnabled & CAN_AFR_UAF2_MASK) == CAN_AFR_UAF2_MASK)
		{
			printf("Filter 2 is enabled for channel %d\n", ch+1);
			enabledFilters[1] = 1;
		}
		if ((acceptFilterEnabled & CAN_AFR_UAF3_MASK) == CAN_AFR_UAF3_MASK)
		{
			printf("Filter 3 is enabled for channel %d\n", ch+1);
			enabledFilters[2] = 1;
		}
		if ((acceptFilterEnabled & CAN_AFR_UAF4_MASK) == CAN_AFR_UAF4_MASK)
		{
			printf("Filter 4 is enabled for channel %d\n", ch+1);
			enabledFilters[3] = 1;
		}
	}

	for (i=1; i <= MAX_CHANNEL_FILTER_COUNT; i++)
	{
		/* Display info about enabled filters */
		if (enabledFilters[(i-1)] == 1)
		{
			can_acceptFilterGet(ch, CAN_AFR_MASK(i), &maskValue, &idValue);
			printf("Mask value for filter %u = 0x%x\n", (unsigned int)i, (unsigned int)maskValue);
			printf("ID value for filter %u = 0x%x\n", (unsigned int)i, (unsigned int)idValue);
		}
		else
			printf("Filter %u is not enabled!\n", (unsigned int)i);
	}
}


