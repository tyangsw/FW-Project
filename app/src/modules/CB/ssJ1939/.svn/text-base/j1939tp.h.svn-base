#ifndef J1939TP_H
#define J1939TP_H



/* error codes for destination specific TP 
   messages.  which are stored at *status */
#define J1939TP_INPROCESS                    255
#define J1939TP_DONE                           0
#define J1939TP_FAILED                         1



extern void
j1939tp_init ( void );

extern uint8_t
j1939tp_tx ( j1939_t *msg, uint8_t *status );

extern void
j1939tp_post_cm ( j1939_t *msg );

extern void
j1939tp_post_dt ( j1939_t *msg );

extern void
j1939tp_update ( uint8_t p );

/**************************************************************************************/
/* DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)										  */
/**************************************************************************************/
extern void
j1939tp_init_p ( uint8_t p );
/**************************************************************************************/
/* END DANT ADDED (NORTH ATLANTIC INDUSTRIES, INC.)									  */
/**************************************************************************************/
#endif
