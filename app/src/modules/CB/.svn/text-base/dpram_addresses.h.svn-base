///////////////////////////////////////////////////////////
//                                                       //
//  Initial Date: 	03-06-14							 //
//                                                       //
///////////////////////////////////////////////////////////
//============= DPRAM addresses ==========
//#define DPRAM_Base_Address						0x43C00000
#define DPRAM_Base_Address						0x40000000

#define DPRAM_ADDRESS							(DPRAM_Base_Address + 0x1000)

#define CHANNEL_DEDICATED_SIZE	 				0x0080 // 128 Bytes
#define CHANNEL_FILTER_PAIR_ADDRESS_SKIP		0x0008 // Mask and Code registers are stored in pairs so if interested in Mask for filter1 and filter2 user must skip the register address for Code to get to the next Mask register.

#define DPRAM_CH1_CONTROL                       (DPRAM_ADDRESS + 0x000)     // Config for Interface
#define DPRAM_CH1_RX_DATA                       (DPRAM_ADDRESS + 0x004)     // FIFO for RX data.
#define DPRAM_CH1_RX_USED                       (DPRAM_ADDRESS + 0x008)     // Size of data received from CAN bus
#define DPRAM_CH1_RX_FRAME_CNT                  (DPRAM_ADDRESS + 0x00C)     // Number of frames in RX FIFO
#define DPRAM_CH1_TX_DATA                       (DPRAM_ADDRESS + 0x010)     // FIFO for TX data.
#define DPRAM_CH1_TX_USED                       (DPRAM_ADDRESS + 0x014)     // Size of data to tx to CAN bus
#define DPRAM_CH1_TX_FRAME_CNT                  (DPRAM_ADDRESS + 0x018)     // Number of frames in TX FIFO
//#define RESERVED								(DPRAM_ADDRESS + 0x01C)		// Reserved for Future Use! OLD Acceptance Mask
//#define RESERVED								(DPRAM_ADDRESS + 0x020)		// Reserved for Future Use! OLD Acceptance Code
#define DPRAM_CH1_LAST_ERR_CODE                 (DPRAM_ADDRESS + 0x024)     // Holds type of status register caller needs to consult
#define DPRAM_CH1_COMM_STATUS                   (DPRAM_ADDRESS + 0x028)     // Holds communication status (typically NAI detected status)
#define DPRAM_CH1_BAUD_TIMING					(DPRAM_ADDRESS + 0x02C)		//
//#define DPRAM_CH1_BAUD_PRESC					(DPRAM_ADDRESS + 0x00E)		//
#define DPRAM_CH1_CERC							(DPRAM_ADDRESS + 0x030)		//
#define DPRAM_CH1_TX_FIFO_SIZE					(DPRAM_ADDRESS + 0x034)		// FUTURE
#define DPRAM_CH1_RX_FIFO_SIZE					(DPRAM_ADDRESS + 0x038)		// FUTURE
#define DPRAM_CH1_TX_ALMOST_EMPTY				(DPRAM_ADDRESS + 0x03C)		//
#define DPRAM_CH1_RX_ALMOST_FULL				(DPRAM_ADDRESS + 0x040)		//
#define DPRAM_CH1_RX_HIGH_WATER_MARK			(DPRAM_ADDRESS + 0x044)		//
#define DPRAM_CH1_RX_LOW_WATER_MARK				(DPRAM_ADDRESS + 0x048)		//
#define DPRAM_CH1_FIFO_STATUS					(DPRAM_ADDRESS + 0x04C)		//
#define DPRAM_CH1_MSG_DROP_COUNT				(DPRAM_ADDRESS + 0x050)		//
#define DPRAM_CH1_BUS_STATUS					(DPRAM_ADDRESS + 0x054)		// Holds bus status (i.e. direct copy of ESR register of CAN)
#define DPRAM_CH1_CORE_STATUS					(DPRAM_ADDRESS + 0x058)		// Holds core status (i.e. direct copy of SR register of CAN)
//#define RESERVED								(DPRAM_ADDRESS + 0x05C)		// Reserved for Future Use!
#define DPRAM_CH1_SOURCE_ADDRESS				(DPRAM_ADDRESS + 0x060)		// Holds the source address to be assigned for the channel (J1939 - CAN/B Only)


// Filtering Registers
#define DPRAM_CH1_FILTER_CONTROL				(DPRAM_ADDRESS + 0x400)     // Filter control - bitmapped register of filter enables
//#define RESERVED								(DPRAM_ADDRESS + 0x404)		// Reserved for Future Use!
#define DPRAM_CH1_ACPT_MASK                  	(DPRAM_ADDRESS + 0x408)     // (MAX 4 MASKS - 0x408, 0x410, 0x418, 0x420)
#define DPRAM_CH1_ACPT_CODE                  	(DPRAM_ADDRESS + 0x40C)     // (MAX 4 CODES - 0x40C, 0x414, 0x41C, 0x424)

#define DPRAM_FACTORY_DIAG					    (DPRAM_ADDRESS + 0x800)		// Factory Diagnostic Mode
