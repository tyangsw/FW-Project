/*
  Copyright Simma Software, Inc. - 2013

  ssJ1939-Multi (Version 1.4)

  Use this software at your own risk.  Simma Software, Inc does not promise,
  state, or guarantee this software to be defect free.

  This file contains software that relates to the implementation of the
  data-link and network management layer of J1939-21 & J1939-81.
*/
#include <stdint.h>
#include <stdio.h>

#include "bits.h"
#include "can.h"
#include "j1939.h"
#include "j1939app.h"
#include "j1939tp.h"
#include "j1939cfg.h"

/**************************************************************************************/
/* DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)										  */
/**************************************************************************************/
#include <string.h>
/**************************************************************************************/
/* END DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)									  */
/**************************************************************************************/

/* wait 250 ms before claiming an address */
#define J1939_ADDRESS_CLAIM_TIME_OUT           (2500u/J1939CFG_TICK_PERIOD)

/* apprx number of msgs in 250ms */
#define J1939_BIP_MSGS_250MS                                           477u

/* babble protection window */
#define J1939_BIP_WINDOW                       (2500u/J1939CFG_TICK_PERIOD)

/**************************************************************************************/
/* DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)										  */
/**************************************************************************************/
uint16_t j1939_enabled_ports[ J1939CFG_PORTS_NUM ];
/**************************************************************************************/
/* END DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)									  */
/**************************************************************************************/

/* babbling-idiot protection variables */
uint16_t j1939_bip_time[ J1939CFG_PORTS_NUM ];
uint16_t j1939_bip_tx_cnt[ J1939CFG_PORTS_NUM ];
uint16_t j1939_bip_tx_cnt_max[ J1939CFG_PORTS_NUM ];
uint8_t  j1939_bip_tx_is_disabled[ J1939CFG_PORTS_NUM ];
uint16_t j1939_bip_tx_cnt_allowed[ J1939CFG_PORTS_NUM ];

/* address claim algorithm variables */
uint16_t j1939_address_claim_time[ J1939CFG_PORTS_NUM ];
uint8_t  j1939_address_claim_requested[ J1939CFG_PORTS_NUM ];
uint8_t  j1939_address_claim_has_failed[ J1939CFG_PORTS_NUM ];
uint8_t  j1939_address_claim_tx_requested[ J1939CFG_PORTS_NUM ];
uint8_t  j1939_address_claim_has_been_sent[ J1939CFG_PORTS_NUM ];
uint8_t  j1939_address_claim_was_successful[ J1939CFG_PORTS_NUM ];
uint8_t  j1939_address_cannot_claim_tx_requested[ J1939CFG_PORTS_NUM ];
uint8_t  j1939_address_claim_received_higher_priority[ J1939CFG_PORTS_NUM ];

/* source address */
uint8_t j1939_sa[ J1939CFG_PORTS_NUM ];

/* modifiable name field */
uint8_t j1939_name[ J1939CFG_PORTS_NUM ][ 8 ];



/*
** Sets the NAME for the controlling application.
** INPUT: n - pointer to NAME structure.
*/
void
j1939_name_set ( uint8_t p, j1939_name_t *n )
{
  j1939_name[p][0] = (uint8_t)(n->identy_num);
  j1939_name[p][1] = (uint8_t)(n->identy_num >> 8);
  j1939_name[p][2] = (uint8_t)((n->mfg_code << 5) | ((n->identy_num >> 16)&0x1F));
  j1939_name[p][3] = (uint8_t)(n->mfg_code >> 3);
  j1939_name[p][4] = (uint8_t)((n->func_inst << 3) | n->ecu_inst);
  j1939_name[p][5] = n->func;
  j1939_name[p][6] = (uint8_t)(n->veh_sys << 1);
  j1939_name[p][7] = (uint8_t)((n->aac << 7) | (n->ind_grp << 4) | (n->veh_sys_inst));
}



/*
** Sets the application's max allowable bus load threshold
** INPUT: rate - percent load from 0 to 100 
*/
void
j1939_bip_tx_rate_allowed_set ( uint8_t p, uint8_t rate ) {

  if( rate <= 100 )
    j1939_bip_tx_cnt_allowed[p] = (J1939_BIP_MSGS_250MS*(uint16_t)rate)/100U;

  return;
}



/*
** Returns the application's peak bus load usage
** RETURN: percent load from 0 to 100 
*/
uint8_t
j1939_bip_tx_rate_max_get ( uint8_t p ) {

  return ((j1939_bip_tx_cnt_max[p] * 100U)/J1939_BIP_MSGS_250MS);
}



/*
** Updates the babbling idiot state machine 
*/
void
j1939_bip_update ( uint8_t p ) {

  /* keep track of max */
  if( j1939_bip_tx_cnt[p] > j1939_bip_tx_cnt_max[p] )
    j1939_bip_tx_cnt_max[p] = j1939_bip_tx_cnt[p];

  /* if max is bigger than allowed, then shutdown transmitter */
  if( j1939_bip_tx_cnt_max[p] > j1939_bip_tx_cnt_allowed[p] )
    j1939_bip_tx_is_disabled[p] = 1;

  if( ++j1939_bip_time[p] >= J1939_BIP_WINDOW ) {

    j1939_bip_time[p] = 0;
    j1939_bip_tx_cnt[p] = 0;
  }

  return;
}



/*
** Initializes the entire J1939 protocol stack 
*/
void
j1939_init ( void ) {

  uint8_t p;

  for( p = 0; p < J1939CFG_PORTS_NUM; p++ ) {

    /*NAI - initialize all ports to not being enabled by default. */
	j1939_enabled_ports[p] = 0;

    /* babbling-idiot protection variables */
    j1939_bip_time[p] = 0;
    j1939_bip_tx_cnt[p] = 0;
    j1939_bip_tx_cnt_max[p] = 0;
    j1939_bip_tx_is_disabled[p] = 0;
    j1939_bip_tx_cnt_allowed[p] = J1939_BIP_MSGS_250MS/4U;

    /* address claim algorithm variables */
    j1939_address_claim_time[p] = 0;
    j1939_address_claim_requested[p] = 0;
    j1939_address_claim_has_failed[p] = 0;
#ifdef _DANT /*DEFER THIS UNTIL CALLER ACTUALLY ATTEMPTS TO CLAIM AN ADDRESS*/
    j1939_address_claim_tx_requested[p] = 1;
#else
    j1939_address_claim_tx_requested[p] = 0;
#endif
    j1939_address_claim_has_been_sent[p] = 0;
    j1939_address_claim_was_successful[p] = 0;
    j1939_address_cannot_claim_tx_requested[p] = 0;
    j1939_address_claim_received_higher_priority[p] = 0;

#ifdef _DANT /*THIS IS NOW ONLY PERFORMED WHEN PORT IS "ENABLED" FOR J1939*/
    /* get the source address */
    j1939_sa[p] = j1939app_sa_get(p);
#endif
  }

#ifdef _DANT /*THIS IS NOW ONLY PERFORMED FOR EACH PORT WHEN "ENABLED" FOR J1939*/
  j1939tp_init();
  j1939app_init();
#endif
}



/*
** This function is the first level of inspection of a J1939 message.  It's
** sole purpose is to decide where the message should go from here.
** INPUT: msg - pointer to J1939 message
*/
void
j1939_process ( j1939_t *msg ) {

  uint8_t p;
  uint8_t cnt, tmp_sa;
  uint32_t rpgn;

  switch( msg->pgn ) {

    /* address claim PGN */
    case J1939_PGN_ADDRESS_CLAIMED: {

      /* what CAN port do we need to service */
      p = msg->port;

      /* if this message isn't using the same source address, ignore it */
      if( msg->src != j1939_sa[p] )
        break;

      /* find first byte which is different (MSB is byte 7) */
      for( cnt=7; (msg->buf[cnt]==j1939_name[p][cnt]) && (cnt > 0); cnt-- );

      /* is their's lower (higher priority)? if it's the same NAME
         we treat it as higher priority and won't transmit */
      if( msg->buf[cnt] <= j1939_name[p][cnt] ) {
        
        /* we can't use the current address anymore */
        j1939_address_claim_was_successful[p] = 0;

        /* we now know there is a higher priority CA out there */
        j1939_address_claim_received_higher_priority[p] = 1;

        /* see if the application has a new address for us to try */
        tmp_sa = j1939app_sa_get(p);

        /* 254 or 255 indicate no more addresses are available */
        if( tmp_sa < 254 ) {

          j1939_address_claim_time[p] = 0;
          j1939_address_claim_tx_requested[p] = 1;
          j1939_address_cannot_claim_tx_requested[p] = 0;
          j1939_address_claim_has_been_sent[p] = 0;
          j1939_address_claim_was_successful[p] = 0;
          j1939_address_claim_received_higher_priority[p] = 0;
          j1939_sa[p] = tmp_sa;

        } else {

          /* we failed to claim an address */
          j1939_address_claim_has_failed[p] = 1;

          /* we can not send a 'cannot claim' without first sending a 'claim' */
          if( j1939_address_claim_has_been_sent[p] )
            j1939_address_cannot_claim_tx_requested[p] = 1;
        }

      } else {

        /* we're high priority, so send another address claim message (only
           if we haven't already received a higher priority before this) */
        if( j1939_address_claim_received_higher_priority[p] == 0 )
          j1939_address_claim_tx_requested[p] = 1;
      }

      break;
    }

    /* request */
    case J1939_PGN_REQUEST: {

      /* find out what pgn is being requested */
      rpgn = msg->buf[0] | ((uint32_t)msg->buf[1]<<8UL) |
                           ((uint32_t)msg->buf[2]<<16UL);

      /* if it's a request for address claimed, we handle it here */
      if( rpgn == J1939_PGN_ADDRESS_CLAIMED ) {

         j1939_address_claim_requested[msg->port] = 1;

      } else {

        j1939app_process(msg);
      }

      break;
    }

    /* tp - connection management message */
    case J1939_PGN_TP_CM: 
      j1939tp_post_cm(msg);
      break;

    /* tp - data transfer message */
    case J1939_PGN_TP_DT:
      j1939tp_post_dt(msg);
      break;

    /* anything not handled here is handed to application layer */
    default:
      j1939app_process(msg);
      break;
  }

  return;
}



/*
** This function translates a J1939 message into a CAN frame which
** is then buffered for transmission.
** INPUT: msg - pointer to J1939 message
** RETURN: 0 - success
**         1 - failure
*/
uint8_t
j1939_tx ( j1939_t *msg ) {

  can_t can;
  uint8_t p;
  uint8_t cnt, ret;

  /* CAN port */
  p = msg->port;

  /* is babbling-idiot protection active? */
  if( j1939_bip_tx_is_disabled[p] )
    return 1;

  /* copy over buffer */
  for( cnt = 0; cnt < msg->buf_len; cnt++ )
    can.buf[cnt] = msg->buf[cnt];

  /* get size of data to go out */
  can.buf_len = (uint8_t)msg->buf_len;

  /* we start off by loading the pgn into the CAN frame */
  can.id = msg->pgn;

  /* is the protocol format less than 240, the ps field is the dst */
  if( ((uint16_t)msg->pgn) < ((uint16_t)0xf000) )
    can.id |= msg->dst;

  /* load in the priority */
  can.id |= (((uint32_t)(msg->pri & 0x7)) << 18UL);

  /* make room for source address and load it in */
  can.id <<= 8;
  can.id |= msg->src;

  /* the 31st bit being set indicates its an extended frame message */
  can.id |= (B31);

  /* send it to the CAN buffer/isr routines */
  ret = can_tx(p,&can);

  /* keep track of number of transmits for babble protection */
  if( ret == 0 )
    if( j1939_bip_tx_cnt[p] < J1939_BIP_MSGS_250MS )
      j1939_bip_tx_cnt[p]++;

  return ret;
}



/*
** This function buffers a J1939 message for transmission.  For multi-packet
** messages, *status will be set to J1939TP_INPROCESS during transmission,
** J1939TP_DONE for success, or J1939TP_FAILED for failure.
** INPUT: msg - pointer to J1939 message
**        status - pointer to status variable
** RETURN: 0 - success
**         1 - failure
*/
uint8_t
j1939_tx_app ( j1939_t *msg, uint8_t *status ) {
  
  /* we can not transmit until we have claimed an address */
  if( j1939_address_claim_was_successful[msg->port] == 0 )
	  return 1;

  /* every message has our source address */
  msg->src = j1939_sa[msg->port];

  /* per J1939/21 any global request must also be processed by this ECU.
     also, the only msg that can be less than 8 bytes is a request */
  if( msg->pgn == J1939_PGN_REQUEST ) {

    if( msg->dst == J1939_ADDR_GLOBAL )
      j1939_process(msg);
    
  } else if( msg->buf_len < 8 )
    return 1;

  /* is it single or multi-packet msg? */
  if( msg->buf_len <= 8 )
    return j1939_tx(msg);
  else
    return j1939tp_tx(msg,status);
}



/*
** Buffers an address claimed message for transmission.
** INPUT: src - source address of controlling application
** RETURN: 0 - success
**         1 - failure
*/
uint8_t
j1939_tx_address_claimed ( uint8_t p, uint8_t src ) {

  j1939_t msg;

  /* destination must be global for all address claim msgs. J1939-81 4.2.2 */
  msg.dst = J1939_ADDR_GLOBAL;
  msg.src = src;
  msg.pgn = J1939_PGN_ADDRESS_CLAIMED;
  msg.pri = 6;
  msg.buf = &j1939_name[p][0];
  msg.buf_len = 8;
  msg.port = p;

  return j1939_tx( &msg );
}



/*
** Translates a CAN frame into a J1939 message
*/
void
j1939_post ( uint8_t p, can_t *can ) {

  j1939_t msg;

  /* if EDP is set, or if it's a 2.0a msg, the message should be ignored */
  if( (can->id & B25) || ((can->id & B31) == 0) )
    return;

  /* get source address of frame and chop it off.
     reference SAE J1939/21 for more information */
  msg.src = (uint8_t)can->id;
  can->id >>= 8;

  /* if the protocol formart is less than 240, it is destination specific */
  if( ((uint8_t)(can->id >> 8)) < 240 ) {

    /* destination specific */
    msg.dst = (uint8_t)can->id;
    msg.pgn = can->id & 0x1ff00;

  } else {

    /* ps field is a group extension */
    msg.dst = J1939_ADDR_GLOBAL;
    msg.pgn = can->id & 0x1ffff;
  }

  /* if this packet isn't for us or isn't a broadcast, then ignore */
  if( (msg.dst != j1939_sa[p]) && (msg.dst != J1939_ADDR_GLOBAL) )
    return;

  /* CAN port */
  msg.port = p;
   
  /* J1939_post doesn't own the passed memory so this is ok.  this memory
     is from the interrupt receive buffer so should be quick and dirty.  */
  msg.buf = can->buf;
  msg.buf_len = can->buf_len;

  /* we decoded the CAN frame into the J1939 parts, so process it */
  j1939_process(&msg);
}



/*
** This function updates the address claim state machine.
*/
void
j1939_update_address_claim ( uint8_t p ) {

  uint8_t tmp;

  /* has an address claim been requested? */
  if( j1939_address_claim_requested[p] ) {
   
    /* if we've failed, we send a 'cannot claim', else 'addr claimed' */
    tmp = j1939_address_claim_has_failed[p] ? J1939_ADDR_NULL : j1939_sa[p];

    if( j1939_tx_address_claimed(p,tmp) == 0 )
      j1939_address_claim_requested[p] = 0;
  }

  /* has an address claim message been requested? */
  if( j1939_address_claim_tx_requested[p] &&
      (j1939_tx_address_claimed(p,j1939_sa[p]) == 0) ) {

    j1939_address_claim_tx_requested[p] = 0;
    j1939_address_claim_has_been_sent[p] = 1;
  }

  /* has a cannot claim message been requested? */
  if( j1939_address_cannot_claim_tx_requested[p] &&
      (j1939_tx_address_claimed(p,J1939_ADDR_NULL) == 0) )
    j1939_address_cannot_claim_tx_requested[p] = 0;

  /* we wait 250ms after sending out an address claim message.  if we didn't
     receive a higher priority, we claim the address */
  if( j1939_address_claim_has_been_sent[p] ) {

    if( j1939_address_claim_time[p] <= J1939_ADDRESS_CLAIM_TIME_OUT ) {

      j1939_address_claim_time[p]++;

    } else if( j1939_address_claim_received_higher_priority[p] == 0 ) {

      j1939_address_claim_was_successful[p] = 1;
    }
  }

  return;
}

/*
** This function is the time base for the entire J1939 module.
*/
void
j1939_update ( void ) {
//#ifdef _DANT /* NAI would rather rely on interrupts to process received CAN messages..so we do not perform this here */
  can_t can;
//#endif
  uint8_t p;

  for( p = 0; p < J1939CFG_PORTS_NUM; p++ ) {
	  /* NAI - only attempt to update those ports that are enabled! */
	  if (j1939_enabled_ports[p] == 1)
	  {
//#ifdef _DANT /* NAI would rather rely on interrupts to process received CAN messages..so we do not perform this here */
#ifdef _ORIGINAL_CODE
		/* read all the CAN frames are and pass them up */
		while( can_rx(p,&can) == 0 )
			j1939_post(p,&can);
#endif /*_ORIGINAL_CODE*/
//DANT
		while (can_isRxMsgReady(p))
		{
			can_interruptClear(p, CAN_IXR_RXOK_MASK);
			if ( can_rx(p,&can) == 0 )
				j1939_post(p,&can);
		}
//END DANT
//#endif
		/* update address claim state machine */
		j1939_update_address_claim(p);

		/* update babbling-idiot protection */
		j1939_bip_update(p);

		/* update transport protocol */
		j1939tp_update(p);
	  }
  }

  /* application layer that transmits messages periodically */
  j1939app_update();

  return;
}

/**************************************************************************************/
/* DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)										  */
/**************************************************************************************/
void
j1939_enable_p ( uint8_t p, uint8_t enable ) {
	if (p < J1939CFG_PORTS_NUM)
	{
		j1939_enabled_ports[p] = (enable >= 1 ? 1 : 0);

		if (j1939_enabled_ports[p])
		{
			/* get the source address */
			j1939_sa[p] = j1939app_sa_get(p);

			/* initialize transport protocol for the enabled port */
			j1939tp_init_p(p);

			/* initialize application layer for the specified port */
			j1939app_init_p(p);
		}
	}
}

void
j1939_enable_all_ports ( uint8_t enable ) {
	uint8_t p;
	for( p = 0; p < J1939CFG_PORTS_NUM; p++ ) {
		j1939_enable_p(p, enable);
	}
}

uint8_t
j1939_is_port_enabled ( uint8_t p )	{
	uint8_t enabled = 0;
	if (p < J1939CFG_PORTS_NUM)
		enabled = (j1939_enabled_ports[p] == 1)?1:0;
	return enabled;
}

uint8_t
j1939_is_any_port_enabled ( ) {
	uint8_t p;
	uint8_t enabled = 0;
	for( p = 0; p < J1939CFG_PORTS_NUM; p++ )
	{
		if (j1939_is_port_enabled(p) == 1)
		{
			enabled = 1;
			break;
		}
	}

	return enabled;
}

void
j1939_init_p ( uint8_t p ) {

  /* NAI - only attempt to initialize the port if it is enabled for J1939! */
  if (p < J1939CFG_PORTS_NUM && (j1939_enabled_ports[p] == 1) )
  {
	 /* babbling-idiot protection variables */
	 j1939_bip_time[p] = 0;
	 j1939_bip_tx_cnt[p] = 0;
	 j1939_bip_tx_cnt_max[p] = 0;
	 j1939_bip_tx_is_disabled[p] = 0;
	 j1939_bip_tx_cnt_allowed[p] = J1939_BIP_MSGS_250MS/4U;

	 /* address claim algorithm variables */
	 j1939_address_claim_time[p] = 0;
	 j1939_address_claim_requested[p] = 0;
	 j1939_address_claim_has_failed[p] = 0;
	 j1939_address_claim_tx_requested[p] = 1;
	 j1939_address_claim_has_been_sent[p] = 0;
	 j1939_address_claim_was_successful[p] = 0;
	 j1939_address_cannot_claim_tx_requested[p] = 0;
	 j1939_address_claim_received_higher_priority[p] = 0;

	 /* get the source address */
	 j1939_sa[p] = j1939app_sa_get(p);

	 j1939tp_init_p(p);
  }
}

/*
** This function receives a single j1939 msg and returns it to the caller.
*/
uint8_t
j1939_can_rx (  uint8_t p, j1939_t *msg ) {
  uint8_t status = CAN_SUCCESS;
  can_t can;

  /* read all the CAN frames are and pass them up */
  status = can_rx(p,&can);
  if (status == CAN_SUCCESS)
  {
	  /* translate cant_t msg into j1939_t msg */
	  status = j1939_translate(p, &can, msg);
  }

  return status;
}

/*
** Translates a CAN frame into a J1939 message
*/
uint8_t
j1939_translate ( uint8_t p, can_t *can, j1939_t *msg ) {

  /* if EDP is set, or if it's a 2.0a msg, the message should be ignored */
  if( (can->id & B25) || ((can->id & B31) == 0) )
    return CAN_UNEXPECTED_FORMAT;

  /* get source address of frame and chop it off.
     reference SAE J1939/21 for more information */
  msg->src = (uint8_t)can->id;
  can->id >>= 8;

  /* if the protocol format is less than 240, it is destination specific */
  if( ((uint8_t)(can->id >> 8)) < 240 ) {

    /* destination specific */
    msg->dst = (uint8_t)can->id;
    msg->pgn = can->id & 0x1ff00;

  } else {

    /* ps field is a group extension */
    msg->dst = J1939_ADDR_GLOBAL;
    msg->pgn = can->id & 0x1ffff;
  }

  /* if this packet isn't for us or isn't a broadcast, then ignore */
  if( (msg->dst != j1939_sa[p]) && (msg->dst != J1939_ADDR_GLOBAL) )
    return CAN_MSG_UNINTENDED_RECIPIENT;

  /* CAN port */
  msg->port = p;

  /* J1939_post doesn't own the passed memory so this is ok.  this memory
     is from the interrupt receive buffer so should be quick and dirty.  */
/*DANT - I THINK THIS IS NOT OK - WE NEED TO OWN THE MEMORY!
  msg->buf = can->buf;
*/
  memcpy(msg->buf, can->buf, can->buf_len);

  msg->buf_len = can->buf_len;

  return CAN_SUCCESS;
}


/*
** Disables the babbling idiot protection for all ports
*/
void
j1939_disable_bip_all_ports () {
  uint8_t p;

  for( p = 0; p < J1939CFG_PORTS_NUM; p++ ) {
	/* NAI - only attempt to disable "bip" for those ports that are enabled for J1939! */
	if (j1939_enabled_ports[p] == 1)
	   j1939_bip_tx_cnt_allowed[p] = (J1939_BIP_MSGS_250MS*(uint16_t)100)/100U;
  }

  return;
}

void
j1939_enable_bip_all_ports (uint8_t rate) {
  uint8_t p;

  if( rate <= 100 ) {
	  for( p = 0; p < J1939CFG_PORTS_NUM; p++ ) {
		  /* NAI - only attempt to enable "bip" for those ports that are enabled for J1939! */
		  if (j1939_enabled_ports[p] == 1)
		     j1939_bip_tx_cnt_allowed[p] = (J1939_BIP_MSGS_250MS*(uint16_t)rate)/100U;
	  }
  }

  return;
}

uint8_t
j1939_get_assigned_sa( uint8_t p) {
  return (j1939_sa[p]);
}

uint8_t
j1939_assigned_sa_claim_in_progress( uint8_t p) {
	return (j1939_address_claim_tx_requested[p]);
}

uint8_t
j1939_assigned_sa_claim_failed( uint8_t p) {
	return (j1939_address_claim_has_failed[p]);
}

uint8_t
j1939_assigned_sa_claim_passed( uint8_t p) {
	return (j1939_address_claim_was_successful[p]);
}

/**************************************************************************************/
/* END DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)									  */
/**************************************************************************************/
