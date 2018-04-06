/*
  Copyright Simma Software, Inc. - 2014

  ssCAN-AXI (Version 1.0)

  Use this software at your own risk.  Simma Software, Inc does not promise,
  state, or guarantee this software to be defect free.

  This file contains software that relates to the implementation of a
  CAN device driver for a Xilinx axi_can.
*/
#include <stdint.h>
#include "can.h"
#include "axi.h"
#include "bits.h"

/* receive and transmit buffer sizes */
/* PORT: adjust sizes per your application requirements */
#define CAN_PORTS          8

/* PORT: change these to match your memory map */
#define CAN1_BASE_ADDRESS  0x50000000
#define CAN2_BASE_ADDRESS  0x50010000
#define CAN3_BASE_ADDRESS  0x50020000
#define CAN4_BASE_ADDRESS  0x50030000
#define CAN5_BASE_ADDRESS  0x50040000
#define CAN6_BASE_ADDRESS  0x50050000
#define CAN7_BASE_ADDRESS  0x50060000
#define CAN8_BASE_ADDRESS  0x50070000

/* location in memory of PIC32 CAN peripherals */
static axi_t volatile *r[CAN_PORTS] = {
  (axi_t volatile *)CAN1_BASE_ADDRESS,
  (axi_t volatile *)CAN2_BASE_ADDRESS,
  (axi_t volatile *)CAN3_BASE_ADDRESS,
  (axi_t volatile *)CAN4_BASE_ADDRESS,
  (axi_t volatile *)CAN5_BASE_ADDRESS,
  (axi_t volatile *)CAN6_BASE_ADDRESS,
  (axi_t volatile *)CAN7_BASE_ADDRESS,
  (axi_t volatile *)CAN8_BASE_ADDRESS
};

/* arbitrary delay */
uint32_t volatile can_delay;


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_init is responsible for initializing the AXI for CAN operation.
</summary>
<param name="p"> : (Input) CAN port to be initialized. </param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 1 : (CAN_FAILED_TO_ENTER_MODE) Failed to enter configuration mode within allotted amount of time.
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_init ( uint8_t p  )
{
	return can_init_with_baud(p, CAN_BRP_500K, CAN_SJW_500K, CAN_TSEG2_500K, CAN_TSEG1_500K);
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_init_with_baud is responsible for initializing the AXI for CAN operation including specific baud rate into.
</summary>
<param name="p"> : (Input) CAN port to be initialized. </param>
<param name="prescaler"> : (Input) Prescaler to be used. </param>
<param name="syncJumpWidth"> : (Input) Synchronization Jump Width to be used. </param>
<param name="timeSegment2"> : (Input) Time Segment 2 to be used. </param>
<param name="timeSegment1"> : (Input) Time Segment 1 to be used. </param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 1 : (CAN_FAILED_TO_ENTER_MODE) Failed to enter configuration mode within allotted amount of time.
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_init_with_baud ( uint8_t p, uint8_t prescaler, uint8_t syncJumpWidth, uint8_t timeSegment2, uint8_t timeSegment1)
{
	uint8_t status = CAN_SUCCESS;
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* PORT: at this point all pins going to AXI should have been configured.
	 Once all pins are configured, do a hardware reset by driving
	 AXI_ARESET_N low, waiting TBD time, driving AXI_ARESET_N high.  */

	/* request configuration mode */
	r[p]->SRR = 0;

	/* wait for configuration mode, (CONFIG bit in SR) */
	/* PORT: after 1 millisecond of waiting, return failure (i.e. 1)
	 the below delay is an arbitrary delay that changes with compiler
	 settings and hardware speed */
	can_delay = 0;
	while( (r[p]->SR & B0) == 0 )
	if( can_delay++ > 0x3FFFF )
		return CAN_FAILED_TO_ENTER_MODE;

	/* set baud rate */
	r[p]->BRPR = prescaler;
	r[p]->BTR = ((uint32_t)syncJumpWidth << 7) | ((uint32_t)timeSegment2 << 4) | (uint32_t)timeSegment1;

	/* disable mask so we accept all messages */
	r[p]->AFR = 0;

	/* leave configuration mode and enter normal */
	r[p]->SRR = B1;

	/* wait for normal mode, (CONFIG bit in SR) */
	can_delay = 0;
	while( (r[p]->SR & B0) == B0 )
		if( can_delay++ > 0x3FFFF )
			status = CAN_FAILED_TO_ENTER_MODE;

	return status;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_rx is responsible for receiving a single CAN frame.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<param name="*frame"> : (Output) Pointer to a CAN frame containing the CAN message received on the given port. </param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 9  : (CAN_RX_ERROR) - An error was detected receiving a CAN frame on the given port.
	- 11 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_rx ( uint8_t p, can_t *frame )
{
	uint32_t volatile tmp[4];
	uint8_t status = CAN_SUCCESS;

	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	status = CAN_RX_ERROR;

	/* if the RXFIFO is not empty, read a frame */
	if( r[p]->ISR & B7 )
	{
		tmp[0] = r[p]->RXFIFO_ID;
		tmp[1] = r[p]->RXFIFO_DLC;
		tmp[2] = r[p]->RXFIFO_DATA1;
		tmp[3] = r[p]->RXFIFO_DATA2;

		/* check IDE to see if it is 11 or 29-bit */
		if( tmp[0] & B19 )
		{
			/* get 17..0, but shift by 1.  then use only those bits. */
			frame->id = (tmp[0] >> 1) & 0x3FFFF;

			/* get 28..18, mask only 28-18, shift by 3 so fits at top of id */
			frame->id |= ((tmp[0] & 0xFFE00000) >> 3); // ID[28..18]

			/* ssJ1939 expects ext frames to have B31 set */
			frame->id |= B31;
		}
		else
		{
			/* get 28..18, but shift by 1.  then use only those bits. */
			frame->id = tmp[0] >> 21 ;
		}

		/* get dlc */
		frame->buf_len = tmp[1] >> 28;

		/* get data */
		frame->buf[0] = tmp[2] >> 24;
		frame->buf[1] = tmp[2] >> 16;
		frame->buf[2] = tmp[2] >> 8;
		frame->buf[3] = tmp[2];

		frame->buf[4] = tmp[3] >> 24;
		frame->buf[5] = tmp[3] >> 16;
		frame->buf[6] = tmp[3] >> 8;
		frame->buf[7] = tmp[3];

#ifdef _NOT_CONVINCED_THIS_IS_NEEDED
//DANT TEST 7-22-2015 (I believe this was just a test...but I will leave this code commented out in case I come across reason why it was put here - ISR handler should clear interrupts.
		/*
		 * Clear RXNEMP bit in ISR. This allows future XCan_IsRxEmpty() call
		 * returns correct RX FIFO occupancy/empty condition.
		 */
		can_interruptClear(p, CAN_IXR_RXNEMP_MASK);
//END DANT
#endif
		status = CAN_SUCCESS;
	}

	return status;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_tx is responsible for sending a single CAN frame.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<param name="*frame"> : (Input) Pointer to a CAN frame containing the CAN message to send out on the
						 given port. </param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 3  : (CAN_TX_ERROR) - An error was detected sending a CAN frame on the given port.
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_tx ( uint8_t p, can_t *frame )
{
	uint32_t volatile tmp[4];
	uint8_t status = CAN_SUCCESS;

	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	status = CAN_TX_ERROR;

	/* if there is room in the FIFO, write the message */
	if( (r[p]->SR & B10) == 0 )
	{
		/* load id field, depending on standard or extended */
		if( frame->id & B31 )
		{
			tmp[0] = (frame->id & 0x3FFFF) << 1;
			tmp[0] |= ((frame->id << 3) & 0xFFE00000);
			tmp[0] |= B19;
		}
		else
		{
			tmp[0] = frame->id << 21;
		}

		/* move over data */
		tmp[1] = ((uint32_t)frame->buf_len) << 28;

		tmp[2] = frame->buf[0];
		tmp[2] <<= 8;
		tmp[2] |= frame->buf[1];
		tmp[2] <<= 8;
		tmp[2] |= frame->buf[2];
		tmp[2] <<= 8;
		tmp[2] |= frame->buf[3];

		tmp[3] = frame->buf[4];
		tmp[3] <<= 8;
		tmp[3] |= frame->buf[5];
		tmp[3] <<= 8;
		tmp[3] |= frame->buf[6];
		tmp[3] <<= 8;
		tmp[3] |= frame->buf[7];

		r[p]->TXFIFO_ID = tmp[0];
		r[p]->TXFIFO_DLC = tmp[1];
		r[p]->TXFIFO_DATA1 = tmp[2];
		r[p]->TXFIFO_DATA2 = tmp[3];

		status = CAN_SUCCESS;
	}

	return status;
}

/*DANT ADDED*/

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_checkPort is responsible for making sure the specified port is in range of the available CANs.
</summary>
<param name="p"> : (Input) CAN port </param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
*/
/**************************************************************************************************************/
uint8_t can_checkPort( uint8_t p )
{
	uint8_t status = CAN_SUCCESS;
	if (p >= CAN_PORTS)
		status = CAN_INVALID_PORT;
	return status;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_reset is responsible for forcing the CAN at the given port into configuration mode.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 1 : (CAN_FAILED_TO_ENTER_MODE) Failed to enter configuration mode within allotted amount of time.
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_reset( uint8_t p )
{
	uint8_t status = CAN_SUCCESS;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* request configuration mode */
	r[p]->SRR = CAN_SRR_SRST_MASK;

	/* wait for configuration mode, (CONFIG bit in SR) */
	/* PORT: after 1 millisecond of waiting, return failure (i.e. 1)
	 the below delay is an arbitrary delay that changes with compiler
	 settings and hardware speed */
	can_delay = 0;
	while( (r[p]->SR & B0) == 0 )
		if( can_delay++ > 0x3FFFF )
			status = CAN_FAILED_TO_ENTER_MODE;

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_selfTest is responsible for performing a Loopback test (basic sanity check) on the CAN at the given port.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 1 : (CAN_FAILED_TO_ENTER_MODE) Failed to enter configuration mode within allotted amount of time.
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
<seealso cref="can_reset">
<seealso cref="can_sendTestFrame">
<seealso cref="can_recvTestFrame">
*/
/**************************************************************************************************************/
uint8_t can_selfTest( uint8_t p )
{
	uint8_t status = CAN_SUCCESS;
	uint8_t origMode = can_getMode(p);

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Reset device first */
    status = can_reset(p);

    if (status == CAN_SUCCESS)
    {
//#ifdef _DANT_
//LEAVE BAUD RATE ALONE
    	/* Setup Baud Rate Prescaler Register (BRPR) and Bit Timing Register
    	   (BTR) such that CAN baud rate equals 40Kbps, given the CAN clock
    	   equal to 24MHz. */

    	/* set baud rate */
    	r[p]->BRPR = 29;
    	r[p]->BTR = (3 << 7) | (2 << 4) | 15;
//#endif

    	/* Enter loopback mode  */
    	r[p]->MSR |= B1;
    	r[p]->SRR = CAN_SRR_CEN_MASK;

    	can_delay = 0;
    	while (can_getMode(p) != CAN_MODE_LOOPBACK)
    		if( can_delay++ > 0x3FFFF )
    			status = CAN_FAILED_TO_ENTER_MODE;

    	if (status == CAN_SUCCESS)
    	{
			/* Send a test frame with known ID and Payload */
			status = can_sendTestFrame(p);

			if (status == CAN_SUCCESS)
			{
				/* Now receive and verify the test frame */
				status = can_recvTestFrame(p);
				if (status == CAN_SUCCESS)
				{
					/* Reset device again before return to the caller */
					status = can_reset(p);
				}
			}
    	}
    }

/*DANT ADDED*/
    /* Return to original mode of operation */
    can_enterMode(p, origMode);
/*END DANT ADDED*/

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_getMode is responsible for returning the current CAN mode for the CAN at the given port.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<returns>uint8_t : CAN Mode
	- 0x01 : Configuration Mode
	- 0x08 : Sleep Mode
	- 0x02 : Normal Mode
	- 0x04 : LoopBack Mode
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_getMode( uint8_t p )
{
	uint8_t mode = 0;
	uint8_t status = CAN_SUCCESS;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return mode;

	if (r[p]->SR & CAN_SR_CONFIG_MASK) 	/* Configuration Mode */
		mode = CAN_MODE_CONFIG;
	else if (r[p]->SR & CAN_SR_SLEEP_MASK)  	/* Sleep Mode */
		mode = CAN_MODE_SLEEP;
	else if (r[p]->SR & CAN_SR_NORMAL_MASK) 	/* Normal Mode */
		mode = CAN_MODE_NORMAL;
	else /* If this line is reached, the device is in Loop Back Mode. */
		mode = CAN_MODE_LOOPBACK;

	return mode;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_enterMode is responsible for putting the CAN found at the given port into the desired Mode.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<param name="operationMode"> : (Input) CAN Mode
	- 0x01 : Configuration Mode
	- 0x08 : Sleep Mode
	- 0x02 : Normal Mode
	- 0x04 : LoopBack Mode </param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
	- 6 : (CAN_INVALID_MODE) - Mode passed in was not recognized as a valid CAN mode.
	- 7 : (CAN_CONFIG_MODE_REQUIRED) - CAN is required to be in configuration mode in order to
	                                   change to the desired Mode.
</returns>
<seealso cref="can_checkPort">
<seealso cref="can_getMode">
*/
/**************************************************************************************************************/
uint8_t can_enterMode( uint8_t p, uint8_t operationMode )
{
	uint8_t currentMode;
	uint8_t status = CAN_SUCCESS;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Get current mode */
	if (operationMode != CAN_MODE_CONFIG &&
		operationMode != CAN_MODE_SLEEP &&
		operationMode != CAN_MODE_NORMAL &&
		operationMode != CAN_MODE_LOOPBACK)
		return CAN_INVALID_MODE;

	currentMode = can_getMode(p);

/*DANT ADDED*/
	/* If currentMode already is set to the desired operationMode..nothing to do! */
	if (currentMode == operationMode)
		return CAN_SUCCESS;
/*END DANT ADDED*/

	/* If current mode is Normal Mode and the mode to enter is Sleep Mode,
	   or if current mode is Sleep Mode and the mode to enter is Normal
	   Mode, no transition through Configuration Mode is needed. */
	if ((currentMode == CAN_MODE_NORMAL) &&
		(operationMode == CAN_MODE_SLEEP))
	{
		/* Normal Mode ---> Sleep Mode */
		r[p]->MSR = CAN_MSR_SLEEP_MASK;

		/* Mode transition is finished in this case and return to the caller */
		return CAN_SUCCESS;
	}

	else if ((currentMode == CAN_MODE_SLEEP) &&
		 (operationMode == CAN_MODE_NORMAL))
	{
		/* Sleep Mode ---> Normal Mode */
		r[p]->MSR = 0;

		/* Mode transition is finished in this case and return to the caller */
		return CAN_SUCCESS;
	}


	/* If the mode transition is not any of the two cases above, CAN must
	   enter Configuration Mode before switching into the target operation
	   mode. */
	r[p]->SRR = 0;

	/* Check if the device has entered Configuration Mode, if not, return to
	   the caller.*/
	if (can_getMode(p) != CAN_MODE_CONFIG)
		return CAN_CONFIG_MODE_REQUIRED;

	switch (operationMode)
	{
		case CAN_MODE_CONFIG:	/* Configuration Mode */
			/*
			 * As CAN is in Configuration Mode already.
			 * Nothing is needed to be done here
			 */
			break;

		case CAN_MODE_SLEEP:	/* Sleep Mode */
			/* Switch the device into Sleep Mode */
			r[p]->MSR = CAN_MSR_SLEEP_MASK;
			r[p]->SRR = CAN_SRR_CEN_MASK;
			break;

		case CAN_MODE_NORMAL:	/* Normal Mode */
			/* Switch the device into Normal Mode */
			r[p]->MSR = 0;
			r[p]->SRR = CAN_SRR_CEN_MASK;
			break;

		case CAN_MODE_LOOPBACK:	/* Loop back Mode */
			/* Switch the device into Loop back Mode */
			r[p]->MSR = CAN_MSR_LBACK_MASK;
			r[p]->SRR = CAN_SRR_CEN_MASK;
			break;
	}

	/* Wait until mode reflects the desired mode */
	can_delay = 0;
	while(can_getMode(p) != operationMode)
		if( can_delay++ > 0x3FFFF )
			status = CAN_FAILED_TO_ENTER_MODE;

	return CAN_SUCCESS;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_setBaudRatePrescaler is responsible for setting the Baud Rate Prescaler for the CAN
found at the given Port.
The system clock for the CAN controller is divided by (Prescaler + 1) to generate the quantum clock
needed for sampling and synchronization. Read the device specification for details.

Baud Rate Prescaler could be set only after CAN device entered Configuration Mode. So please call
XCan_EnterMode() to enter Configuration Mode before using this function.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<param name="prescaler"> : (Input) desired prescaler value</param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
	- 7 : (CAN_CONFIG_MODE_REQUIRED) - CAN is required to be in configuration mode in order to
	                                   change to the desired prescaler.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_setBaudRatePrescaler( uint8_t p, uint8_t prescaler )
{
	uint8_t status = CAN_SUCCESS;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Return error code if the device currently is NOT in Configuration Mode */
	if (can_getMode(p) != CAN_MODE_CONFIG)
		return CAN_CONFIG_MODE_REQUIRED;

	r[p]->BRPR = prescaler;

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_getBaudRatePrescaler is responsible for getting the Baud Rate Prescaler value.
The system clock for the CAN controller is divided by (Prescaler + 1) to generate the quantum
clock needed for sampling and synchronization. Read the device specification for details.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<param name="pBaudRatePrescaler"> : (Output) Baud Rate Prescaler value. The value range is from 0 to 255. </param>
<returns>uint8_t : 0 : Success; Non-Zero otherwise
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_getBaudRatePrescaler( uint8_t p, uint8_t *pBaudRatePrescaler )
{
	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	*pBaudRatePrescaler = (uint8_t)(r[p]->BRPR);

	return CAN_SUCCESS;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_setBitTiming is responsible for setting the Bit timing parameters for the CAN found at the specified
port.
Time segment 1, Time segment 2 and Synchronization Jump Width are set in this function. Device specification
requires the values passed into this function be one less than the actual values of these fields. Read the
device specification for details.

Bit time could be set only after CAN device entered Configuration Mode. Please call XCan_EnterMode() to
enter Configuration Mode before using this function.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<param name="syncJumpWidth"> : (Input) Synchronization Jump Width value. Valid values are from 0 to 3 </param>
<param name="timeSegment2"> : (Input) Time Segment 2 value. Valid values are from 0 to 7.</param>
<param name="timeSegment1"> : (Input) Time Segment 1 value. Valid values are from 0 to 15.</param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
	- 8  : (CAN_INVALID_BIT_TIMING_PARAM) - One of the parameters were outside the permissible range.
	- 7  : (CAN_CONFIG_MODE_REQUIRED) - CAN is required to be in configuration mode in order to
	                                   change to the desired prescaler.
</returns>
<seealso cref="can_checkPort">
<seealso cref="can_getMode">
*/
/**************************************************************************************************************/
uint8_t can_setBitTiming( uint8_t p, uint8_t syncJumpWidth, uint8_t timeSegment2, uint8_t timeSegment1 )
{
	uint32_t value;
	uint8_t status = CAN_SUCCESS;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	if (syncJumpWidth > 3 || timeSegment2 > 7 || timeSegment1 > 15)
		return CAN_INVALID_BIT_TIMING_PARAM;

	/* Return error code if the device is NOT in Configuration Mode */
	if (can_getMode(p) != CAN_MODE_CONFIG)
		return CAN_CONFIG_MODE_REQUIRED;

	value = ((uint32_t) timeSegment1) & CAN_BTR_TS1_MASK;
	value |= (((uint32_t) timeSegment2) << CAN_BTR_TS2_SHIFT) & CAN_BTR_TS2_MASK;
	value |= (((uint32_t) syncJumpWidth) << CAN_BTR_SJW_SHIFT) & CAN_BTR_SJW_MASK;
	r[p]->BTR = value;

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_getBitTiming is responsible for getting the Bit time. Time segment 1, Time segment 2 and
Synchronization Jump Width values are read in this function. According to device specification,
the actual value of each of these fields is one more than the value read. Read the device
specification for details.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<param name="pSyncJumpWidth"> : (Output) Synchronization Jump Width value. Valid values are from 0 to 3 </param>
<param name="pTimeSegment2"> : (Output) Time Segment 2 value. Valid values are from 0 to 7.</param>
<param name="pTimeSegment1"> : (Output) Time Segment 1 value. Valid values are from 0 to 15.</param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_getBitTiming( uint8_t p, uint8_t *pSyncJumpWidth,
			   	   	  	  uint8_t *pTimeSegment2, uint8_t *pTimeSegment1 )
{
	uint32_t value;
	uint32_t status = CAN_SUCCESS;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
	{
		*pSyncJumpWidth = 0;
		*pTimeSegment2 = 0;
		*pTimeSegment1 = 0;
		return status;
	}

	value = r[p]->BTR;

	*pTimeSegment1  = (uint8_t)(value & CAN_BTR_TS1_MASK);
	*pTimeSegment2  = (uint8_t)((value & CAN_BTR_TS2_MASK) >> CAN_BTR_TS2_SHIFT);
	*pSyncJumpWidth = (uint8_t)((value & CAN_BTR_SJW_MASK) >> CAN_BTR_SJW_SHIFT);

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_sendTestFrame is responsible for sending a predefined CAN msg from the CAN at the specified port.
This function waits until TX FIFO has room for at least one frame before sending a frame. So this function
may block if the hardware is not built correctly.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame. </param>
<returns>uint8_t : Status
	- 0 : SUCCESS
	- 3 : (CAN_TX_ERROR) - An error was detected sending a CAN frame on the given port.
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
<seealso cref="can_isTxFifoFull">
<seealso cref="can_tx">
*/
/**************************************************************************************************************/
uint8_t can_sendTestFrame( uint8_t p )
{
	uint8_t status = CAN_SUCCESS;
	can_t tst_pkt;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Create a test packet */
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

	/* Wait until TX FIFO has room */
	can_delay = 0;
	while (can_isTxFifoFull(p) == TRUE)
		if( can_delay++ > 0x3FFFF )
			status = CAN_NO_FIFO_ROOM;

	/* Send Message */
	if (status == CAN_SUCCESS)
		status = can_tx(p, &tst_pkt);

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_recvTestFrame is responsible for receiving a predefined CAN msg from the CAN at the specified port.
This function waits until RX FIFO becomes not empty before reading a frame from it. So this function may
block if the hardware is not built correctly.
</summary>
<param name="p"> : (Input) CAN port from which to receive a single CAN frame.</param>
<returns>uint8_t : Status
	- 0 : SUCCESS
	- 3 : (CAN_RX_ERROR) - An error was detected receiving a CAN frame on the given port.
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
<seealso cref="can_isRxEmpty">
<seealso cref="can_rx">
*/
/**************************************************************************************************************/
uint8_t can_recvTestFrame( uint8_t p )
{
	uint8_t status = CAN_SUCCESS;
	can_t RXtst_pkt;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	can_delay = 0;
	/* Wait until a frame is received. */
	while (can_isRxEmpty(p) == TRUE)
	if( can_delay++ > 0x3FFFF )
		return CAN_NO_DATA_TO_RX;

	/* Receive a frame and verify its contents. */
	status = can_rx(p, &RXtst_pkt);
	if (status != CAN_SUCCESS)
		return status;

	/* Validate test packet */
	status = CAN_LOOPBACK_TEST_ERROR;
	if ((RXtst_pkt.id & 0x1FFFFFFF) == 0x123)
	{
		if (RXtst_pkt.buf_len == 8)
		{
			if ((RXtst_pkt.buf[0] == 1) &&
				(RXtst_pkt.buf[1] == 2) &&
				(RXtst_pkt.buf[2] == 3) &&
				(RXtst_pkt.buf[3] == 4) &&
				(RXtst_pkt.buf[4] == 5) &&
				(RXtst_pkt.buf[5] == 6) &&
				(RXtst_pkt.buf[6] == 7) &&
				(RXtst_pkt.buf[7] == 8))
				status = CAN_SUCCESS;
		}
	}

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_isTxFifoFull is responsible for determining whether or not the Fifo for the given CAN port is full.
</summary>
<param name="p"> : (Input) CAN port.</param>
<returns>uint8_t :
	- 1 : TX Fifo is Full
	- 0 : TX Fifo is Not Full
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_isTxFifoFull( uint8_t p )
{
	uint8_t status = CAN_SUCCESS;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return 0;

	return ((r[p]->SR & CAN_SR_TXFLL_MASK) == CAN_SR_TXFLL_MASK);
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_isRxEmpty is responsible for determining whether or not the RX for the given CAN port is empty.
</summary>
<param name="p"> : (Input) CAN port.</param>
<returns>uint8_t :
	- 1 : RX Fifo is Empty
	- 0 : RX Fifo is Not Empty
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_isRxEmpty( uint8_t p )
{
	uint8_t status = CAN_SUCCESS;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return 0;

	return (((r[p]->ISR & CAN_IXR_RXNEMP_MASK) == 0)?1:0);
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_isRxEmpty is responsible for determining whether or not the RX for the given CAN port is empty.
</summary>
<param name="p"> : (Input) CAN port.</param>
<returns>uint8_t :
	- 1 : RX Fifo is Empty
	- 0 : RX Fifo is Not Empty
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_isRxMsgReady( uint8_t p )
{
	uint8_t status = CAN_SUCCESS;

	/* Make sure port is within range */
	status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return 0;

	return (((r[p]->ISR & CAN_IXR_RXOK_MASK) == CAN_IXR_RXOK_MASK)?1:0);
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_isAcceptFilterBusy is responsible for determining whether or not the acceptance filter is busy.
</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="pAcceptFilterBusy"> : (Output)
	- 1 : Acceptance Filter is Busy
	- 0 : Acceptance Filter is Not Busy
</param>
<returns>uint8_t : 0 :Success; Non-Zero otherwise
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_isAcceptFilterBusy( uint8_t p, uint8_t *pAcceptFilterBusy )
{
	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	*pAcceptFilterBusy = ((r[p]->SR & CAN_SR_ACFBSY_MASK) == CAN_SR_ACFBSY_MASK);

	return CAN_SUCCESS;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_getCoreStatusRegisterContents is responsible for returning the core status register contents.
</summary>
<param name="p"> : (Input) CAN port.</param>
<returns>uint8_t :
	- 11 : Invalid Port
	- 0  : Success
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_getCoreStatusRegisterContents( uint8_t p, uint32_t *pSRRegValue )
{
	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	*pSRRegValue = r[p]->SR;

	return CAN_SUCCESS;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_getBusErrorCounter is responsible for retrieving both the Rx and Tx error counts for the CAN at the
specified port.
</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="pRxErrorCount"> : (Output)pRxErrorCount will contain Receive Error Counter value after this
function returns</param>
<param name="pTxErrorCount"> : (Output)pTxErrorCount will contain Transmit Error Counter value after this
function returns</param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 4  : (CAN_INVALID_PORT) - The specified port is not within range.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_getBusErrorCounter( uint8_t p, uint8_t *pRxErrorCount, uint8_t *pTxErrorCount )
{
	uint32_t result;

	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Read Error Counter Register and parse it. */
	result = r[p]->ECR;

	*pRxErrorCount = (result & CAN_ECR_REC_MASK) >> CAN_ECR_REC_SHIFT;
	*pTxErrorCount = result & CAN_ECR_TEC_MASK;

	return status;
}

uint8_t can_getBusErrorCounterCombined( uint8_t p, uint32_t *pRxTxErrorCount )
{
	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Read Error Counter Register and parse it. */
	*pRxTxErrorCount = r[p]->ECR;

	return status;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_getBusErrorStatus is responsible for reading Error Status value from Error Status Register (ESR). Use
the CAN_ESR_* constants defined in can.h to interpret the returned value.
</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="pBusErrorStatus"> : (Output) Contents of ESR will be copied into this output parameter.</param>
<returns>uint8_t: 0 if Success else non-zero
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_getBusErrorStatus( uint8_t p, uint32_t *pBusErrorStatus )
{
	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	*pBusErrorStatus = r[p]->ESR;

	return CAN_SUCCESS;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_clearBusErrorStatus is responsible for clearing the Error Status bit(s) previously set in Error
Status Register (ESR). Use the CAN_ESR_* constants defined in can.h to create the value to pass in.
If a bit was cleared in Error Status Register before this function is called, it will not be touched.
</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="mask"> : (Input) Mask is he 32-bit mask used to clear bits in Error Status Register.
Multiple CAN_ESR_* values could be 'OR'ed to clear multiple bits.</param>
<returns>uint8_t : Status
	- 0 : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_clearBusErrorStatus( uint8_t p, uint32_t mask )
{
	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;
/* DANT CHANGED TO ESR AS PER XILINX DOCUMENTATION - WRITING A 1 CLEARS THE STATUS
	r[p]->MSR = mask;
*/
	r[p]->ESR = mask;
	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_acceptFilterEnable is responsible for enabling individual acceptance filters. Up to 4 filters could
be enabled.

NOTE: Acceptance Filter Register is an optional register in Xilinx CAN device.  If it is NOT existing in
the device, this function should NOT be used. Calling this function in this case will cause an assertion failure.

</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="filterIndexes"> : (Input) filterIndexes specifies which filter(s) to enable. Use any
CAN_AFR_UAF*_MASK to enable one filter, and "Or" multiple CAN_AFR_UAF*_MASK values if multiple filters
need to be enabled. Any filter not specified in this parameter will keep its previous enable/disable setting.</param>
<returns>uint8_t : Status
	- 0 : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_acceptFilterEnable( uint8_t p, uint32_t filterIndexes)
{
	uint32_t enabledFilters;

	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Read the currently enabled filters from Acceptance Filter Register(AFR),
	   which defines which filters are enabled/disabled. */
	enabledFilters = r[p]->AFR;

	/* Calculate new value to write to AFR */
	enabledFilters |= filterIndexes;
	enabledFilters &= CAN_AFR_UAF_ALL_MASK;

	/* Write AFR */
	r[p]->AFR = enabledFilters;

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_acceptFilterDisable is responsible for disabling individual acceptance filters. Up to 4 filters could
be disabled. If all acceptance filters are disabled then all received frames are stored in the RX FIFO.

NOTE: Acceptance Filter Register is an optional register in Xilinx CAN device.  If it is NOT existing in
the device, this function should NOT be used. Calling this function in this case will cause an assertion failure.

</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="filterIndexes"> : (Input) filterIndexes specifies which filter(s) to disable. Use any
CAN_AFR_UAF*_MASK to disable one filter, and "Or" multiple CAN_AFR_UAF*_MASK values if multiple filters
need to be disabled. Any filter not specified in this parameter will keep its previous enable/disable setting.
If all acceptance filters are disabled then all received frames are stored in the RX FIFO.</param>
<returns>uint8_t : Status
	- 0 : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_acceptFilterDisable( uint8_t p, uint32_t filterIndexes )
{
	uint32_t enabledFilters;

	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Read the currently enabled filters from Acceptance Filter Register(AFR),
	   which defines which filters are enabled/disabled. */
	enabledFilters = r[p]->AFR;

	/* Calculate new value to write to AFR */
	enabledFilters &= CAN_AFR_UAF_ALL_MASK & (~filterIndexes);

	/* Write AFR */
	r[p]->AFR = enabledFilters;

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_acceptFilterGetEnabled is responsible for returning the enabled acceptance filters.
Use CAN_AFR_UAF*_MASK defined in xcan.h to interpret the returned value. If no acceptance filters
are enabled then all received frames are stored in the RX FIFO.

NOTE: Acceptance Filter Register is an optional register in Xilinx CAN device.  If it is NOT existing in
the device, this function should NOT be used. Calling this function in this case will cause an assertion failure.

</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="pAcceptFilterEnabled"> : (Output) Value stored in the enabled acceptance filter register.</param>
<returns>uint8_t : 0 if Success; non-zero otherwise
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_acceptFilterGetEnabled( uint8_t p, uint32_t *pAcceptFilterEnabled )
{
	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	*pAcceptFilterEnabled = (r[p]->AFR);

	return CAN_SUCCESS;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_acceptFilterSet is responsible for setting values to the Acceptance Filter Mask Register (AFMR) and
Acceptance Filter ID Register (AFIR) for the specified Acceptance Filter.
Use CAN_IDR_* defined in can.h to create the values to set the filter.
Read can.h and device specification for details. Use CAN_AFR_UAF*_MASK defined in can.h to interpret the
returned value. If no acceptance filters are enabled then all received frames are stored in the RX FIFO.

This function should be called only after:
   - The given filter is disabled by calling can_acceptFilterDisable();
   - And the CAN device is ready to accept writes to AFMR and AFIR, i.e.,
     can_IsAcceptFilterBusy() returns FALSE.

NOTE: Acceptance Filter Mask and ID Registers are optional registers in Xilinx CAN device. If they are
NOT existing in the device, this function should NOT be used. Calling this function in this case
will cause an assertion failure.

</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="filterIndex"> : filterIndex defines which Acceptance Filter Mask and ID Register to set.
Use any single CAN_AFR_UAF*_MASK value.</param>
<param name="maskValue"> : maskValue is the value to write to the chosen Acceptance Filter Mask Register.</param>
<param name="idValue"> : idValue is the value to write to the chosen Acceptance Filter ID Register.</param>
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
	- 9  : (CAN_INVALID_FILTER_INDEX) - Filter Index passed in was evaluated to be invalid.
	- 10 : (CAN_FILTER_ALREADY_ENABLED) - Filter is already enabled.
	- 11 : (CAN_ACCEPTANCE_FILTER_BUSY) - Acceptance filter is busy.
</returns>
<seealso cref="can_checkPort">
<seealso cref="can_acceptFilterGetEnabled">
<seealso cref="can_isAcceptFilterBusy">
*/
/**************************************************************************************************************/
uint8_t can_acceptFilterSet( uint8_t p, uint32_t filterIndex, uint32_t maskValue, uint32_t idValue )
{
	uint32_t enabledFilters  = 0;
	uint8_t acceptFilterBusy = 0;

	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Make sure a valid accept filter index was provided */
	if (filterIndex != CAN_AFR_UAF4_MASK &&
		filterIndex != CAN_AFR_UAF3_MASK &&
		filterIndex != CAN_AFR_UAF2_MASK &&
		filterIndex != CAN_AFR_UAF1_MASK)
		return CAN_INVALID_FILTER_INDEX;

	/* Check if the given filter is currently enabled.
	   If yes, return error code.*/
	can_acceptFilterGetEnabled(p, &enabledFilters);
	if ((enabledFilters & filterIndex) == filterIndex)
		return CAN_FILTER_ALREADY_ENABLED;

	/* If the CAN device is not ready to accept writes to AFMR and AFIR,
	   return error code.*/
	can_isAcceptFilterBusy(p, &acceptFilterBusy);
	if (acceptFilterBusy == TRUE)
		return CAN_ACCEPTANCE_FILTER_BUSY;

	/* Write AFMR and AFIR of the given filter */
	switch (filterIndex)
	{
		case CAN_AFR_UAF1_MASK:	/* Acceptance Filter No. 1 */
			/* Write Mask Register */
			r[p]->AFMR1 = maskValue;

			/* Write ID Register */
			r[p]->AFIR1 = idValue;
			break;
		case CAN_AFR_UAF2_MASK:	/* Acceptance Filter No. 2 */
			/* Write Mask Register */
			r[p]->AFMR2 = maskValue;

			/* Write ID Register */
			r[p]->AFIR2 = idValue;
			break;
		case CAN_AFR_UAF3_MASK:	/* Acceptance Filter No. 3 */
			/* Write Mask Register */
			r[p]->AFMR3 = maskValue;

			/* Write ID Register */
			r[p]->AFIR3 = idValue;
			break;
		case CAN_AFR_UAF4_MASK:	/* Acceptance Filter No. 4 */
			/* Write Mask Register */
			r[p]->AFMR4 = maskValue;

			/* Write ID Register */
			r[p]->AFIR4 = idValue;
			break;
	}

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_acceptFilterGet is responsible for reading the values of the Acceptance Filter Mask
and ID Register for the specified Acceptance Filter. Use CAN_IDR_* defined in can.h to
interpret the values. Read can.h and device specification for details.

NOTE: Acceptance Filter Mask and ID Registers are optional registers in Xilinx CAN device. If they are
NOT existing in the device, this function should NOT be used. Calling this function in this case
will cause an assertion failure.
</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="filterIndex"> : (Input) filterIndex defines which Acceptance Filter Mask Register to get
Mask and ID from. Use any single XCAN_FILTER_* value.</param>
<param name="pMaskValue"> : (Output) pMaskValue will store the Mask value read from the chosen Acceptance Filter
Mask Register after this function returns.</param>
<param name="pIdValue"> : (Output) pIdValue will store the ID value read from the chosen Acceptance Filter ID
Register after this function returns.
<returns>uint8_t : Status
	- 0  : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
	- 9  : (CAN_INVALID_FILTER_INDEX) - Filter Index passed in was evaluated to be invalid.
</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_acceptFilterGet( uint8_t p, uint32_t filterIndex,
		   	  	  	  	     uint32_t *pMaskValue, uint32_t *pIdValue )
{
	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;


	if (filterIndex != CAN_AFR_UAF4_MASK &&
		filterIndex != CAN_AFR_UAF3_MASK &&
		filterIndex != CAN_AFR_UAF2_MASK &&
		filterIndex != CAN_AFR_UAF1_MASK)
		return CAN_INVALID_FILTER_INDEX;

	switch (filterIndex)
	{
		case CAN_AFR_UAF1_MASK:	/* Acceptance Filter No. 1 */
			/* Read Mask Register */
			*pMaskValue = r[p]->AFMR1;
			/* Read ID Register */
			*pIdValue = r[p]->AFIR1;
			break;
		case CAN_AFR_UAF2_MASK:	/* Acceptance Filter No. 2 */
			/* Read Mask Register */
			*pMaskValue = r[p]->AFMR2;
			/* Read ID Register */
			*pIdValue = r[p]->AFIR2;
			break;
		case CAN_AFR_UAF3_MASK:	/* Acceptance Filter No. 3 */
			/* Read Mask Register */
			*pMaskValue = r[p]->AFMR3;
			/* Read ID Register */
			*pIdValue = r[p]->AFIR3;
			break;
		case CAN_AFR_UAF4_MASK:	/* Acceptance Filter No. 4 */
			/* Read Mask Register */
			*pMaskValue = r[p]->AFMR4;
			/* Read ID Register */
			*pIdValue = r[p]->AFIR4;
			break;
	}

	return status;
}


/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_interruptEnable is responsible for enabling interrupt(s). Use the CAN_IXR_* constants defined in
can.h to create the bit-mask to enable interrupts.
</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="Mask"> : (Input) Interrupt Mask to set. Bit positions of 1 will be enabled.
Bit positions of 0 will keep the previous setting. This mask is formed by OR'ing XCAN_IXR_*
bits defined in can.h.</param>
<returns>uint8_t : Status
	- 0 : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
<seealso cref="can_interruptGetEnabled">
*/
/**************************************************************************************************************/
uint8_t can_interruptEnable(uint8_t p, uint32_t Mask)
{
	uint32_t IntrValue;

	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Read currently enabled interrupts. */
	status = can_interruptGetEnabled(p, &IntrValue);
	if (status != CAN_SUCCESS)
		return status;

	/* Calculate the new interrupts that should be enabled */
	IntrValue |= Mask & CAN_IXR_ALL;

	/* Write to IER to enable interrupts */
	r[p]->IER = IntrValue;

	return status;
}

uint8_t can_interruptEnableAll(uint32_t Mask)
{
	uint8_t p;
	uint8_t status = CAN_SUCCESS;

	for (p=0; p < CAN_PORTS; p++)
	{
		status = can_interruptEnable(p, Mask);
		if (status != CAN_SUCCESS)
			break;
	}

	return status;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_interruptDisable is responsible for disabling interrupt(s). Use the CAN_IXR_* constants defined in
can.h to create the bit-mask to disable interrupt(s).
</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="Mask"> : (Input) Mask is the mask to disable. Bit positions of 1 will be disabled. Bit
positions of 0 will keep the previous setting. This mask is formed by OR'ing XCAN_IXR_* bits
defined in can.h.</param>
<returns>uint8_t : Status
	- 0 : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
<seealso cref="can_interruptGetEnabled">
*/
/**************************************************************************************************************/
uint8_t can_interruptDisable(uint8_t p, uint32_t Mask)
{
	uint32_t IntrValue;

	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Read currently enabled interrupts. */
	status = can_interruptGetEnabled(p, &IntrValue);
	if (status != CAN_SUCCESS)
		return status;

	/* Calculate the new interrupts that should be kept enabled */
	IntrValue &= ~Mask;

	/* Write to IER to enable interrupts */
	r[p]->IER = IntrValue;

	return status;
}

uint8_t can_interruptDisableAll(uint32_t Mask)
{
	uint8_t p;
	uint8_t status = CAN_SUCCESS;

	for (p=0; p < CAN_PORTS; p++)
	{
		status = can_interruptDisable(p, Mask);
		if (status != CAN_SUCCESS)
			break;
	}

	return status;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_interruptGetEnabled is responsible for enabling interrupt(s). Use the CAN_IXR_* constants
defined in can_l.h to interpret the returned value.
</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="pInterruptEnabled"> : (Output) Enabled interrupt(s) in a 32-bit format.</param>
<returns>0 if Success; non-zero otherwise.</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_interruptGetEnabled(uint8_t p, uint32_t *pInterruptEnabled)
{
	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	*pInterruptEnabled = r[p]->IER;

	return CAN_SUCCESS;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_interruptGetStatus is responsible for returning the interrupt status.
Use the CAN_IXR_* constants defined in can.h to interpret the returned value.
</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="pInterruptStatus"> : (Output)The value stored in Interrupt Status Register.</param>
<returns>0 if Success; non-zero otherwise.</returns>
<seealso cref="can_checkPort">
*/
/**************************************************************************************************************/
uint8_t can_interruptGetStatus(uint8_t p, uint32_t *pInterruptStatus)
{
	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	*pInterruptStatus = r[p]->ISR;

	return CAN_SUCCESS;
}

/**************************************************************************************************************/
/**
\ingroup CAN_API
<summary>
can_interruptClear is responsible for clearing interrupt(s). Every bit set in Interrupt
Status Register indicates that a specific type of interrupt is occurring, and this
function clears one or more interrupts by writing a bit mask to Interrupt Clear Register.
</summary>
<param name="p"> : (Input) CAN port.</param>
<param name="Mask"> : (Input) Mask is the mask to clear. Bit positions of 1 will be cleared.
Bit positions of 0 will not change the previous interrupt status. This mask is formed by
OR'ing CAN_IXR_* bits defined in can.h..</param>
<returns>uint8_t : Status
	- 0 : SUCCESS
	- 4 : (CAN_INVALID_PORT) Specified port is not within range of available CANs.
</returns>
<seealso cref="can_checkPort">
<seealso cref="can_interruptGetStatus">
*/
/**************************************************************************************************************/
uint8_t can_interruptClear(uint8_t p, uint32_t Mask)
{
	uint32_t IntrValue;

	/* Make sure port is within range */
	uint8_t status = can_checkPort(p);
	if (status != CAN_SUCCESS)
		return status;

	/* Read currently pending interrupts. */
	status = can_interruptGetStatus(p, &IntrValue);
	if (status != CAN_SUCCESS)
		return status;

	/* Calculate the interrupts that should be cleared. */
	IntrValue &= Mask;

	/* Write to ICR to clear interrupts */
	r[p]->ICR = IntrValue;

	return status;
}

uint8_t can_interruptClearAll(uint32_t Mask)
{
	uint8_t p;
	uint8_t status = CAN_SUCCESS;

	for (p=0; p < CAN_PORTS; p++)
	{
		status = can_interruptClear(p, Mask);
		if (status != CAN_SUCCESS)
			break;
	}

	return status;
}

/*END DANT ADDED*/
