/*
 * Test menu for Maximillion modules.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#if SERIAL_DEBUG

#include <includes.h>


/* Global variables */
volatile static bool irq_triggered = FALSE;

/* Local functions */
static void eeprom_menu(void);
static void flash_menu(void);
static void l2c_menu(void);
static void mem_dump(void);
static void reg_write(void);
static void ram_test(void);
static void i2c_bus_probe(void);
static void eeprom_ops(u8 op);
static void flash_ops(u8 op);
static void temp_regs(u8 chip);
static void volt_temp_mon(void);
static void int_test(void);
static void test_irq_handler(void *context);
static void l2c_ops(u8 op, u8 ev0, u8 ev1);
static char pend_rx_char(void);


void test_menu(void)
{
    bool    exit_menu = FALSE;
    char    *inbuf;

    /* Print menu choices */
    while (exit_menu == FALSE)
    {
        puts("\n\n>>>  Main Menu  <<<\n");
        puts("1. Mem/Reg Dump");
        puts("2. Reg Write");
        puts("3. RAM Test");
        puts("4. Probe I2C Bus");
        puts("5. EEPROM Tests");
        puts("6. Flash Tests");
        puts("7. Temp Sensor Regs");
        puts("8. Volt/Temp Monitor");
        puts("9. Interrupt Test");
        puts("0. L2 Cache Events");
        puts("--------------------");
        printf("T. %s Test Menu\n", MODULE_NAME);
        puts("E. Exit test menu");
        puts("R. Reset Module");
        printf("\nEnter Selection: ");
        inbuf = pend_rx_line();
        if (!inbuf || (strlen(inbuf) > 1))
            continue;

        switch (tolower((int)*inbuf))
        {
            case '1':
                mem_dump();
                break;
            case '2':
                reg_write();
                break;
            case '3':
                ram_test();
                break;
            case '4':
                i2c_bus_probe();
                break;
            case '5':
                eeprom_menu();
                break;
            case '6':
                flash_menu();
                break;
            case '7':
                temp_regs(I2C_TEMP_FUNC_MOD_ADDR);
                break;
            case '8':
                volt_temp_mon();
                break;
            case '9':
                int_test();
                break;
            case '0':
                l2c_menu();
                break;
            case 't':
                module_menu();
                break;
            case 'e':
                exit_menu = TRUE;
                puts("\nRunning module 'main' loop...");
                break;
            case 'r':
                cpu_reset();
                break;
        }
    }
}


void eeprom_menu(void)
{
    bool    exit_menu = FALSE;
    char    *inbuf;

    /* Print menu choices */
    while (exit_menu == FALSE)
    {
        puts("\n\n>>>  EEPROM Menu  <<<\n");
        puts("1. Serial Number");
        puts("2. Read");
        puts("3. Erase");
        puts("4. Write");
        puts("---------------------");
        puts("M. Main menu");
        printf("\nEnter Selection: ");
        inbuf = pend_rx_line();
        if (!inbuf || (strlen(inbuf) > 1))
            continue;

        switch (tolower((int)*inbuf))
        {
            case '1':
                eeprom_ops(EEPROM_OPCODE_READ_SN);
                break;
            case '2':
                eeprom_ops(EEPROM_OPCODE_READ);
                break;
            case '3':
                eeprom_ops(EEPROM_OPCODE_ERASE);
                break;
            case '4':
                eeprom_ops(EEPROM_OPCODE_WRITE);
                break;
            case 'm':
                exit_menu = TRUE;
                break;
        }
    }
}


void flash_menu(void)
{
    bool    exit_menu = FALSE;
    char    *inbuf;

    /* Print menu choices */
    while (exit_menu == FALSE)
    {
        puts("\n\n>>>  Flash Menu  <<<\n");
        puts("1. Detect");
        puts("2. Read");
        puts("3. Erase");
        puts("4. Write");
        puts("--------------------");
        puts("M. Main menu");
        printf("\nEnter Selection: ");
        inbuf = pend_rx_line();
        if (!inbuf || (strlen(inbuf) > 1))
            continue;

        switch (tolower((int)*inbuf))
        {
            case '1':
                flash_ops(XQSPIPS_FLASH_OPCODE_RDID);
                break;
            case '2':
                flash_ops(XQSPIPS_FLASH_OPCODE_NORM_READ);
                break;
            case '3':
                flash_ops(XQSPIPS_FLASH_OPCODE_SE);
                break;
            case '4':
                flash_ops(XQSPIPS_FLASH_OPCODE_PP);
                break;
            case 'm':
                exit_menu = TRUE;
                break;
        }
    }
}


void l2c_menu(void)
{
    bool    exit_menu = FALSE;
    char    *inbuf;

    /* Print menu choices */
    while (exit_menu == FALSE)
    {
        puts("\n\n>>>  L2 Cache Menu  <<<\n");
        puts("1. Monitor DRREQ/DRHIT");
        puts("2. Monitor DWREQ/DWHIT");
        puts("3. Monitor IRREQ/IRHIT");
        puts("4. Clear and Start");
        puts("5. Stop and Read");
        puts("-----------------------");
        puts("M. Main menu");
        printf("\nEnter Selection: ");
        inbuf = pend_rx_line();
        if (!inbuf || (strlen(inbuf) > 1))
            continue;

        switch (tolower((int)*inbuf))
        {
            case '1':
                l2c_ops(L2C_OPCODE_CONFIG, XL2CC_DRREQ, XL2CC_DRHIT);
                break;
            case '2':
                l2c_ops(L2C_OPCODE_CONFIG, XL2CC_DWREQ, XL2CC_DWHIT);
                break;
            case '3':
                l2c_ops(L2C_OPCODE_CONFIG, XL2CC_IRREQ, XL2CC_IRHIT);
                break;
            case '4':
                l2c_ops(L2C_OPCODE_START, 0, 0);
                break;
            case '5':
                l2c_ops(L2C_OPCODE_READ, 0, 0);
                break;
            case 'm':
                exit_menu = TRUE;
                break;
        }
    }
}


static void mem_dump(void)
{
    u32     val;
    char    *inbuf;
    static u8   width = 32;
    static u32  start = MODULE_MEM_BASE;
    static u32  count = 1;

    /* Get width */
    printf("\nWidth (8/16/32) [%d]: ", width);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && ((val == 8) || (val == 16) || (val == 32)))
        {
            width = val;
        }
        else
        {
            puts("Valid values are 8, 16, 32");
            pause();
            return;
        }
    }

    /* Get start address */
    printf("Address [%lX]: ", start);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
        {
            start = val;
        }
        else
        {
            puts("Valid values are 0x0 to 0xFFFFFFFF");
            pause();
            return;
        }
    }

    /* Get count */
    printf("Count [%lX]: ", count);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
        {
            count = val;
        }
        else
        {
            puts("Valid values are 0x0 to 0xFFFFFFFF");
            pause();
            return;
        }
    }

    /* Dump it */
    putchar('\n');
    print_buffer(start, (void *)start, width / 8, count);

    pause();
}


static void reg_write(void)
{
    u32     val;
    char    *inbuf;
    static u8   width = 32;
    static u32  addr = MODULE_MEM_BASE;
    static u32  data = 0;

    /* Get width */
    printf("\nWidth (8/16/32) [%d]: ", width);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && ((val == 8) || (val == 16) || (val == 32)))
        {
            width = val;
        }
        else
        {
            puts("Valid values are 8, 16, 32");
            pause();
            return;
        }
    }

    while (1)
    {
        /* Get address */
        printf("Address [%lX]: ", addr);
        inbuf = pend_rx_line();
        if (!inbuf)
        {
            pause();
            return;
        }
        if (strlen(inbuf))
        {
            if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
            {
                addr = val;
            }
            else
            {
                puts("Valid values are 0x0 to 0xFFFFFFFF");
                pause();
                return;
            }
        }

        /* Get value */
        printf("Value [%lX]: ", data);
        inbuf = pend_rx_line();
        if (!inbuf)
        {
            pause();
            return;
        }
        if (strlen(inbuf))
        {
            if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
            {
                data = val;
            }
            else
            {
                puts("Valid values are 0x0 to 0xFFFFFFFF");
                pause();
                return;
            }
        }

        /* Write it */
        if (width == 32)
            Xil_Out32(addr, data);
        else if (width == 16)
            Xil_Out16(addr, data);
        else
            Xil_Out8(addr, data);

        putchar('\n');
    }

    pause();
}


static void ram_test(void)
{
    u32     addr;
    u32     val, temp;
    u32     progress;
    u32     i;
    char    *inbuf;
    char    cval;
    int     vt_i;
    static u8   width = 32;
    static u32  start = MODULE_MEM_BASE;
    static u32  end = MODULE_MEM_BASE + 0xFFC;
    static u32  pattern = 0;
    static char increment = 'y';
    static u32  count = 1;

    /* Get width */
    printf("\nWidth (8/16/32) [%d]: ", width);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && ((val == 8) || (val == 16) || (val == 32)))
        {
            width = val;
        }
        else
        {
            puts("Valid values are 8, 16, 32");
            pause();
            return;
        }
    }

    /* Get start address */
    printf("Start address [%lX]: ", start);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
        {
            start = val;
        }
        else
        {
            puts("Valid values are 0x0 to 0xFFFFFFFF");
            pause();
            return;
        }
    }

    /* Get end address */
    printf("End address [%lX]: ", end);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
        {
            end = val;
        }
        else
        {
            puts("Valid values are 0x0 to 0xFFFFFFFF");
            pause();
            return;
        }
    }
    if (end < start)
    {
        puts("End address must be > start address");
        pause();
        return;
    }

    /* Get pattern */
    printf("Pattern [%lX]: ", pattern);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
        {
            pattern = val;
        }
        else
        {
            puts("Valid values are 0x0 to 0xFFFFFFFF");
            pause();
            return;
        }
    }

    /* Get increment */
    printf("Increment? (y/n) [%c]: ", increment);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isalpha((int)inbuf[0]) && (sscanf(inbuf, "%c", &cval) == 1) && ((tolower((int)cval) == 'y') || (tolower((int)cval) == 'n')))
        {
            increment = tolower((int)cval);
        }
        else
        {
            puts("Valid values are 'y' and 'n'");
            pause();
            return;
        }
    }

    /* Get count */
    printf("Iterations [%ld]: ", count);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1))
        {
            count = val;
        }
        else
        {
            puts("Valid values are 0 for 'infinite' or any decimal value");
            pause();
            return;
        }
    }
    putchar('\n');

    /* Disable data cache */
    Xil_DCacheDisable();

    progress = 0x200000 - (width >> 3);
    for (i = 0; !count || i < count; i++)
    {
        addr = start;
        val = pattern & (0xFFFFFFFF >> (32 - width));
        while (addr <= end && addr >= start)
        {
            /* Write the pattern, read it back and verify */
            if (width == 32)
            {
                Xil_Out32(addr, val);
                temp = Xil_In32(addr);
            }
            else if (width == 16)
            {
                Xil_Out16(addr, val);
                temp = Xil_In16(addr);
            }
            else
            {
                Xil_Out8(addr, val);
                temp = Xil_In8(addr);
            }

            /* Abort on mismatch */
            if (val != temp)
            {
                printf("\n\nData mismatch at 0x%08lX; expected 0x%0*lX, got 0x%0*lX\n", addr, width >> 2, val, width >> 2, temp);
                if_volt_temp_read(XADCPS_CH_TEMP, &vt_i, NULL);
                printf("Core_T=%dC, ", vt_i);
                if_volt_temp_read(XADCPS_CH_VPVN, &vt_i, NULL);
                printf("PCB_T=%dC\n", vt_i);
                goto cleanup;
            }

            /* Display progress */
            if (((addr - start) & progress) == 0)
            {
                putchar('.');

                /* Check for abort */
                if (getchar() > 0)
                {
                    printf("\n\nAborted at 0x%08lX after %lu iteration(s)\n", addr, i + 1);
                    goto cleanup;
                }
            }

            /* Increment address and pattern */
            addr += (width >> 3);
            if (increment == 'y')
                val = (val + 1) & (0xFFFFFFFF >> (32 - width));
        }
    }

    puts("\n\nPASSED");

cleanup:

    /* Re-enable data cache */
    Xil_DCacheEnable();

    pause();
}


static void i2c_bus_probe(void)
{
    u8 chip;

    puts("\nProbing I2C bus...\n");

    printf("Valid chip addresses:");
    for (chip = 0; chip < 128; chip++)
    {
        if (i2c_probe(chip) == XST_SUCCESS)
            printf(" %02X", chip);
    }
    putchar('\n');

    pause();
}


static void eeprom_ops(u8 op)
{
    u32     i;
    u32     val;
    u8      chip = I2C_EEPROM_IF_MOD_ADDR;
    char    *inbuf;
    XStatus rc;
    static u8   buf[EEPROM_PAGE_SIZE];
    static u32  addr = 0;
    static u32  data = 0;

    /* Get chip number */
    printf("\nEEPROM chip # (0/1) [0]: ");
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && (val >= 0) && (val <= 1))
        {
            chip = val ? I2C_EEPROM_FUNC_MOD_ADDR : I2C_EEPROM_IF_MOD_ADDR;
        }
        else
        {
            puts("Valid values are 0 and 1");
            pause();
            return;
        }
    }
    putchar('\n');

    /* Perform chosen operation */
    if (op == EEPROM_OPCODE_READ_SN)
    {
        /* Dump Serial Number */
        rc = i2c_dev_read(chip | 0x08, EEPROM_SN_ADDR, buf, EEPROM_SN_SIZE, TRUE);
        if (rc == XST_SUCCESS)
        {
            printf("EEPROM at 0x%02X S/N: ", chip);
            for (i = 0; i < EEPROM_PAGE_SIZE; i++)
            {
                printf("%02X", buf[i]);
            }
            putchar('\n');
        }
    }
    else if (op == EEPROM_OPCODE_READ)
    {
        /* Dump it */
        for (i = 0; i < (EEPROM_SIZE / EEPROM_PAGE_SIZE); i++)
        {
            rc = i2c_dev_read(chip, i * EEPROM_PAGE_SIZE, buf, EEPROM_PAGE_SIZE, TRUE);
            if (rc)
                break;
            if (print_buffer(i * EEPROM_PAGE_SIZE, (void *)buf, 1, EEPROM_PAGE_SIZE))
                break;
        }
    }
    else if (op == EEPROM_OPCODE_ERASE)
    {
        /* Erase it */
        printf("Erasing... ");
        for (i = 0; i < EEPROM_SIZE; i++)
        {
            buf[0] = 0xFF;
            rc = i2c_dev_write(chip, i, buf, 1, TRUE);
            if (rc)
                break;
        }
        if (rc == XST_SUCCESS)
            puts("Done");
    }
    else if (op == EEPROM_OPCODE_WRITE)
    {
        while (1)
        {
            /* Get address */
            printf("Address [%lX]: ", addr);
            inbuf = pend_rx_line();
            if (!inbuf)
            {
                pause();
                return;
            }
            if (strlen(inbuf))
            {
                if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1) && (val < 0x100))
                {
                    addr = val;
                }
                else
                {
                    puts("Valid values are 0x0 to 0xFF");
                    pause();
                    return;
                }
            }

            /* Get value */
            printf("Value [%lX]: ", data);
            inbuf = pend_rx_line();
            if (!inbuf)
            {
                pause();
                return;
            }
            if (strlen(inbuf))
            {
                if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1) && (val < 0x100))
                {
                    data = val;
                }
                else
                {
                    puts("Valid values are 0x0 to 0xFF");
                    pause();
                    return;
                }
            }
            putchar('\n');

            /* Write it */
            rc = i2c_dev_write(chip, addr, (u8 *)&data, 1, TRUE);
            if (rc)
                break;
        }
    }

    pause();
}


static void flash_ops(u8 op)
{
    u32     addr, count, bytes;
    u32     val;
    char    *inbuf;
    XStatus rc;
    static u8   buf[FLASH_PAGE_SIZE];
    static u32  offset = 0;
    static u32  len = FLASH_PAGE_SIZE;
    static u32  ram_addr = 0x100000;

    if (op == XQSPIPS_FLASH_OPCODE_RDID)
    {
        /* Detect and display flash ID */
        flash_id(buf);

        if (buf[FLASH_ID_MFG] == FLASH_ID_MFG_SPANSION && buf[FLASH_ID_SIZE] == FLASH_ID_SIZE_16MB)
            printf("\nDetected S25FL128S_64K: page = 256 Bytes, sector = 64 KB, size = 16 MB\n");
        else
            printf("\nUnsupported flash ID: 0x%02X 0x%02X 0x%02X\n", buf[1], buf[2], buf[3]);

        pause();
        return;
    }

    /* Get offset */
    printf("\nOffset [%lX]: ", offset);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1) && (val <= FLASH_SIZE))
        {
            offset = val;
        }
        else
        {
            printf("Valid values are 0x0 to 0x%X\n", FLASH_SIZE);
            pause();
            return;
        }
    }

    /* Get length */
    printf("Length [%lX]: ", len);
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1) && (val <= FLASH_SIZE))
        {
            len = val;
        }
        else
        {
            printf("Valid values are 0x0 to 0x%X\n", FLASH_SIZE);
            pause();
            return;
        }
    }

    /* Get RAM address for writes */
    if (op == XQSPIPS_FLASH_OPCODE_PP)
    {
        printf("RAM address [%lX]: ", ram_addr);
        inbuf = pend_rx_line();
        if (!inbuf)
        {
            pause();
            return;
        }
        if (strlen(inbuf))
        {
            if (ishex(inbuf, sizeof(u32)) && (sscanf(inbuf, "%lx", &val) == 1))
            {
                ram_addr = val;
            }
            else
            {
                puts("Valid values are 0x0 to 0xFFFFFFFF");
                pause();
                return;
            }
        }
    }
    putchar('\n');

    /* Perform chosen operation */
    if (op == XQSPIPS_FLASH_OPCODE_NORM_READ)
    {
        /* Dump it */
        addr = offset;
        count = len;
        while (count)
        {
            bytes = (count > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : count;

            rc = flash_read(addr, bytes, buf);
            if (rc)
            {
                printf("flash_read failed: rc = 0x%08X\n", rc);
                pause();
                return;
            }

            if (print_buffer(addr, (void *)buf, 1, bytes))
                break;

            addr += bytes;
            count -= bytes;
        }
    }
    else if (op == XQSPIPS_FLASH_OPCODE_SE)
    {
        /* Erase it */
        rc = flash_erase(offset, len);
        if (rc)
            printf("flash_erase failed: rc = 0x%08X\n", rc);
        else
            printf("Erased 0x%lX bytes at offset 0x%lX\n", len, offset);
    }
    else if (op == XQSPIPS_FLASH_OPCODE_PP)
    {
        /* Write it */
        rc = flash_write(offset, len, (u8 *)ram_addr);
        if (rc)
            printf("flash_write failed: rc = 0x%08X\n", rc);
        else
            printf("Wrote 0x%lX bytes from 0x%lX to offset 0x%lX\n", len, ram_addr, offset);
    }

    pause();
}


static void temp_regs(u8 chip)
{
    s8  temp_i, temp_f;
    u8  buf[2];

    puts("\nTemp sensor registers:");

    /* Capability register */
    i2c_dev_read(chip, TEMP_CAPS_REG, buf, 2, TRUE);
    printf(STR_TEMP_2, "Caps", buf[0], buf[1]);

    /* Configuration register */
    i2c_dev_read(chip, TEMP_CONFIG_REG, buf, 2, TRUE);
    printf(STR_TEMP_2, "Config", buf[0], buf[1]);

    /* T_Upper register */
    i2c_dev_read(chip, TEMP_UPPER_REG, buf, 2, TRUE);
    func_temp_convert(buf, &temp_i, &temp_f);
    printf(STR_TEMP, "T_Upper", buf[0], buf[1], temp_i, temp_f);
    printf(")\n");

    /* T_Lower register */
    i2c_dev_read(chip, TEMP_LOWER_REG, buf, 2, TRUE);
    func_temp_convert(buf, &temp_i, &temp_f);
    printf(STR_TEMP, "T_Lower", buf[0], buf[1], temp_i, temp_f);
    printf(")\n");

    /* T_Crit register */
    i2c_dev_read(chip, TEMP_CRIT_REG, buf, 2, TRUE);
    func_temp_convert(buf, &temp_i, &temp_f);
    printf(STR_TEMP, "T_Crit", buf[0], buf[1], temp_i, temp_f);
    printf(")\n");

    /* Ambient temperature register */
    i2c_dev_read(chip, TEMP_A_REG, buf, 2, TRUE);
    func_temp_convert(buf, &temp_i, &temp_f);
    printf(STR_TEMP, "Temp", buf[0], buf[1], temp_i, temp_f);
    if (buf[0] & TEMP_GTE_CRIT_BIT)
        printf(" >=T_Crit");
    if (buf[0] & TEMP_GT_UPPER_BIT)
        printf(" >T_Upper");
    if (buf[0] & TEMP_LT_LOWER_BIT)
        printf(" <T_Lower");
    printf(")\n");

    /* Manufacturer ID register */
    i2c_dev_read(chip, TEMP_MFG_ID_REG, buf, 2, TRUE);
    printf(STR_TEMP_2, "Mfg ID", buf[0], buf[1]);

    /* Device ID/Revision register */
    i2c_dev_read(chip, TEMP_DEV_ID_REG, buf, 2, TRUE);
    printf(STR_TEMP_1, "Dev ID", buf[0]);
    printf(STR_TEMP_1, "Dev Rev", buf[1]);

    /* Resolution register */
    i2c_dev_read(chip, TEMP_RES_REG, buf, 2, TRUE);
    printf(STR_TEMP_1, "Res", buf[0]);

    pause();
}


static void volt_temp_mon(void)
{
    int  vt_i, vt_f;

    puts("\nFuncM_T  IfceM_T   Core_T VccCore  VccAux  VccDDR");
    puts("------- -------- -------- ------- ------- -------");
    while (1)
    {
        func_temp_read((s8 *)&vt_i, (s8 *)&vt_f);
        printf("%3d.%02dC ", (s8)vt_i, (s8)vt_f);

        if_volt_temp_read(XADCPS_CH_VPVN, &vt_i, &vt_f);
        printf("%3d.%03dC ", vt_i, vt_f);

        if_volt_temp_read(XADCPS_CH_TEMP, &vt_i, &vt_f);
        printf("%3d.%03dC ", vt_i, vt_f);

        if_volt_temp_read(XADCPS_CH_VCCPINT, &vt_i, &vt_f);
        printf("%2d.%03dV ", vt_i, vt_f);

        if_volt_temp_read(XADCPS_CH_VCCPAUX, &vt_i, &vt_f);
        printf("%2d.%03dV ", vt_i, vt_f);

        if_volt_temp_read(XADCPS_CH_VCCPDRO, &vt_i, &vt_f);
        printf("%2d.%03dV\r", vt_i, vt_f);

        sleep(1);
        if (getchar() > 0)
            break;
    }
    putchar('\n');

    pause();
}


//#define TIME_IRQ_LATENCY
static void int_test(void)
{
    u32     val;
    char    *inbuf;
    u32     int_id = 32;
    u32     reg, bit;
    XTime   time;
    XStatus rc;

    /* Get IRQ number */
    printf("\nIRQ number (32-92) [32]: ");
    inbuf = pend_rx_line();
    if (!inbuf)
    {
        pause();
        return;
    }
    if (strlen(inbuf))
    {
        if (isdigit((int)inbuf[0]) && (sscanf(inbuf, "%lu", &val) == 1) && (val >= 32) && (val <= 92))
        {
            int_id = val;
        }
        else
        {
            puts("Valid values are 32 to 92");
            pause();
            return;
        }
    }

#ifdef TIME_IRQ_LATENCY
    // GPIO54 as output; low: LED5 = ON
    Xil_Out32(0xE000A284, 0x1);
    Xil_Out32(0xE000A288, 0x1);
    Xil_Out32(0xE000A048, 0x0);
#endif

    /* Register IRQ handler */
    rc = irq_register(int_id, test_irq_handler, &int_id);
    if (rc)
        printf("irq_register() failed with rc = %d\n", rc);

#ifdef TIME_IRQ_LATENCY
    // GPIO54 high: LED5 = OFF
    Xil_Out32(0xE000A048, 0x1);
#endif

    /* Trigger IRQ */
    irq_triggered = FALSE;
    reg = int_id >> 5;
    bit = int_id & 31;
    XScuGic_WriteReg(XPAR_SCUGIC_DIST_BASEADDR, XSCUGIC_PENDING_SET_OFFSET + (reg * 4), 1 << bit);

    /* Wait for IRQ to trigger */
    time = get_timer(0);
    while (!irq_triggered && get_timer(time) < 10)
        ;

    if (irq_triggered)
        printf("\nInterrupt %ld triggered\n", int_id);
    else
        printf("\nTimed out waiting for interrupt %ld to trigger\n", int_id);

    pause();
}


static void test_irq_handler(void *context)
{
    u32 int_id = *(u32 *)context;
    u32 reg = int_id >> 5;
    u32 bit = int_id & 31;

#ifdef TIME_IRQ_LATENCY
    // GPIO54 low: LED5 = ON
    Xil_Out32(0xE000A048, 0x0);
#endif

    irq_triggered = TRUE;

    /* Clear it */
    XScuGic_WriteReg(XPAR_SCUGIC_DIST_BASEADDR, XSCUGIC_PENDING_CLR_OFFSET + (reg * 4), 1 << bit);

    /* Disable it */
    XScuGic_WriteReg(XPAR_SCUGIC_DIST_BASEADDR, XSCUGIC_DISABLE_OFFSET + (reg * 4), 1 << bit);
}


static void l2c_ops(u8 op, u8 ev0, u8 ev1)
{
    u32     reqs, hits;
    static  bool config = FALSE;
    static  bool started = FALSE;

    /* Perform chosen operation */
    if (op == L2C_OPCODE_CONFIG)
    {
        /* Configure event counters */
        XL2cc_EventCtrInit(ev0, ev1);
        config = TRUE;

        printf("\nL2 cache events configured\n");
    }
    else if (op == L2C_OPCODE_START)
    {
        /* Start counters */
        if (config)
        {
            XL2cc_EventCtrStart();
            started = TRUE;

            printf("\nL2 cache events started\n");
        }
        else
        {
            printf("\nERROR: L2 cache events not configured!\n");
        }
    }
    else if (op == L2C_OPCODE_READ)
    {
        /* Read counters */
        if (started)
        {
            XL2cc_EventCtrStop(&reqs, &hits);
            started = FALSE;

            printf("\nL2 cache events:\n");
            printf("  Requests:      %10lu\n", reqs);
            printf("  Hits:          %10lu\n", hits);
            printf("  Misses:        %10lu\n", reqs - hits);
            printf("  Hit/Req ratio: %10lu%%\n", 100 * hits / reqs);
        }
        else
        {
            printf("\nERROR: L2 cache events not started!\n");
        }
    }

    pause();
}


char pend_rx_char(void)
{
    int c;

    while ((c = getchar()) <= 0)
        ;

    return c;
}


char *pend_rx_line(void)
{
    static char line_buf[LINE_BUF_SIZE];
    char    *ptr;
    char    c;
    int     i;

    for (i = 1, ptr = line_buf; i < LINE_BUF_SIZE; i++)
    {
        c = pend_rx_char();
        switch (c)
        {
            case ENTER:
                *ptr++ = 0;
                putchar('\n');
                return line_buf;

            case ESCAPE:
                line_buf[0] = 0;
                putchar('\n');
                return 0;

            case BACKSPACE:
                /* Remove previous char, if there */
                if (ptr > line_buf)
                {
                    i   -= 2;
                    ptr -= 1;
                    putchar(BACKSPACE);
                    putchar(SPACE);
                    putchar(BACKSPACE);
                }
                else
                    --i;
                break;

            default:
                *ptr++ = c;
                putchar(c);
        }
    }

    /* Only get here if entered line is longer than line buffer */
    line_buf[LINE_BUF_SIZE - 1] = 0;
    putchar('\n');

    return line_buf;
}


void pause(void)
{
    printf("\nPress any key to continue...");
    pend_rx_char();
    putchar('\n');
}


bool ishex(char *str, int hex_bytes)
{
    int     len;
    int     i;
    char    *p = str;

    /* Strip "0x", if prepended */
    if ((str[0] == '0') && (tolower((int)str[1]) == 'x'))
        p += 2;

    len = strlen(str);

    /* We're looking for a 1-X char hex value */
    if ((len > 0) && (len <= (hex_bytes << 1)))
    {
        for (i = 0; i < len; ++i)
        {
            if (isxdigit((int)p[i]) == 0)
                break;
        }
    }

    return (i == len);
}

#endif // #if SERIAL_DEBUG

