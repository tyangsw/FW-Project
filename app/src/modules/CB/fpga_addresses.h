//////////////////////////////////////////////////////////////
//															//
//	Module  Name:	fpgaAddrs.h								//
//	Initial Date:	10-24-06								//
//	Description:	For all dtos							//
//															//
//////////////////////////////////////////////////////////////
//============= FPGA addresses ==========
#define _FPGA_IMPLEMENTED 1

#define FPGA_Base_Address						0x60000000
#define FPGA_ADDRESS							(FPGA_Base_Address + 0x1000)

#define FPGA_CHANNEL_DEDICATED_SIZE	 0x0020 //32 Registers per channel

//---------------------------------------------
// 			Local register access
//---------------------------------------------
#define FPGA_CH1_DDR_TX_FIFO_START_ADDR			(FPGA_ADDRESS + 0x000)
#define FPGA_CH1_DDR_TX_FIFO_FULL_COUNT			(FPGA_ADDRESS + 0x004)
#define FPGA_CH1_DDR_RX_FIFO_START_ADDR			(FPGA_ADDRESS + 0x008)
#define FPGA_CH1_DDR_RX_FIFO_FULL_COUNT			(FPGA_ADDRESS + 0x00C)

//#define FPGA_TBD_1							FPGA_ADDRESS + 0x010		// RESERVED
//#define FPGA_TBD_2							FPGA_ADDRESS + 0x014		// RESERVED
//#define FPGA_TBD_3							FPGA_ADDRESS + 0x018		// RESERVED
//#define FPGA_TBD_4							FPGA_ADDRESS + 0x01C		// RESERVED

#define FPGA_FIFO_CONTROL						(FPGA_ADDRESS + 0x100)
#define FPGA_PS_ENABLE							(FPGA_ADDRESS + 0x104)		// Power Supply Enable
#define FPGA_HPS_CONTROL						(FPGA_ADDRESS + 0x108)
#define FPGA_INTERRUPT_STATUS					(FPGA_ADDRESS + 0x10C)
#define FPGA_BIT_STATUS							(FPGA_ADDRESS + 0x110)
