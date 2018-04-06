/*
 * I2C support functions targeting I2C0.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */
 
#include <includes.h>


/* Global variables */
static XIicPs IicInstance;

/* Local functions */
static XStatus i2c_wait(void);


XStatus i2c_init(void)
{
    XIicPs_Config   *cfg = NULL;
    XStatus         rc;

    /* Lookup I2C configuration */
    cfg = XIicPs_LookupConfig(XPAR_XIICPS_0_DEVICE_ID);
    if (!cfg)
        return XST_FAILURE;

    /* Initialize I2C driver */
    memset(&IicInstance, 0, sizeof(IicInstance));
    rc = XIicPs_CfgInitialize(&IicInstance, cfg, cfg->BaseAddress);
    if (rc)
        return rc;

    /* Set I2C serial clock rate */
    XIicPs_SetSClk(&IicInstance, I2C_SCLK_RATE);

    return rc;
}


XStatus i2c_read(u8 chip, u8 addr, u8 *buf, u16 len)
{
    XStatus rc;

    /* Send address */
    rc = XIicPs_MasterSendPolled(&IicInstance, &addr, 1, chip);
    if (rc)
        return rc;

    /* Receive data */
    rc = XIicPs_MasterRecvPolled(&IicInstance, buf, len, chip);
    if (rc)
        return rc;

    /* Wait until bus is idle */
    rc = i2c_wait();

    return rc;
}


XStatus i2c_write(u8 chip, u8 addr, u8 *buf, u16 len)
{
    u8      lbuf[I2C_MAX_WRITE_LEN + 1];
    XStatus rc;

    /* Make sure length doesn't exceed max */
	if (len > I2C_MAX_WRITE_LEN)
	{
        printf("Error in i2c_write: len > max (len=%d, max=%d)\n", len, I2C_MAX_WRITE_LEN);
        return XST_FAILURE;
	}

    /* Copy address and data into local buffer - they must be sent in one transfer */
    lbuf[0] = addr;
    memcpy(&lbuf[1], &buf[0], len);

    /* Send address + data */
    rc = XIicPs_MasterSendPolled(&IicInstance, lbuf, len + 1, chip);
    if (rc)
        return rc;

    /* Wait until bus is idle */
    rc = i2c_wait();

    return rc;
}


XStatus i2c_dev_read(u8 chip, u8 addr, u8 *buf, u16 len, bool verbose)
{
    XStatus rc;

    memset(buf, 0, len);
    rc = i2c_read(chip, addr, buf, len);
    if (rc && verbose)
        printf("Error reading I2C chip %02X addr %02X\n", chip, addr);

    return rc;
}


XStatus i2c_dev_write(u8 chip, u8 addr, u8 *buf, u16 len, bool verbose)
{
    XStatus rc;

    rc = i2c_write(chip, addr, buf, len);
    if (rc && verbose)
        printf("Error writing I2C chip %02X addr %02X\n", chip, addr);

    /* Wait for write operation to complete on actual memory */
    usleep(10000);

    return rc;
}


XStatus i2c_probe(u8 chip)
{
    XStatus rc;
    u8      buf;

    rc = i2c_read(chip, 0, &buf, 1);

    return rc;
}


static XStatus i2c_wait(void)
{
    XTime time = get_timer(0);

    while (XIicPs_BusIsBusy(&IicInstance))
    {
        /* Evaluate timeout */
        if (get_timer(time) > I2C_BYTE_TO_BB)
            return XST_FAILURE;
    }

    return XST_SUCCESS;
}

