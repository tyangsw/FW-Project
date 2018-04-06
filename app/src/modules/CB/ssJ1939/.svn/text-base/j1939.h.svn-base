#ifndef J1939_H
#define J1939_H
#include <stdint.h>
#include "j1939cfg.h"

/**************************************************************************************/
/* DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)										  */
/**************************************************************************************/
#include "can.h"
/**************************************************************************************/
/* END DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)									  */
/**************************************************************************************/

/* SAE J1939 Source Addresses found in J1939 p. 45. 252 is 
   for experimental use */
#define J1939_ADDR_DIAG_TOOL1                         249
#define J1939_ADDR_EXPERIMENTAL_USE                   252
#define J1939_ADDR_NULL                               254
#define J1939_ADDR_GLOBAL                             255

/* SAE J1939 parameter group numbers */
#define J1939_PGN_REQUEST                           59904
#define J1939_PGN_ADDRESS_CLAIMED                   60928
#define J1939_PGN_PROPRIETARY_A                     61184
#define J1939_PGN_TP_CM                             60416
#define J1939_PGN_TP_DT                             60160


typedef struct {

  uint8_t aac;          /* 1-bit Arbitrary Address Capable */
  uint8_t ind_grp;      /* 3-bit Industry Group */
  uint8_t veh_sys_inst; /* 4-bit Vehicle System Instance */
  uint8_t veh_sys;      /* 7-bit Vehicle System */ 
  uint8_t func;         /* 8-bit Function */
  uint8_t func_inst;    /* 5-bit Function Instance */
  uint8_t ecu_inst;     /* 3-bit ECU Instance */
  uint16_t mfg_code;    /* 11-bit Manufacturer Code */
  uint32_t identy_num;  /* 21-bit Identity Number */

} j1939_name_t;


typedef struct {

  uint32_t pgn;     /* parameter group number */
  uint8_t *buf;     /* pointer to data */
  uint16_t buf_len; /* size of data */
  uint8_t dst;      /* destination of message */
  uint8_t src;      /* source of message */
  uint8_t pri;      /* priority of message */
  uint8_t port;     /* CAN port of message */

} j1939_t;


extern void
j1939_init( void );

extern void
j1939_update( void );

extern uint8_t
j1939_bip_tx_rate_max_get ( uint8_t p );

extern void
j1939_name_set( uint8_t p, j1939_name_t *n );

extern uint8_t
j1939_tx_app( j1939_t *msg, uint8_t *status );

extern void
j1939_bip_tx_rate_allowed_set( uint8_t p, uint8_t rate );

/**************************************************************************************/
/* DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)										  */
/**************************************************************************************/
extern void
j1939_enable_p ( uint8_t p, uint8_t enable );

extern void
j1939_enable_all_ports ( uint8_t enable );

extern uint8_t
j1939_is_port_enabled ( uint8_t p );

extern uint8_t
j1939_is_any_port_enabled ( void );

extern void
j1939_init_p ( uint8_t p );

extern uint8_t
j1939_can_rx (  uint8_t p, j1939_t *msg );

extern uint8_t
j1939_translate ( uint8_t p, can_t *can, j1939_t *msg );

extern void
j1939_disable_bip_all_ports ( void );

extern void
j1939_enable_bip_all_ports( uint8_t rate );

extern uint8_t
j1939_get_assigned_sa( uint8_t p);

extern uint8_t
j1939_assigned_sa_claim_in_progress( uint8_t p);

extern uint8_t
j1939_assigned_sa_claim_failed( uint8_t p);

extern uint8_t
j1939_assigned_sa_claim_passed( uint8_t p);

extern void
j1939_update_address_claim ( uint8_t p );

/**************************************************************************************/
/* END DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)									  */
/**************************************************************************************/
#endif
