#ifndef CAN_H
#define CAN_H

/*DANT ADDED*/
#ifndef TRUE
#  define TRUE		1
#endif

#ifndef FALSE
#  define FALSE		0
#endif

/** CAN Last Error Status Types **/
#define CAN_BUS_STATUS_TYPE				0x00000001
#define CAN_COMM_STATUS_TYPE			0x00000002

/** NAI Error Codes **/
#define CAN_NAI_FIFO_EMPTY				0x0001
#define CAN_NAI_FIFO_NOT_ENOUGH_DATA	0x0002
#define CAN_NAI_FIFO_NO_START_FLAG		0x0003
#define CAN_NAI_TX_DATA_COUNT_NULL		0x0004
#define CAN_NAI_FIFO_END_FLAG_NOT_FOUND 0x0005  /* Either not found or not found where expected */
#define CAN_NAI_BAD_SIZE				0x0006
#define CAN_NAI_NOT_IN_CONFIG_MODE		0x0007
#define CAN_NAI_TX_FAILURE				0x0008

/** Error Messages **/
#define CAN_SUCCESS  					0
#define CAN_NO_DATA_TO_TX	 			1
#define CAN_TX_MIN_FRAME_VIOLATION 		2
#define CAN_TX_START_NOT_FOUND			3
#define CAN_TX_NULL_DATA_BUFF			4
#define CAN_TX_END_NOT_FOUND			5
#define CAN_BAD_DATA_SIZE				6
#define CAN_IRQ_REGISTER_ERROR      	7
#define CAN_FAILED_TO_ENTER_MODE 		8
#define CAN_RX_ERROR					9
#define CAN_TX_ERROR				   10
#define CAN_INVALID_PORT			   11
#define CAN_LOOPBACK_TEST_ERROR		   12
#define CAN_INVALID_MODE 			   13
#define CAN_CONFIG_MODE_REQUIRED 	   14
#define CAN_INVALID_BIT_TIMING_PARAM   15
#define CAN_INVALID_FILTER_INDEX	   16
#define CAN_FILTER_ALREADY_ENABLED	   17
#define CAN_ACCEPTANCE_FILTER_BUSY	   18
#define CAN_NO_FIFO_ROOM	   	   	   20
#define CAN_NO_DATA_TO_RX			   21
#define CAN_UNEXPECTED_FORMAT		   22
#define CAN_MSG_UNINTENDED_RECIPIENT   23


/** CAN operation modes */
#define CAN_MODE_CONFIG		0x00000001 /* Configuration mode */
#define CAN_MODE_NORMAL		0x00000002 /* Normal mode */
#define CAN_MODE_LOOPBACK	0x00000004 /* Loop Back mode */
#define CAN_MODE_SLEEP		0x00000008 /* Sleep mode */

#define CAN_IXR_RXNEMP_MASK	0x00000080  /* RX FIFO Not Empty Intr Mask */

#define CAN_SRR_CEN_MASK	0x00000002  /* CAN Enable Mask */
#define CAN_SRR_SRST_MASK	0x00000001  /* Reset Mask */

#define CAN_MSR_LBACK_MASK	0x00000002  /* Loop Back Mode Select Mask */
#define CAN_MSR_SLEEP_MASK	0x00000001  /* Sleep Mode Select Mask */

/* Bit Timing Register */
#define CAN_BTR_SJW_MASK	0x00000180  /* Sync Jump Width Mask */
#define CAN_BTR_SJW_SHIFT	7	    	/* Sync Jump Width Shift */
#define CAN_BTR_TS2_MASK	0x00000070  /* Time Segment 2 Mask */
#define CAN_BTR_TS2_SHIFT	4	    	/* Time Segment 2 Shift */
#define CAN_BTR_TS1_MASK	0x0000000F  /* Time Segment 1 Mask */

/* Status Register */
#define CAN_SR_ACFBSY_MASK	0x00000800  /* Acceptance Filter busy Mask */
#define CAN_SR_TXFLL_MASK	0x00000400  /* TX FIFO is full Mask */
#define CAN_SR_TXBFLL_MASK	0x00000200  /* TX High Priority Buffer full */
#define CAN_SR_ESTAT_MASK	0x00000180  /* Error Status Mask */
#define CAN_SR_ESTAT_SHIFT	7	    	/* Error Status Shift */
#define CAN_SR_ERRWRN_MASK	0x00000040  /* Error Warning Mask */
#define CAN_SR_BBSY_MASK	0x00000020  /* Bus Busy Mask */
#define CAN_SR_BIDLE_MASK	0x00000010  /* Bus Idle Mask */
#define CAN_SR_NORMAL_MASK	0x00000008  /* Normal Mode Mask */
#define CAN_SR_SLEEP_MASK	0x00000004  /* Sleep Mode Mask */
#define CAN_SR_LBACK_MASK	0x00000002  /* Loop Back Mode Mask */
#define CAN_SR_CONFIG_MASK	0x00000001  /* Configuration Mode Mask */

/* Error Counter Register */
#define CAN_ECR_REC_MASK	0x0000FF00  /* Receive Error Counter Mask */
#define CAN_ECR_REC_SHIFT	8	    	/* Receive Error Counter Shift */
#define CAN_ECR_TEC_MASK	0x000000FF  /* Transmit Error Counter Mask */

/* Acceptance Filter Register */
#define CAN_AFR_UAF4_MASK	0x00000008  /* Use Acceptance Filter No.4 */
#define CAN_AFR_UAF3_MASK	0x00000004  /* Use Acceptance Filter No.3 */
#define CAN_AFR_UAF2_MASK	0x00000002  /* Use Acceptance Filter No.2 */
#define CAN_AFR_UAF1_MASK	0x00000001  /* Use Acceptance Filter No.1 */
#define CAN_AFR_UAF_ALL_MASK	(CAN_AFR_UAF4_MASK | CAN_AFR_UAF3_MASK | \
				 	 	 	 	 CAN_AFR_UAF2_MASK | CAN_AFR_UAF1_MASK)
#define CAN_AFR_MASK(n) (n == 1 ? CAN_AFR_UAF1_MASK : (n == 2 ? CAN_AFR_UAF2_MASK : (n == 3 ? CAN_AFR_UAF3_MASK : (n==4 ? CAN_AFR_UAF4_MASK : 0))))

/* Interrupt Status/Enable/Clear Register */
#define CAN_IXR_WKUP_MASK	0x00000800  /**< Wake up Interrupt Mask */
#define CAN_IXR_SLP_MASK	0x00000400  /**< Sleep Interrupt Mask */
#define CAN_IXR_BSOFF_MASK	0x00000200  /**< Bus Off Interrupt Mask */
#define CAN_IXR_ERROR_MASK	0x00000100  /**< Error Interrupt Mask */
#define CAN_IXR_RXNEMP_MASK	0x00000080  /**< RX FIFO Not Empty Intr Mask */
#define CAN_IXR_RXOFLW_MASK	0x00000040  /**< RX FIFO Overflow Intr Mask */
#define CAN_IXR_RXUFLW_MASK	0x00000020  /**< RX FIFO Underflow Intr Mask */
#define CAN_IXR_RXOK_MASK	0x00000010  /**< New Message Received Intr */
#define CAN_IXR_TXBFLL_MASK	0x00000008  /**< TX High Priority Buf Full  */
#define CAN_IXR_TXFLL_MASK	0x00000004  /**< TX FIFO Full Interrupt Mask */
#define CAN_IXR_TXOK_MASK	0x00000002  /**< TX Successful Interrupt Mask */
#define CAN_IXR_ARBLST_MASK	0x00000001  /**< Arbitration Lost Intr Mask */
#define CAN_IXR_ALL		(CAN_IXR_WKUP_MASK   | \
				CAN_IXR_SLP_MASK	| \
				CAN_IXR_BSOFF_MASK  | \
				CAN_IXR_ERROR_MASK  | \
				CAN_IXR_RXNEMP_MASK | \
 				CAN_IXR_RXOFLW_MASK | \
				CAN_IXR_RXUFLW_MASK | \
	 			CAN_IXR_RXOK_MASK   | \
				CAN_IXR_TXBFLL_MASK | \
				CAN_IXR_TXFLL_MASK  | \
				CAN_IXR_TXOK_MASK   | \
				CAN_IXR_ARBLST_MASK)

#define CAN_IXR_RX_TX_SUCCESS_MASK (CAN_IXR_RXNEMP_MASK | CAN_IXR_RXOK_MASK | CAN_IXR_TXOK_MASK)

#define CAN_IDR_ID1_MASK	0xFFE00000  /**< Standard Msg Ident Mask */
#define CAN_IDR_ID1_SHIFT	21	    /**< Standard Msg Ident Shift */
#define CAN_IDR_SRR_MASK	0x00100000  /**< Substitute Remote TX Req */
#define CAN_IDR_SRR_SHIFT	20
#define CAN_IDR_IDE_MASK	0x00080000  /**< Identifier Extension Mask */
#define CAN_IDR_IDE_SHIFT	19	    /**< Identifier Extension Shift */
#define CAN_IDR_ID2_MASK	0x0007FFFE  /**< Extended Msg Ident Mask */
#define CAN_IDR_ID2_SHIFT	1	    /**< Extended Msg Ident Shift */
#define CAN_IDR_RTR_MASK	0x00000001  /**< Remote TX Request Mask */

/*END DANT*/

								//500K at 8MHz Sample Point at 50.0% (http://www.bittiming.can-wiki.info/#XCAN)
#define CAN_BRP_500K           ((uint32_t)(2-1))
#define CAN_TSEG1_500K         ((uint32_t)(3-1))
#define CAN_TSEG2_500K         ((uint32_t)(4-1))
#define CAN_SJW_500K           ((uint32_t)(1-1))


#define Can_CreateIdValue(StandardId, SubRemoteTransReq, IdExtension, \
		ExtendedId, RemoteTransReq) \
	((((StandardId) << CAN_IDR_ID1_SHIFT) & CAN_IDR_ID1_MASK) | \
	(((SubRemoteTransReq) << CAN_IDR_SRR_SHIFT) & CAN_IDR_SRR_MASK) | \
	(((IdExtension) << CAN_IDR_IDE_SHIFT) & CAN_IDR_IDE_MASK) | \
	(((ExtendedId) << CAN_IDR_ID2_SHIFT) & CAN_IDR_ID2_MASK) | \
	((RemoteTransReq) & CAN_IDR_RTR_MASK))

typedef struct
{
  uint32_t id;
  uint8_t buf[8];
  uint8_t buf_len;
} can_t;


extern uint8_t can_init( uint8_t p );
extern uint8_t can_init_with_baud( uint8_t p, uint8_t prescaler, uint8_t syncJumpWidth, uint8_t timeSegment2, uint8_t timeSegment1);
extern uint8_t can_rx( uint8_t p, can_t *frame );
extern uint8_t can_tx( uint8_t p, can_t *frame );

/*DANT ADDED*/
extern uint8_t can_checkPort( uint8_t p );
extern uint8_t can_reset( uint8_t p );
extern uint8_t can_selfTest( uint8_t p );
extern uint8_t can_getMode( uint8_t p );
extern uint8_t can_enterMode( uint8_t p, uint8_t operationMode);
extern uint8_t can_setBaudRatePrescaler( uint8_t p, uint8_t prescaler );
extern uint8_t can_getBaudRatePrescaler( uint8_t p, uint8_t *pBaudRatePrescaler );
extern uint8_t can_setBitTiming( uint8_t p, uint8_t syncJumpWidth, uint8_t timeSegment2, uint8_t timeSegment1 );
extern uint8_t can_getBitTiming( uint8_t p, uint8_t *pSyncJumpWidth, uint8_t *pTimeSegment2, uint8_t *pTimeSegment1 );
extern uint8_t can_isTxFifoFull( uint8_t p );
extern uint8_t can_isRxEmpty( uint8_t p );
extern uint8_t can_isRxMsgReady( uint8_t p );
extern uint8_t can_isAcceptFilterBusy( uint8_t p, uint8_t *pAcceptFilterBusy );
extern uint8_t can_getCoreStatusRegisterContents( uint8_t p, uint32_t *pSRRegValue );
extern uint8_t can_sendTestFrame( uint8_t p );
extern uint8_t can_recvTestFrame( uint8_t p );
extern uint8_t can_getBusErrorCounter( uint8_t p, uint8_t *pRxErrorCount, uint8_t *pTxErrorCount );
extern uint8_t can_getBusErrorCounterCombined( uint8_t p, uint32_t *pRxTxErrorCount );
extern uint8_t can_getBusErrorStatus( uint8_t p, uint32_t *pBusErrorStatus );
extern uint8_t can_clearBusErrorStatus( uint8_t p, uint32_t mask );
extern uint8_t can_acceptFilterEnable( uint8_t p, uint32_t filterIndexes);
extern uint8_t can_acceptFilterDisable( uint8_t p, uint32_t filterIndexes );
extern uint8_t can_acceptFilterGetEnabled( uint8_t p, uint32_t *pAcceptFilterEnabled );
extern uint8_t can_acceptFilterSet( uint8_t p, uint32_t filterIndex, uint32_t maskValue, uint32_t idValue );
extern uint8_t can_acceptFilterGet( uint8_t p, uint32_t filterIndex, uint32_t *pMaskValue, uint32_t *pIdValue );

/* CAN Interrupts */
uint8_t can_interruptEnable(uint8_t p, uint32_t Mask);
uint8_t can_interruptEnableAll(uint32_t Mask);
uint8_t can_interruptDisable(uint8_t p, uint32_t Mask);
uint8_t can_interruptDisableAll(uint32_t Mask);
uint8_t can_interruptGetEnabled(uint8_t p, uint32_t *pInterruptEnabled);
uint8_t can_interruptGetStatus(uint8_t p, uint32_t *pInterruptStatus);
uint8_t can_interruptClear(uint8_t p, uint32_t Mask);
uint8_t can_interruptClearAll(uint32_t Mask);

/*END DANT ADDED*/
#endif
