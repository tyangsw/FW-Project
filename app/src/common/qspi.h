/*
 * QSPI flash support definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __QSPI_H__
#define __QSPI_H__

/*
 * Definitions
 */
#define FLASH_ID_MFG_SPANSION   0x01        // Spansion
#define FLASH_ID_SIZE_16MB      0x18        // S25FL128S_64K

#define FLASH_SIZE              (1 << FLASH_ID_SIZE_16MB)
#define FLASH_SECTOR_SIZE       0x10000     // 64 KB
#define FLASH_PAGE_SIZE         256

#define FLASH_COMMAND           0           // Flash command
#define FLASH_ADDRESS_MSB       1           // MSB byte of address to read/write/erase
#define FLASH_ADDRESS_MID       2           // Middle byte of address to read/write/erase
#define FLASH_ADDRESS_LSB       3           // LSB byte of address to read/write/erase
#define FLASH_DATA              4           // Start of data for read/write
#define FLASH_CMD_SIZE          4           // Extra bytes (non data) sent to flash

#define FLASH_ID_MFG            1           // Read ID response byte 1 = manufacturer
#define FLASH_ID_SIZE           3           // Read ID response byte 3 = size

#define FLASH_STATUS            1           // Read status response
#define FLASH_STATUS_BUSY       (1 << 0)    // Bit 0: WIP - device busy

#define READ_ID_CMD_SIZE        4           // Read ID command + 3 byte response
#define WRITE_EN_CMD_SIZE       1           // Write enable command
#define WRITE_CMD_SIZE          4           // Write command + 3 byte address
#define ERASE_CMD_SIZE          4           // Sector erase command + 3 byte address
#define READ_CMD_SIZE           4           // Read command + address
#define READ_STATUS_CMD_SIZE    2           // Read status command + 1 byte response

#define READ_CMD_TIMEOUT        2           // 2 ms (~40 us typical)
#define WRITE_CMD_TIMEOUT       2           // 2 ms (750 us max)
#define ERASE_CMD_TIMEOUT       11000       // 11 s (10,400 ms max for top/bottom 4KB boot sectors)

/*
 * Function prototypes
 */
XStatus qspi_init(void);
XStatus flash_id(u8 *buf);
XStatus flash_read(u32 offset, u32 len, u8 *buf);
XStatus flash_erase(u32 offset, u32 len);
XStatus flash_write(u32 offset, u32 len, u8 *buf);

#endif /* __QSPI_H__ */

