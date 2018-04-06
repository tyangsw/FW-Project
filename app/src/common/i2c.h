/*
 * I2C support definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __I2C_H__
#define __I2C_H__

/*
 * Definitions
 */
#define I2C_SCLK_RATE       400000
/* Worst case timeout for 1 byte is kept as 2ms */
#define I2C_BYTE_TO         2
#define I2C_BYTE_TO_BB      (I2C_BYTE_TO * 16)
#define I2C_MAX_WRITE_LEN   2

/*
 * Function prototypes
 */
XStatus i2c_init(void);
XStatus i2c_probe(u8 chip);
XStatus i2c_dev_read(u8 chip, u8 addr, u8 *buf, u16 len, bool verbose);
XStatus i2c_dev_write(u8 chip, u8 addr, u8 *buf, u16 len, bool verbose);

#endif /* __I2C_H__ */

