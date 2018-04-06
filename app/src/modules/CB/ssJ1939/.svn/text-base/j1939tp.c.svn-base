/*
  Copyright Simma Software, Inc. - 2013

  ssJ1939-Multi (Version 1.4)

  Use this software at your own risk.  Simma Software, Inc does not promise,
  state, or guarantee this software to be defect free.

  This module handles the transport protocol as defined in J1939-21.  There
  are four main sections:
     - Transmitting BAM type msgs (i.e. global dst)
     - Transmitting RTS/CTS type msgs (i.e. dst specific)
     - Receiving BAM type msgs (i.e. global dst)
     - Receiving RTS/CTS type msgs (i.e. dst specific)

  Each one of the above sections has an update function which is called
  periodically to handle time related issues.  They are:
     - j1939tp_update_tx_bam()
     - j1939tp_update_tx_rtscts()
     - j1939tp_update_rx_bam()
     - j1939tp_update_rx_rtscts()

  Besides the update functions, there are two routines that process incoming
  connection management (CM) and data transfer (DT) messages.  They are:
     - j1939tp_post_cm()
     - j1939tp_post_dt()
*/
#include "bits.h"
#include "j1939.h"
#include "j1939tp.h"
#include "j1939cfg.h"
#include "j1939app.h"

/**************************************************************************************/
/* DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)										  */
/**************************************************************************************/
extern uint16_t j1939_enabled_ports[ J1939CFG_PORTS_NUM ];
/**************************************************************************************/
/* END DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)									  */
/**************************************************************************************/

/* sanity check */
#if J1939CFG_TP_RX_BUF_SIZE > 1785
#error: J1939CFG_TP_RX_BUF_SIZE is to big
#endif
#if J1939CFG_TP_TX_BUF_SIZE > 1785
#error: J1939CFG_TP_TX_BUF_SIZE is to big
#endif


/* different transport protocol states */
#define J1939TP_STATE_UNUSED                                 0
#define J1939TP_STATE_SEND_BAM                               1
#define J1939TP_STATE_SEND_RTS                               2
#define J1939TP_STATE_SEND_CTS                               3
#define J1939TP_STATE_SEND_DATA                              4
#define J1939TP_STATE_SEND_ACK                               5
#define J1939TP_STATE_SEND_ABORT                             6
#define J1939TP_STATE_WAIT_CTS                               7
#define J1939TP_STATE_WAIT_DATA                              8
#define J1939TP_STATE_WAIT_ACK                               9

/* type of TP msg (see spec -21) */
#define J1939TP_CTRL_BYTE_RTS                               16
#define J1939TP_CTRL_BYTE_CTS                               17
#define J1939TP_CTRL_BYTE_ACK                               19
#define J1939TP_CTRL_BYTE_BAM                               32
#define J1939TP_CTRL_BYTE_ABORT                            255

/* reasons to send an abort msg (see spec -21) */
#define J1939TP_RSN_BUSY                                     1
#define J1939TP_RSN_RSRCS_FREED                              2
#define J1939TP_RSN_TIMEOUT                                  3
#define J1939TP_RSN_ERROR                                  254 

/* timeouts (see spec -21) */
#define J1939TP_TIMEOUT_BM         (500u/J1939CFG_TICK_PERIOD)
#define J1939TP_TIMEOUT_TR        (2000u/J1939CFG_TICK_PERIOD)
#define J1939TP_TIMEOUT_TH        (5000u/J1939CFG_TICK_PERIOD)
#define J1939TP_TIMEOUT_T1        (7500u/J1939CFG_TICK_PERIOD)
#define J1939TP_TIMEOUT_T2       (12500u/J1939CFG_TICK_PERIOD)
#define J1939TP_TIMEOUT_T3       (12500u/J1939CFG_TICK_PERIOD)
#define J1939TP_TIMEOUT_T4       (10500u/J1939CFG_TICK_PERIOD)



/* internal buffers used to hold
   large incoming and outgoing msgs */
typedef struct {

  uint8_t state;
  uint8_t *status;
  uint32_t pgn;
  uint16_t msg_size;
  uint16_t timer;
  uint8_t num_packets;
  uint8_t num_cur;
  uint8_t cts_cnt;
  uint8_t cts_rcvd;
  uint8_t rsn;
  uint8_t dst;
  uint8_t src;
  uint8_t buf[ J1939CFG_TP_TX_BUF_SIZE ];

} j1939tp_tx_t;

typedef struct {

  uint8_t state;
  uint32_t pgn;
  uint16_t msg_size;
  uint16_t timer;
  uint8_t num_packets;
  uint8_t next_seq;
  uint8_t cts_max;
  uint8_t cts_cnt;
  uint8_t rsn;
  uint8_t dst;
  uint8_t src;
  uint8_t buf[ J1939CFG_TP_RX_BUF_SIZE ];

} j1939tp_rx_t;

/* buffers */
j1939tp_tx_t j1939tp_txbuf[ J1939CFG_PORTS_NUM ][ J1939CFG_TP_TX_BUF_NUM ];
j1939tp_rx_t j1939tp_rxbuf[ J1939CFG_PORTS_NUM ][ J1939CFG_TP_RX_BUF_NUM ];

/* used instead of NULL pointers */
uint8_t j1939tp_tmp;



/*
** When a destination specific transmission fails this function
** informs the application and marks the buffer as unused. 
*/
void
j1939tp_rtscts_failed ( uint8_t p, uint8_t index ) {

  /* we failed */
  *j1939tp_txbuf[p][index].status = J1939TP_FAILED;
  j1939tp_txbuf[p][index].state = J1939TP_STATE_UNUSED;
}



/*
** Searches entire RX buffer for indexes which are currently receiving the
** same type of TP message (i.e. dst) from the same source and removes them.
*/
void
j1939tp_rxbuf_clean ( uint8_t p, uint8_t dst, uint8_t src ) {

  uint8_t cnt;

  for( cnt = 0; cnt < J1939CFG_TP_RX_BUF_NUM; cnt++ )
    if( (j1939tp_rxbuf[p][cnt].state != J1939TP_STATE_UNUSED) &&
        (j1939tp_rxbuf[p][cnt].dst == dst) &&
        (j1939tp_rxbuf[p][cnt].src == src) )
     j1939tp_rxbuf[p][cnt].state = J1939TP_STATE_UNUSED;

  return;
}



/*
** Searches entire RX buffer for the index which matches the incoming CM PGN.
*/
uint8_t
j1939tp_rxbuf_find ( uint8_t p, uint8_t dst, uint8_t src, uint32_t pgn ) {

  uint8_t cnt;

  for( cnt = 0; cnt < J1939CFG_TP_RX_BUF_NUM; cnt++ )
    if( (j1939tp_rxbuf[p][cnt].state != J1939TP_STATE_UNUSED)
         && (j1939tp_rxbuf[p][cnt].dst == dst)
         && (j1939tp_rxbuf[p][cnt].src == src)
         && (j1939tp_rxbuf[p][cnt].pgn == pgn) )
      break;

  return cnt;
}



/*
** Searches entire TX buffer for the index which matches the incoming CM PGN.
*/
uint8_t
j1939tp_txbuf_find ( uint8_t p, uint8_t dst, uint8_t src, uint32_t pgn ) {

  uint8_t cnt;

  for( cnt = 0; cnt < J1939CFG_TP_TX_BUF_NUM; cnt++ )
    if( (j1939tp_txbuf[p][cnt].state != J1939TP_STATE_UNUSED)
         && (j1939tp_txbuf[p][cnt].dst == src)
         && (j1939tp_txbuf[p][cnt].src == dst)
         && (j1939tp_txbuf[p][cnt].pgn == pgn) )
      break;

  return cnt;
}



/*
** Initializes the J1939 transport protocol (called from j1939.c)
*/
void
j1939tp_init ( void ) {

  uint8_t p;
  uint8_t cnt;

  for( p = 0; p < J1939CFG_PORTS_NUM; p++ ) {
	/* NAI - only attempt to init rx and tx buffers for those ports that are enabled! */
	if (j1939_enabled_ports[p] == 1)
	{
       for( cnt = 0; cnt < J1939CFG_TP_RX_BUF_NUM; cnt++ )
         j1939tp_rxbuf[p][cnt].state = J1939TP_STATE_UNUSED;

       for( cnt = 0; cnt < J1939CFG_TP_TX_BUF_NUM; cnt++ )
         j1939tp_txbuf[p][cnt].state = J1939TP_STATE_UNUSED;
	}
  }
}

/**************************************************************************************/
/* DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)										  */
/**************************************************************************************/
extern void
j1939tp_init_p ( uint8_t p ) {
  uint8_t cnt;

  /* NAI - only attempt to init rx and tx buffers if the given port is enabled! */
  if (p < J1939CFG_PORTS_NUM && (j1939_enabled_ports[p] == 1))
  {
    for( cnt = 0; cnt < J1939CFG_TP_RX_BUF_NUM; cnt++ )
	  j1939tp_rxbuf[p][cnt].state = J1939TP_STATE_UNUSED;

    for( cnt = 0; cnt < J1939CFG_TP_TX_BUF_NUM; cnt++ )
      j1939tp_txbuf[p][cnt].state = J1939TP_STATE_UNUSED;
  }
}

/**************************************************************************************/
/* END DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)									  */
/**************************************************************************************/

/*
** This function receives an outgoing message, with more than 8 bytes of data,
** from the j1939 module and attempts to place it in a TX buffer.  *status       
** will be set to J1939TP_INPROCESS during transmission, J1939TP_DONE for
** success, or J1939TP_FAILED for failure.
** RETURNS: 0 - successfully placed in TX buffer
**          1 - failed to be placed in TX buffer
*/
uint8_t
j1939tp_tx ( j1939_t *msg, uint8_t *status ) {

  uint8_t cnt, p;
  uint8_t index, ret = 1;

  p = msg->port;

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	  return 1;

  /* we can only transmit to one destination at a time, so check */ 
  for( cnt = 0; cnt < J1939CFG_TP_TX_BUF_NUM; cnt++ )
    if( (j1939tp_txbuf[p][cnt].state != J1939TP_STATE_UNUSED) &&
        (j1939tp_txbuf[p][cnt].dst == msg->dst) )
      break;

  /* are we ok to send and is there enough room? */
  if( (cnt == J1939CFG_TP_TX_BUF_NUM)
      && (msg->buf_len <= J1939CFG_TP_TX_BUF_SIZE) ) {

    /* find the first available tx buffer */
    for( index = 0; index < J1939CFG_TP_TX_BUF_NUM; index++ ) {

      if( j1939tp_txbuf[p][index].state == J1939TP_STATE_UNUSED ) {

        /* is it a BAM message or RTS/CTS? */
        if( msg->dst == J1939_ADDR_GLOBAL ) {

          j1939tp_txbuf[p][index].state = J1939TP_STATE_SEND_BAM;
          j1939tp_txbuf[p][index].timer = 0;

        } else {

          j1939tp_txbuf[p][index].state = J1939TP_STATE_SEND_RTS;
          j1939tp_txbuf[p][index].timer = J1939TP_TIMEOUT_T3;
        }

        /* if there isn't a pointer, use this tmp location */ 
        if( status == 0 )
          status = &j1939tp_tmp;

        *status = J1939TP_INPROCESS;
        j1939tp_txbuf[p][index].dst = msg->dst;
        j1939tp_txbuf[p][index].src = msg->src;
        j1939tp_txbuf[p][index].pgn = msg->pgn;
        j1939tp_txbuf[p][index].msg_size = msg->buf_len;
        j1939tp_txbuf[p][index].num_packets = (uint8_t)((msg->buf_len+6)/7);
        j1939tp_txbuf[p][index].num_cur = 1;
        j1939tp_txbuf[p][index].cts_cnt = 0;
        j1939tp_txbuf[p][index].cts_rcvd = 0;
        j1939tp_txbuf[p][index].status = status;
        for( cnt = 0; cnt < msg->buf_len; cnt++ )
          j1939tp_txbuf[p][index].buf[cnt] = msg->buf[cnt];

        ret = 0;
        break;
      }
    }
  }
 
  return ret; 
}



/*
** Transmits either a BAM or RTS msg.
** INPUTS: index - index into TX buffer
**         ctrl_byte - BAM or RTS
** RETURNS: 0 - success
**          1 - failure
*/
uint8_t
j1939tp_tx_bam_rts ( uint8_t p, uint8_t index, uint8_t ctrl_byte ) {

  j1939_t msg_tx;
  uint8_t buf[8];

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	  return 1;

  msg_tx.buf = buf;
  msg_tx.pgn = J1939_PGN_TP_CM;
  msg_tx.pri = 7;
  msg_tx.buf[0] = ctrl_byte;
  msg_tx.buf[1] = (uint8_t)j1939tp_txbuf[p][index].msg_size;
  msg_tx.buf[2] = (uint8_t)(j1939tp_txbuf[p][index].msg_size >> 8);
  msg_tx.buf[3] = j1939tp_txbuf[p][index].num_packets;
  msg_tx.buf[4] = 0xff;
  msg_tx.buf[5] = (uint8_t)j1939tp_txbuf[p][index].pgn;
  msg_tx.buf[6] = (uint8_t)(j1939tp_txbuf[p][index].pgn >> 8);
  msg_tx.buf[7] = (uint8_t)(j1939tp_txbuf[p][index].pgn >> 16); 
  msg_tx.buf_len = 8;
  msg_tx.dst = j1939tp_txbuf[p][index].dst;
  msg_tx.port = p;

  return j1939_tx_app( &msg_tx, 0 );
}



/*
** Transmits a DT msg.
** INPUTS: index - index into TX buffer
** RETURNS: 0 - success
**          1 - failure
*/
uint8_t
j1939tp_tx_dt ( uint8_t p, uint8_t index ) {

  j1939_t msg_tx;
  uint8_t buf[8], cnt;
  uint16_t b_rem, b_snt;

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	  return 1;

  msg_tx.buf = buf;
  msg_tx.pgn = J1939_PGN_TP_DT;
  msg_tx.pri = 7;
  msg_tx.buf[0] = j1939tp_txbuf[p][index].num_cur;
  msg_tx.buf_len = 8;
  msg_tx.dst = j1939tp_txbuf[p][index].dst;
  msg_tx.port = p;

  /* how many bytes have been sent and are remaining? */
  b_snt = (j1939tp_txbuf[p][index].num_cur-1)*7;
  b_rem = j1939tp_txbuf[p][index].msg_size - b_snt;

  for( cnt = 0; cnt < 7; cnt++ )
    msg_tx.buf[1+cnt] = (b_rem>cnt) ? j1939tp_txbuf[p][index].buf[b_snt+cnt] : 255;

  return j1939_tx_app( &msg_tx, 0 );
}



/*
** Transmits an abort CM msg.
** INPUTS: dst - msg desination
**         rsn - reason for abort
**         pgn - PGN
** RETURNS: 0 - success
**          1 - failure
*/
uint8_t
j1939tp_tx_abort ( uint8_t p, uint8_t dst, uint8_t rsn, uint32_t pgn ) {

  j1939_t msg_tx;
  uint8_t buf[8];

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	  return 1;

  msg_tx.buf = buf;
  msg_tx.pgn = J1939_PGN_TP_CM;
  msg_tx.pri = 7;
  msg_tx.buf[0] = J1939TP_CTRL_BYTE_ABORT;
  msg_tx.buf[1] = rsn;
  msg_tx.buf[2] = 0xff;
  msg_tx.buf[3] = 0xff;
  msg_tx.buf[4] = 0xff;
  msg_tx.buf[5] = (uint8_t)pgn;
  msg_tx.buf[6] = (uint8_t)(pgn >> 8);
  msg_tx.buf[7] = (uint8_t)(pgn >> 16); 
  msg_tx.buf_len = 8;
  msg_tx.dst = dst;
  msg_tx.port = p;

  return j1939_tx_app(&msg_tx,0);
}



/*
** Transmits a CTS CM msg.
** INPUTS: index - index into TX buffer
** RETURNS: 0 - success
**          1 - failure
*/
uint8_t
j1939tp_tx_cts ( uint8_t p, uint8_t index ) {

  j1939_t msg_tx;
  uint8_t buf[8];

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	  return 1;

  msg_tx.buf = buf;
  msg_tx.pgn = J1939_PGN_TP_CM;
  msg_tx.pri = 7;
  msg_tx.buf[0] = J1939TP_CTRL_BYTE_CTS;
  msg_tx.buf[1] = j1939tp_rxbuf[p][index].cts_max;
  msg_tx.buf[2] = j1939tp_rxbuf[p][index].next_seq;
  msg_tx.buf[3] = 0xff;
  msg_tx.buf[4] = 0xff;
  msg_tx.buf[5] = (uint8_t)(j1939tp_rxbuf[p][index].pgn);
  msg_tx.buf[6] = (uint8_t)(j1939tp_rxbuf[p][index].pgn >> 8);
  msg_tx.buf[7] = (uint8_t)(j1939tp_rxbuf[p][index].pgn >> 16); 
  msg_tx.buf_len = 8;
  msg_tx.dst = j1939tp_rxbuf[p][index].src;
  msg_tx.port = p;

  return j1939_tx_app(&msg_tx,0);
}



/*
** Transmits an ack CM msg.
** INPUTS: index - index into TX buffer
** RETURNS: 0 - success
**          1 - failure
*/
uint8_t
j1939tp_tx_ack ( uint8_t p, uint8_t index ) {

  j1939_t msg_tx;
  uint8_t buf[8];

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	  return 1;

  msg_tx.buf = buf;
  msg_tx.pgn = J1939_PGN_TP_CM;
  msg_tx.pri = 7;
  msg_tx.buf[0] = J1939TP_CTRL_BYTE_ACK;
  msg_tx.buf[1] = (uint8_t)j1939tp_rxbuf[p][index].msg_size;
  msg_tx.buf[2] = j1939tp_rxbuf[p][index].msg_size >> 8;
  msg_tx.buf[3] = j1939tp_rxbuf[p][index].num_packets;
  msg_tx.buf[4] = 0xff;
  msg_tx.buf[5] = (uint8_t)j1939tp_rxbuf[p][index].pgn;
  msg_tx.buf[6] = (uint8_t)(j1939tp_rxbuf[p][index].pgn >> 8);
  msg_tx.buf[7] = (uint8_t)(j1939tp_rxbuf[p][index].pgn >> 16); 
  msg_tx.buf_len = 8;
  msg_tx.dst = j1939tp_rxbuf[p][index].src;
  msg_tx.port = p;

  return j1939_tx_app(&msg_tx,0);
}



/*
** RX BAM state machine for processing received DT messages.
** INPUTS: index - index into RX buffer
**         msg - pointer to received msg
*/
void
j1939tp_state_rx_bam ( uint8_t p, uint8_t index, j1939_t *msg ) {

  j1939_t n_msg;
  uint16_t cnt, b_rem, b_rcv, min;

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	return;

  /* how many bytes have been received and are remaining? */
  b_rcv = (j1939tp_rxbuf[p][index].next_seq-1)*7;
  b_rem = j1939tp_rxbuf[p][index].msg_size - b_rcv;

  /* the entire CAN frame might not be data, so we need to know
     how many bytes are suppose to be left */
  min = 7;
  if( b_rem < 7 )
    min = b_rem;

  switch( j1939tp_rxbuf[p][index].state ) {

    case J1939TP_STATE_WAIT_DATA: {
  
      /* check to make sure that the sequence number and rx count
         are correct and that an overflow won't occur */
      if( (j1939tp_rxbuf[p][index].next_seq == msg->buf[0])
           && ((b_rcv+min) <= J1939CFG_TP_RX_BUF_SIZE)
           && ((b_rcv+b_rem) == j1939tp_rxbuf[p][index].msg_size) ) {

        j1939tp_rxbuf[p][index].timer = 0;
        for( cnt = 0; cnt < min; cnt++ )
          j1939tp_rxbuf[p][index].buf[b_rcv+cnt] = msg->buf[1+cnt];

        /* was that the last packet? */
        if( j1939tp_rxbuf[p][index].next_seq++ ==
            j1939tp_rxbuf[p][index].num_packets ) {
  
          n_msg.pgn = j1939tp_rxbuf[p][index].pgn; 
          n_msg.buf = j1939tp_rxbuf[p][index].buf; 
          n_msg.buf_len = j1939tp_rxbuf[p][index].msg_size; 
          n_msg.dst = j1939tp_rxbuf[p][index].dst; 
          n_msg.src = j1939tp_rxbuf[p][index].src; 
          n_msg.port = p;
          j1939app_process( &n_msg );
          j1939tp_rxbuf[p][index].state = J1939TP_STATE_UNUSED;
        }

      } else {

        /* sequence was wrong, so we discard the entire message */
        j1939tp_rxbuf[p][index].state = J1939TP_STATE_UNUSED;
      }
      break;
    }

    default:
      j1939tp_rxbuf[p][index].state = J1939TP_STATE_UNUSED;
      break;
  }

  return;
}



/*
** RX BAM state machine for processing update events.  Once we've started
** receving a BAM this function determines if it's timed out.
** INPUTS: index - index into RX buffer
*/
void
j1939tp_update_rx_bam ( uint8_t p, uint8_t index ) {

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
    return;

  /* see if this BAM has timed out */
  if( ++j1939tp_rxbuf[p][index].timer >= J1939TP_TIMEOUT_T1 )
    j1939tp_rxbuf[p][index].state = J1939TP_STATE_UNUSED;

  return;
}



/*
** TX BAM state machine for processing update events.  Once a message has been
** placed into a TX buffer this handles sending out the data msgs.
** INPUTS: index - index into RX buffer
*/
void
j1939tp_update_tx_bam ( uint8_t p, uint8_t index ) {

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	return;

  /* either send a BAM or DT */
  switch( j1939tp_txbuf[p][index].state ) {

    case J1939TP_STATE_SEND_BAM: {

      if( j1939tp_tx_bam_rts(p,index,J1939TP_CTRL_BYTE_BAM) == 0 ) {

        j1939tp_txbuf[p][index].timer = 0;
        j1939tp_txbuf[p][index].state = J1939TP_STATE_SEND_DATA;
      }
      break;
    }

    case J1939TP_STATE_SEND_DATA: {

      /* has it been at least 200 ms since the last transmit? */
      if( ++j1939tp_txbuf[p][index].timer > J1939TP_TIMEOUT_TR ) {
  
        /* it's been too long, so mark this as failed */
        j1939tp_txbuf[p][index].state = J1939TP_STATE_UNUSED;

        /* we failed */
        *j1939tp_txbuf[p][index].status = J1939TP_FAILED;

      /* transmit when it's been at least 50 ms */
      } else if( j1939tp_txbuf[p][index].timer > J1939TP_TIMEOUT_BM ) {

        if( j1939tp_tx_dt(p,index) == 0 ) {

          j1939tp_txbuf[p][index].timer = 0;
          if(j1939tp_txbuf[p][index].num_cur++ >=j1939tp_txbuf[p][index].num_packets){
            j1939tp_txbuf[p][index].state = J1939TP_STATE_UNUSED;
            *j1939tp_txbuf[p][index].status = J1939TP_DONE;
          }
        }
      }
      break;
    }

    default:
      j1939tp_txbuf[p][index].state = J1939TP_STATE_UNUSED;
      *j1939tp_txbuf[p][index].status = J1939TP_FAILED;
      break;
  }

  return;
}



/*
** RX RTS/CTS state machine for processing received DT messages.  It checks the
** DT message for validity and copies the data into the matching RX buffer.
** INPUTS: index - index into RX buffer
**         msg - pointer to received message
*/
void
j1939tp_state_rx_rtscts ( uint8_t p, uint8_t index, j1939_t *msg ) {

  j1939_t n_msg;
  uint16_t cnt, b_rem, b_rcv, min;

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	return;

  /* how many bytes have been received and are remaining? */
  b_rcv = (j1939tp_rxbuf[p][index].next_seq-1)*7;
  b_rem = j1939tp_rxbuf[p][index].msg_size - b_rcv;

  /* the entire CAN frame might not be data, so we need to know
     how many bytes are suppose to be left */
  min = 7;
  if( b_rem < 7 )
    min = b_rem;

  if( j1939tp_rxbuf[p][index].state == J1939TP_STATE_WAIT_DATA ) {

    if( (j1939tp_rxbuf[p][index].next_seq == msg->buf[0])
         && ((b_rcv+min) <= J1939CFG_TP_RX_BUF_SIZE)
         && ((b_rcv+b_rem) == j1939tp_rxbuf[p][index].msg_size) ) {

      j1939tp_rxbuf[p][index].timer = J1939TP_TIMEOUT_T1;
      for( cnt = 0; cnt < min; cnt++ )
        j1939tp_rxbuf[p][index].buf[b_rcv+cnt] = msg->buf[1+cnt];

      /* was that the last packet? */
      if( j1939tp_rxbuf[p][index].next_seq++ >= j1939tp_rxbuf[p][index].num_packets ) {
  
        n_msg.pgn = j1939tp_rxbuf[p][index].pgn; 
        n_msg.buf = j1939tp_rxbuf[p][index].buf; 
        n_msg.buf_len = j1939tp_rxbuf[p][index].msg_size; 
        n_msg.dst = j1939tp_rxbuf[p][index].dst; 
        n_msg.src = j1939tp_rxbuf[p][index].src; 
        n_msg.port = p;
        j1939app_process(&n_msg);
        j1939tp_rxbuf[p][index].timer = J1939TP_TIMEOUT_TR;
        j1939tp_rxbuf[p][index].state = J1939TP_STATE_SEND_ACK;

      } else if( ++j1939tp_rxbuf[p][index].cts_cnt >= j1939tp_rxbuf[p][index].cts_max ) {

        j1939tp_rxbuf[p][index].timer = J1939TP_TIMEOUT_TR;
        j1939tp_rxbuf[p][index].state = J1939TP_STATE_SEND_CTS;
      }

    } else {
     
      j1939tp_rxbuf[p][index].rsn = J1939TP_RSN_RSRCS_FREED;
      j1939tp_rxbuf[p][index].state = J1939TP_STATE_SEND_ABORT;
    }
  }

  return;
}



/*
** RX RTS/CTS state machine for processing update events.  Once a message has been
** placed into a RX buffer this handles sending out the CTS/ACK/ABORT messages. 
** INPUTS: index - index into RX buffer
*/
void
j1939tp_update_rx_rtscts( uint8_t p, uint8_t index ) {

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	return;

  /* see if any stage has timed out */
  if( j1939tp_rxbuf[p][index].timer )
    if( --j1939tp_rxbuf[p][index].timer == 0 ) {
      j1939tp_rxbuf[p][index].rsn = J1939TP_RSN_TIMEOUT;
      j1939tp_rxbuf[p][index].state = J1939TP_STATE_SEND_ABORT;
    }

  switch( j1939tp_rxbuf[p][index].state ) {

    case J1939TP_STATE_SEND_CTS: {
      if( j1939tp_tx_cts(p,index) == 0 ) {

        j1939tp_rxbuf[p][index].cts_cnt = 0;
        j1939tp_rxbuf[p][index].timer = J1939TP_TIMEOUT_T3;
        j1939tp_rxbuf[p][index].state = J1939TP_STATE_WAIT_DATA;
      }
      break;
    }

    case J1939TP_STATE_SEND_ACK: {
      if( j1939tp_tx_ack(p,index) == 0 )
        j1939tp_rxbuf[p][index].state = J1939TP_STATE_UNUSED;
      break;
    }

    case J1939TP_STATE_SEND_ABORT: {
      if( j1939tp_tx_abort(p, j1939tp_rxbuf[p][index].src,
                           j1939tp_rxbuf[p][index].rsn,
                           j1939tp_rxbuf[p][index].pgn) == 0 )
        j1939tp_rxbuf[p][index].state = J1939TP_STATE_UNUSED;
      break;
    }
  }
}



/*
** TX RTS/CTS state machine for processing update events.  Once a message has been
** placed into a TX buffer this handles sending out the RTS/DT/ABORT messages. 
** INPUTS: index - index into TX buffer
*/
void
j1939tp_update_tx_rtscts( uint8_t p, uint8_t index ) {

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
    return;

  /* see if this msg has timed out */
  if( j1939tp_txbuf[p][index].timer ) {
    if( --j1939tp_txbuf[p][index].timer == 0 ) {
      if( j1939tp_txbuf[p][index].state != J1939TP_STATE_SEND_RTS ) {
        j1939tp_txbuf[p][index].rsn = J1939TP_RSN_TIMEOUT;
        j1939tp_txbuf[p][index].state = J1939TP_STATE_SEND_ABORT;
      } else {
        j1939tp_rtscts_failed( p, index );
      }
    }
  }

  switch( j1939tp_txbuf[p][index].state ) {

    case J1939TP_STATE_SEND_RTS: {

      if( j1939tp_tx_bam_rts(p,index,J1939TP_CTRL_BYTE_RTS) == 0 ) {

        j1939tp_txbuf[p][index].timer = J1939TP_TIMEOUT_T3;
        j1939tp_txbuf[p][index].state = J1939TP_STATE_WAIT_CTS;
      }
      break;
    }

    case J1939TP_STATE_SEND_DATA: {

      if( j1939tp_tx_dt(p,index) == 0 ) {

        j1939tp_txbuf[p][index].timer = J1939TP_TIMEOUT_TR;

        if( j1939tp_txbuf[p][index].num_cur++ >=
            j1939tp_txbuf[p][index].num_packets ) {

          j1939tp_txbuf[p][index].state = J1939TP_STATE_WAIT_ACK;

        } else if( --j1939tp_txbuf[p][index].cts_cnt == 0 ) {

          j1939tp_txbuf[p][index].state = J1939TP_STATE_WAIT_CTS;
        }
      }
      break;
    }

    case J1939TP_STATE_SEND_ABORT: {

      if( j1939tp_txbuf[p][index].cts_rcvd ) {

        if( j1939tp_tx_abort(p,j1939tp_txbuf[p][index].dst,
                             j1939tp_txbuf[p][index].rsn,
                             j1939tp_txbuf[p][index].pgn) == 0 ) {

          j1939tp_rtscts_failed( p, index );
        }

      } else {

        j1939tp_rtscts_failed( p, index );
      }
      break;
    }
  }

  return;
}



/*
** This function receives CM (BAM/RTS/CTS/ACK/ABORT) messages directly
** from j1939.c and decides what to do with them.
** INPUT: msg - pointer to received CM message
*/
void
j1939tp_post_cm ( j1939_t *msg ) {

  uint8_t p;
  uint8_t cnt;
  uint32_t pgn;
  uint16_t msg_size;

  p = msg->port;

  /* all cm messages have the pgn in the same location */
  pgn = msg->buf[5] | ((uint32_t)msg->buf[6]<<8) | ((uint32_t)msg->buf[7]<<16);

  /* msg_size is in RTS, ACK, and BAM so make sure it's only used there */
  msg_size = msg->buf[1] | ((uint16_t)msg->buf[2] << 8);

  switch( msg->buf[0] ) {

    case J1939TP_CTRL_BYTE_BAM: {

      /* to be a valid BAM, it must have a global dst */
      if( msg->dst == J1939_ADDR_GLOBAL ) {

        /* first check to make sure we're not receiving a BAM from this addr.
           if we are, then discard the old BAM. */
        j1939tp_rxbuf_clean( msg->port, J1939_ADDR_GLOBAL, msg->src );

        /* will the msg fit and does it have a correct size? */
        if( (msg_size <= J1939CFG_TP_RX_BUF_SIZE) &&
            (msg_size <= (uint16_t)msg->buf[3] * 7) ) {

          /* everything looks good, so find a slot for it */
          for( cnt = 0; cnt < J1939CFG_TP_RX_BUF_NUM; cnt++ ) {

            if( j1939tp_rxbuf[p][cnt].state == J1939TP_STATE_UNUSED ) {

              j1939tp_rxbuf[p][cnt].state = J1939TP_STATE_WAIT_DATA;
              j1939tp_rxbuf[p][cnt].pgn = pgn;
              j1939tp_rxbuf[p][cnt].msg_size = msg_size;
              j1939tp_rxbuf[p][cnt].num_packets = msg->buf[3];
              j1939tp_rxbuf[p][cnt].next_seq = 1;
              j1939tp_rxbuf[p][cnt].timer = 0;
              j1939tp_rxbuf[p][cnt].dst = J1939_ADDR_GLOBAL;
              j1939tp_rxbuf[p][cnt].src = msg->src;
              break;
            }
          }
        }
      }

      break;
    }

    case J1939TP_CTRL_BYTE_RTS: {
 
      /* someone wants to establish a rts/cts connection.
         it must not have a global dst, which means it has our dst */
      if( msg->dst != J1939_ADDR_GLOBAL ) {

        /* first check to make sure we're not receiving a rts/cts from
           this addr.  if we are, then discard the old msg. */
        j1939tp_rxbuf_clean( msg->port, msg->dst, msg->src );

        /* will the msg fit and does it have a correct size? */
        if( (msg_size <= J1939CFG_TP_RX_BUF_SIZE) &&
            (msg_size <= (uint16_t)msg->buf[3] * 7) ) {

          /* everything looks good, so find a slot for it */
          for( cnt = 0; cnt < J1939CFG_TP_RX_BUF_NUM; cnt++ ) {

            if( j1939tp_rxbuf[p][cnt].state == J1939TP_STATE_UNUSED ) {

              j1939tp_rxbuf[p][cnt].state = J1939TP_STATE_SEND_CTS;
              j1939tp_rxbuf[p][cnt].pgn = pgn;
              j1939tp_rxbuf[p][cnt].msg_size = msg_size;
              j1939tp_rxbuf[p][cnt].num_packets = msg->buf[3];
              j1939tp_rxbuf[p][cnt].next_seq = 1;
              j1939tp_rxbuf[p][cnt].timer = J1939TP_TIMEOUT_TR;
              j1939tp_rxbuf[p][cnt].cts_max = msg->buf[4];
              j1939tp_rxbuf[p][cnt].dst = msg->dst;
              j1939tp_rxbuf[p][cnt].src = msg->src;
              break;
            }
          }

        } else {

          /* since we can't, we should send an abort */
          (void)j1939tp_tx_abort(p,msg->src, J1939TP_RSN_BUSY, pgn);
        }
      }
      break;
    }

    case J1939TP_CTRL_BYTE_CTS: {

      cnt = j1939tp_txbuf_find(p,msg->dst, msg->src, pgn);
      if( cnt < J1939CFG_TP_TX_BUF_NUM ) {

        /* if we never get a CTS, then an abort shouldn't be sent.  if we did
           get a CTS, then an abort needs to be sent if a timeout happens */
        j1939tp_txbuf[p][cnt].cts_rcvd = 1;

        /* spec says only 1 CTS can be received at a time */
        if( j1939tp_txbuf[p][cnt].cts_cnt ) {

          j1939tp_txbuf[p][cnt].rsn = J1939TP_RSN_ERROR;
          j1939tp_txbuf[p][cnt].state = J1939TP_STATE_SEND_ABORT;
          j1939tp_txbuf[p][cnt].timer = J1939TP_TIMEOUT_TR;

        } else if( (j1939tp_txbuf[p][cnt].state == J1939TP_STATE_WAIT_CTS)
                   || (j1939tp_txbuf[p][cnt].state == J1939TP_STATE_WAIT_ACK) ) {
  
            j1939tp_txbuf[p][cnt].state = J1939TP_STATE_SEND_DATA;
            j1939tp_txbuf[p][cnt].cts_cnt = msg->buf[1];
            j1939tp_txbuf[p][cnt].num_cur = msg->buf[2];
            if( j1939tp_txbuf[p][cnt].cts_cnt )
              j1939tp_txbuf[p][cnt].timer = J1939TP_TIMEOUT_TR;
            else
              j1939tp_txbuf[p][cnt].timer = J1939TP_TIMEOUT_T4;
        }
      }
      break;
    }

    case J1939TP_CTRL_BYTE_ACK: {

      cnt = j1939tp_txbuf_find(p,msg->dst, msg->src, pgn);
      if( cnt < J1939CFG_TP_TX_BUF_NUM ) {

        if( j1939tp_txbuf[p][cnt].state == J1939TP_STATE_WAIT_ACK ) {

          j1939tp_txbuf[p][cnt].state = J1939TP_STATE_UNUSED;
          if( j1939tp_txbuf[p][cnt].msg_size == msg_size ) {

            /* we passed */
            *j1939tp_txbuf[p][cnt].status = J1939TP_DONE;

          } else {

            j1939tp_rtscts_failed( p, cnt );
          }
        }
      }

      break;
    }

    case J1939TP_CTRL_BYTE_ABORT: {

      cnt = j1939tp_txbuf_find(msg->port, msg->dst, msg->src, pgn);
      if( cnt < J1939CFG_TP_TX_BUF_NUM )
        j1939tp_rtscts_failed( p, cnt );

      cnt = j1939tp_rxbuf_find(msg->port, msg->dst, msg->src, pgn);
      if( cnt < J1939CFG_TP_RX_BUF_NUM )
        j1939tp_rxbuf[p][cnt].state = J1939TP_STATE_UNUSED;

      break;
    }
  }

  return;
}



/*
** This function receives DT messages directly from j1939.c
** and decides what to do with them.
** INPUT: msg - pointer to received DT message
*/
void
j1939tp_post_dt ( j1939_t *msg ) {

  uint8_t p;
  uint8_t cnt;

  p = msg->port;

  /* find which receive buffer this matches with */
  for( cnt = 0; cnt < J1939CFG_TP_RX_BUF_NUM; cnt++ ) {

    if( (j1939tp_rxbuf[p][cnt].state != J1939TP_STATE_UNUSED) &&
        (j1939tp_rxbuf[p][cnt].dst == msg->dst) &&
        (j1939tp_rxbuf[p][cnt].src == msg->src) ) {

      if( msg->dst == J1939_ADDR_GLOBAL )
         j1939tp_state_rx_bam( p, cnt, msg );
      else
         j1939tp_state_rx_rtscts( p, cnt, msg );

      break;
    }
  }

  return;
}



/*
** This function is called from j1939.c at the system tick rate and is
** responsible for looking at all indexes in the RX and TX buffers and 
** calling the update functions for the indexes which are being used.
*/
void
j1939tp_update ( uint8_t p ) {
  
  uint8_t cnt;

  /* NAI - if port involved is not enabled for J1939...error! */
  if (j1939_enabled_ports[p] == 0)
	return;

  for( cnt = 0; cnt < J1939CFG_TP_RX_BUF_NUM; cnt++ ) {

    if( j1939tp_rxbuf[p][cnt].state != J1939TP_STATE_UNUSED ) {

      if( j1939tp_rxbuf[p][cnt].dst == J1939_ADDR_GLOBAL )
        j1939tp_update_rx_bam( p, cnt );
      else
        j1939tp_update_rx_rtscts( p, cnt );
    }
  }

  for( cnt = 0; cnt < J1939CFG_TP_TX_BUF_NUM; cnt++ ) {

    if( j1939tp_txbuf[p][cnt].state != J1939TP_STATE_UNUSED ) {

      if( j1939tp_txbuf[p][cnt].dst == J1939_ADDR_GLOBAL )
        j1939tp_update_tx_bam( p, cnt );
      else
        j1939tp_update_tx_rtscts( p, cnt );
    }
  }

  return;
}
