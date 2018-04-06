/*
 * QSPI flash support functions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */
 
#include <includes.h>


/* Global variables */
static XQspiPs QspiInstance;
static u8 lbuf[FLASH_CMD_SIZE + FLASH_PAGE_SIZE];

/* Local functions */
static XStatus flash_wait(XTime timeout);


XStatus qspi_init(void)
{
    XQspiPs_Config  *cfg = NULL;
    XStatus         rc;

    /* Lookup QSPI configuration */
    cfg = XQspiPs_LookupConfig(XPAR_XQSPIPS_0_DEVICE_ID);
    if (!cfg)
        return XST_FAILURE;

    /* Initialize QSPI driver */
    memset(&QspiInstance, 0, sizeof(QspiInstance));
    rc = XQspiPs_CfgInitialize(&QspiInstance, cfg, cfg->BaseAddress);
    if (rc)
        return rc;

    /* Set I/O mode, manual start, manual chip select, and drive HOLD_B high */
    XQspiPs_SetOptions(&QspiInstance, XQSPIPS_MANUAL_START_OPTION | XQSPIPS_FORCE_SSELECT_OPTION |
        XQSPIPS_HOLD_B_DRIVE_OPTION);

    /* Set prescaler for QSPI clock */
    XQspiPs_SetClkPrescaler(&QspiInstance, XQSPIPS_CLK_PRESCALE_8);

    /* Assert flash chip select */
    XQspiPs_SetSlaveSelect(&QspiInstance);

    return rc;
}


XStatus flash_id(u8 *buf)
{
    XStatus rc;

    /* Send Read ID command */
    buf[FLASH_COMMAND] = XQSPIPS_FLASH_OPCODE_RDID;
    rc = XQspiPs_PolledTransfer(&QspiInstance, buf, buf, READ_ID_CMD_SIZE);

    return rc;
}


XStatus flash_read(u32 offset, u32 len, u8 *buf)
{
    u8      *pBuf = buf;
    u32     bytes;
    XStatus rc;

    while (len)
    {
        bytes = (len > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : len;

        /* Send read command */
        lbuf[FLASH_COMMAND]     = XQSPIPS_FLASH_OPCODE_NORM_READ;
        lbuf[FLASH_ADDRESS_MSB] = offset >> 16;
        lbuf[FLASH_ADDRESS_MID] = offset >> 8;
        lbuf[FLASH_ADDRESS_LSB] = offset & 0xFF;
        rc = XQspiPs_PolledTransfer(&QspiInstance, lbuf, lbuf, READ_CMD_SIZE + bytes);
        if (rc)
            break;

        /* Wait for completion */
        rc = flash_wait(READ_CMD_TIMEOUT);
        if (rc)
            break;

        /* Copy data to user buffer */
        memcpy(pBuf, &lbuf[FLASH_DATA], bytes);

        offset += bytes;
        pBuf += bytes;
        len -= bytes;
    }

    return rc;
}


XStatus flash_erase(u32 offset, u32 len)
{
    u32     i, count;
    XStatus rc;

    /* We can only erase full sectors */
    count = len / FLASH_SECTOR_SIZE;
    if (len % FLASH_SECTOR_SIZE)
        ++count;

    for (i = 0; i < count; ++i)
    {
        /* Send write enable command */
        lbuf[FLASH_COMMAND] = XQSPIPS_FLASH_OPCODE_WREN;
        rc = XQspiPs_PolledTransfer(&QspiInstance, lbuf, NULL, WRITE_EN_CMD_SIZE);
        if (rc)
            break;

        /* Send sector erase command */
        lbuf[FLASH_COMMAND]     = XQSPIPS_FLASH_OPCODE_SE;
        lbuf[FLASH_ADDRESS_MSB] = offset >> 16;
        lbuf[FLASH_ADDRESS_MID] = offset >> 8;
        lbuf[FLASH_ADDRESS_LSB] = offset & 0xFF;
        rc = XQspiPs_PolledTransfer(&QspiInstance, lbuf, NULL, ERASE_CMD_SIZE);
        if (rc)
            break;

        /* Wait for completion */
        rc = flash_wait(ERASE_CMD_TIMEOUT);
        if (rc)
            break;

        offset += FLASH_SECTOR_SIZE;
    }

    return rc;
}


XStatus flash_write(u32 offset, u32 len, u8 *buf)
{
    u32     i;
    XStatus rc;

    /* Length must be a multiple of page size */
    if (len % FLASH_PAGE_SIZE)
        return XST_FAILURE;

    for (i = 0; i < (len / FLASH_PAGE_SIZE); ++i)
    {
        /* Copy one page of user data into local buffer */
        memcpy(&lbuf[FLASH_DATA], &buf[i * FLASH_PAGE_SIZE], FLASH_PAGE_SIZE);

        /* Send write enable command */
        lbuf[FLASH_COMMAND] = XQSPIPS_FLASH_OPCODE_WREN;
        rc = XQspiPs_PolledTransfer(&QspiInstance, lbuf, NULL, WRITE_EN_CMD_SIZE);
        if (rc)
            break;

        /* Send page program command */
        lbuf[FLASH_COMMAND]     = XQSPIPS_FLASH_OPCODE_PP;
        lbuf[FLASH_ADDRESS_MSB] = offset >> 16;
        lbuf[FLASH_ADDRESS_MID] = offset >> 8;
        lbuf[FLASH_ADDRESS_LSB] = offset & 0xFF;
        rc = XQspiPs_PolledTransfer(&QspiInstance, lbuf, NULL, WRITE_CMD_SIZE + FLASH_PAGE_SIZE);
        if (rc)
            break;

        /* Wait for completion */
        rc = flash_wait(WRITE_CMD_TIMEOUT);
        if (rc)
            break;

        offset += FLASH_PAGE_SIZE;
    }

    return rc;
}


static XStatus flash_wait(XTime timeout)
{
    XTime   time = get_timer(0);
    XStatus rc;

    while (1)
    {
        /* Send read status command */
        lbuf[FLASH_COMMAND] = XQSPIPS_FLASH_OPCODE_RDSR1;
        rc = XQspiPs_PolledTransfer(&QspiInstance, lbuf, lbuf, READ_STATUS_CMD_SIZE);
        if (rc)
            return rc;

        /* Completed? */
        if ((lbuf[FLASH_STATUS] & FLASH_STATUS_BUSY) == 0)
            break;

        /* Evaluate timeout */
        if (get_timer(time) > timeout)
            return XST_FAILURE;

        /* Kick the dog */
        watchdog_reset();
    }

    return XST_SUCCESS;
}

