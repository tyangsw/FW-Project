/*
 * CBx module specific definitions.
 *
 * Copyright (C) 2014 North Atlantic Industries, Inc.
 */

#ifndef __MODULE_H__
#define __MODULE_H__

#define _ALL_CHANNELS 1
//#define _VERBOSE 1

/*
 * Module memory layout
 */
#define MODULE_MEM_MAP_REV  1
#define SYS_REGS_BASE       MODULE_REGS_BASE

/*
 * Are parameter and/or calibration files required for this module?
 */
#define PARAM_FILE_REQUIRED     0
#define CAL_FILE_REQUIRED       0

/*
 * Is programmable CPLD 1 implemented on this module?
 * Should it be programmed late? (after module specific initialization)
 */
#define CPLD1_IMPLEMENTED       0
#define CPLD1_PROGRAM_LATE      0

/*
 * Is programmable CPLD 2 implemented on this module?
 * Should it be programmed late? (after module specific initialization)
 */
#define CPLD2_IMPLEMENTED       0
#define CPLD2_PROGRAM_LATE      0

#define MIN_FRAME_LENGTH		5

typedef enum
{
    CHANNEL_1       = 0,
    CHANNEL_2		= 1,
#ifdef _ALL_CHANNELS
    CHANNEL_3		= 2,
    CHANNEL_4		= 3,
    CHANNEL_5		= 4,
    CHANNEL_6		= 5,
    CHANNEL_7		= 6,
    CHANNEL_8		= 7,
    NUM_CHANNELS	= 8,
#endif
    ALL_CHANNELS	= 256
} CHANNEL;

//---------------------------------------------
// 		Defines for the Bosch Core
//---------------------------------------------
#define MAIN_IF             1
#define INTER_IF            2

//NOTE MUST MATCH VALUES FOUND IN naibrd_can.h (naibrd library)
#define MAX_CHANNEL_FILTER_COUNT		4
#define FILTCTRL_ENABLE_FILTER_MASK 			0x00000007  //1st 3 bits of register indicate how many filters are enabled (MIN is 0) and (MAX is 4)
#define FILTCTRL_EDIT_FILTER_MASK				0x00000008 // 4th bit (0x8) indicates whether filter is currently being modified (1=modifying, 0=apply/execute filter)
#define FILTCTRL_AF1_ENABLE_ACC_CODE_AND_MASK  	0x00000010
#define FILTCTRL_AF1_ENABLE_AB_FILT				0x00000020
#define FILTCTRL_AF1_ENABLE_SRR_FILT			0x00000040
#define FILTCTRL_AF1_ENABLE_RTR_FILT			0x00000080
#define FILTCTRL_AF1_STDEXT_FILT				0x00000100
#define FILTCTRL_AF1_SRR_FILT					0x00000200
#define FILTCTRL_AF1_RTR_FILT					0x00000400

#define FILTCTRL_AF2_ENABLE_ACC_CODE_AND_MASK  	0x00000800
#define FILTCTRL_AF2_ENABLE_AB_FILT				0x00001000
#define FILTCTRL_AF2_ENABLE_SRR_FILT			0x00002000
#define FILTCTRL_AF2_ENABLE_RTR_FILT			0x00004000
#define FILTCTRL_AF2_STDEXT_FILT				0x00008000
#define FILTCTRL_AF2_SRR_FILT					0x00010000
#define FILTCTRL_AF2_RTR_FILT					0x00020000

#define FILTCTRL_AF3_ENABLE_ACC_CODE_AND_MASK  	0x00040000
#define FILTCTRL_AF3_ENABLE_AB_FILT				0x00080000
#define FILTCTRL_AF3_ENABLE_SRR_FILT			0x00100000
#define FILTCTRL_AF3_ENABLE_RTR_FILT			0x00200000
#define FILTCTRL_AF3_STDEXT_FILT				0x00400000
#define FILTCTRL_AF3_SRR_FILT					0x00800000
#define FILTCTRL_AF3_RTR_FILT					0x01000000

#define FILTCTRL_AF4_ENABLE_ACC_CODE_AND_MASK  	0x02000000
#define FILTCTRL_AF4_ENABLE_AB_FILT				0x04000000
#define FILTCTRL_AF4_ENABLE_SRR_FILT			0x08000000
#define FILTCTRL_AF4_ENABLE_RTR_FILT			0x10000000
#define FILTCTRL_AF4_STDEXT_FILT				0x20000000
#define FILTCTRL_AF4_SRR_FILT					0x40000000
#define FILTCTRL_AF4_RTR_FILT					0x80000000

#define FILTCTRL_ENABLE_ACC_CODE_AND_MASK(n) (n == 1 ? FILTCTRL_AF1_ENABLE_ACC_CODE_AND_MASK : (n == 2 ? FILTCTRL_AF2_ENABLE_ACC_CODE_AND_MASK : (n == 3 ? FILTCTRL_AF3_ENABLE_ACC_CODE_AND_MASK : (n == 4 ? FILTCTRL_AF4_ENABLE_ACC_CODE_AND_MASK : 0))))
#define FILTCTRL_ENABLE_AB_FILT(n) (n == 1 ? FILTCTRL_AF1_ENABLE_AB_FILT : (n == 2 ? FILTCTRL_AF2_ENABLE_AB_FILT : (n == 3 ? FILTCTRL_AF3_ENABLE_AB_FILT : (n == 4 ? FILTCTRL_AF4_ENABLE_AB_FILT : 0))))
#define FILTCTRL_ENABLE_SRR_FILT(n) (n == 1 ? FILTCTRL_AF1_ENABLE_SRR_FILT : (n == 2 ? FILTCTRL_AF2_ENABLE_SRR_FILT : (n == 3 ? FILTCTRL_AF3_ENABLE_SRR_FILT : (n == 4 ? FILTCTRL_AF4_ENABLE_SRR_FILT : 0))))
#define FILTCTRL_ENABLE_RTR_FILT(n) (n == 1 ? FILTCTRL_AF1_ENABLE_RTR_FILT : (n == 2 ? FILTCTRL_AF2_ENABLE_RTR_FILT : (n == 3 ? FILTCTRL_AF3_ENABLE_RTR_FILT : (n == 4 ? FILTCTRL_AF4_ENABLE_RTR_FILT : 0))))
#define FILTCTRL_STDEXT_FILT(n) (n == 1 ? FILTCTRL_AF1_STDEXT_FILT : (n == 2 ? FILTCTRL_AF2_STDEXT_FILT : (n == 3 ? FILTCTRL_AF3_STDEXT_FILT : (n == 4 ? FILTCTRL_AF4_STDEXT_FILT : 0))))
#define FILTCTRL_SRR_FILT(n) (n == 1 ? FILTCTRL_AF1_SRR_FILT : (n == 2 ? FILTCTRL_AF2_SRR_FILT : (n == 3 ? FILTCTRL_AF3_SRR_FILT : (n == 4 ? FILTCTRL_AF4_SRR_FILT : 0))))
#define FILTCTRL_RTR_FILT(n) (n == 1 ? FILTCTRL_AF1_RTR_FILT : (n == 2 ? FILTCTRL_AF2_RTR_FILT : (n == 3 ? FILTCTRL_AF3_RTR_FILT : (n == 4 ? FILTCTRL_AF4_RTR_FILT : 0))))

//------------------------------------------------
// Control Register Values
//------------------------------------------------
#define CTRL_CONFIG_TX             		0x00000001
#define CTRL_CONFIG_RX             		0x00000004
#define CTRL_CONFIG_RETRIEVE_ADDRESS	0x00001000
#define CTRL_CONFIG_ASSIGN_ADDRESS		0x00002000
#define CTRL_CONFIG_BIT					0x00004000
#define CTRL_RESET              		0x00008000

// --- Defines for Protocol
#define CTRL_CONFIG_PROTOCOL_MASK		0x000000F0  /* Bits 4 - 7 define what protocol should be enabled for the given channel */
#define CTRL_CONFIG_ENABLE_AB			0x00000010
#define CTRL_CONFIG_ENABLE_J1939		0x00000020

#define CAN_ADDR_CLAIM                  0x00008000
#define CAN_ADDR_CLAIM_OK               0x00004000
#define CAN_ADDR_CLAIM_FAIL             0x00002000

// --- Defines for FIFO Flags ---
#define START_FLAG              		0x00008000
#define END_FLAG                		0x00004000

struct RX_CONTROL
{
   u32 ctrl;
   u32 filterCtrl;
   u32 ac_msk[MAX_CHANNEL_FILTER_COUNT];
   u32 ac_code[MAX_CHANNEL_FILTER_COUNT];
};

#define STATUS_RX_FIFO_ALMOST_FULL_MASK  	0x00000001
#define STATUS_TX_FIFO_ALMOST_EMPTY_MASK 	0x00000002
#define STATUS_RX_FIFO_HIGH_WATERMARK_MASK	0x00000004
#define STATUS_RX_FIFO_LOW_WATERMARK_MASK  	0x00000008
#define STATUS_RX_FIFO_EMPTY_MASK			0x00000010
#define STATUS_TX_FIFO_FULL_MASK			0x00000020

/* These functions should be available regardless of CB module type */
void can_enable_interrupts();
void can_disable_interrupts();
bool can_set_baud_to_250(CHANNEL ch);
bool can_set_baud_to_500(CHANNEL ch);
bool can_self_test(CHANNEL ch);
bool can_reset_channel_test(CHANNEL ch);
void can_force_interrupt_on_channel(CHANNEL ch);
void can_dump_bit_timing(CHANNEL ch);
void can_dump_error_status(CHANNEL ch);
void can_dump_filter_config(CHANNEL ch);

#if (MODULE_CB2 || MODULE_CB3)
	/* Functions implemented in module_main to be exposed to outside */
	bool canj1939_tx_test(CHANNEL ch);
	bool canj1939_tx_flood_test(CHANNEL ch, int nMsgCount);
	bool canj1939_rx_test(CHANNEL ch);
	bool canj1939_filter_test(CHANNEL ch);
#endif

#if (MODULE_CB1 || MODULE_CB3)
	/* CAN AB Functions */
	bool canA_host_tx_test(CHANNEL ch);
	bool canA_host_rx_test(CHANNEL ch);
	bool canB_host_tx_test(CHANNEL ch);
	bool canB_host_rx_test(CHANNEL ch);
	bool canAB_filter_test(CHANNEL ch);
	bool canB_host_tx_flood_test(CHANNEL ch, int nMsgCount);
	bool canAB_module_round_trip_test(CHANNEL ch, int nMsgCount);
	bool can_set_baud_to_1Mb(CHANNEL ch);
#endif

#ifdef _NEED_MORE_DEBUG_INFO
	void can_fifo_tx_test(CHANNEL ch);
#endif

#endif /* __MODULE_H__ */

