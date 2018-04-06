#ifndef J1939CFG_H
#define J1939CFG_H

/* tick period in 0.1 milliseconds (e.g. 10=1ms) */
#define J1939CFG_TICK_PERIOD                       50

/* number of J1939 networks/ports */
#define J1939CFG_PORTS_NUM                          8

/* multi-packet TX buffer size per network */
#define J1939CFG_TP_TX_BUF_NUM                     15
#define J1939CFG_TP_TX_BUF_SIZE                  1024

/* multi-packet RX buffer size per network */
#define J1939CFG_TP_RX_BUF_NUM                     15
#define J1939CFG_TP_RX_BUF_SIZE                  1024

#ifdef _ISSUE_FITTING
/*TEST CODE (TEMPORARY) - reduced sizes to reduce footprint.  Now it does not appear to be needed but leaving here as reminder*/
/* multi-packet TX buffer size per network */
#define J1939CFG_TP_TX_BUF_NUM                     15
#define J1939CFG_TP_TX_BUF_SIZE                  512

/* multi-packet RX buffer size per network */
#define J1939CFG_TP_RX_BUF_NUM                     15
#define J1939CFG_TP_RX_BUF_SIZE                  512
#endif

/**************************************************************************************/
/* DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)										  */
/**************************************************************************************/
								//500K at 8MHz with 87.5% Sample Point (recommended for J1939) (http://www.bittiming.can-wiki.info/#XCAN)
#define CAN_J1939_BRP_500K      ((uint32_t)(1-1))
#define CAN_J1939_TSEG1_500K    ((uint32_t)(13-1))
#define CAN_J1939_TSEG2_500K    ((uint32_t)(2-1))
#define CAN_J1939_SJW_500K      ((uint32_t)(1-1))
/**************************************************************************************/
/* END DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)									  */
/**************************************************************************************/
#endif
