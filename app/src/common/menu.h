/*
 * Menu system definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __MENU_H__
#define __MENU_H__

/*
 * Definitions
 */
#define LINE_BUF_SIZE   16

#define BACKSPACE       0x08
#define ENTER           0x0D
#define ESCAPE          0x1B
#define SPACE           0x20

#define STR_TEMP_1      "  %-8s: 0x%02X\n"
#define STR_TEMP_2      "  %-8s: 0x%02X%02X\n"
#define STR_TEMP        "  %-8s: 0x%02X%02X (%d.%02dC"

typedef enum
{
    EEPROM_OPCODE_READ_SN = 0,
    EEPROM_OPCODE_READ,
    EEPROM_OPCODE_ERASE,
    EEPROM_OPCODE_WRITE
} EEPROM_OPS;

typedef enum
{
    L2C_OPCODE_CONFIG = 0,
    L2C_OPCODE_START,
    L2C_OPCODE_READ
} L2C_OPS;

/*
 * Function prototypes
 */
char *pend_rx_line(void);
void pause(void);
bool ishex(char *str, int hex_bytes);
void test_menu(void);
void module_menu(void);

#endif /* __MENU_H__ */

