/*
 * Voltage and temperature monitoring definitions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#ifndef __VOLT_TEMP_H__
#define __VOLT_TEMP_H__

/*
 * Temperature sensor definitions
 */
#define I2C_TEMP_FUNC_MOD_ADDR  0x1C

#define TEMP_CAPS_REG           0
#define TEMP_CONFIG_REG         1
#define TEMP_UPPER_REG          2
#define TEMP_LOWER_REG          3
#define TEMP_CRIT_REG           4
#define TEMP_A_REG              5
#define TEMP_MFG_ID_REG         6
#define TEMP_DEV_ID_REG         7
#define TEMP_RES_REG            8

#define TEMP_LT_0_BIT           (1 << 4)
#define TEMP_LT_LOWER_BIT       (1 << 5)
#define TEMP_GT_UPPER_BIT       (1 << 6)
#define TEMP_GTE_CRIT_BIT       (1 << 7)

#define TEMP_FRACT_MASK         0x0F

#define TEMP_UPPER_VAL          75  // +75 C
#define TEMP_LOWER_VAL          20  // -20 C
#define TEMP_CRIT_VAL           95  // +95 C

#define VOLT_TEMP_READ_INTERVAL 60000   // 1 min

/*
 * LM60 temperature sensor definitions
 */
#define LM60_INT2FLOAT      ((0.015259 * 3) / 2)    // 1000 * 1/1^16 * 2/3: 1000mv/v * 1/2^16--16 bits * resistor divider of 2/3
#define LM60_ZERO_OFFSET    424                     // Temp sensor generates 424mv at 0 deg. C
#define LM60_GAIN           6.25                    // Temp sensor has a gain of 6.25mv/deg. C

/*
 * Function prototypes
 */
void volt_temp_read(void);
XStatus if_temp_volt_init(void);
void if_volt_temp_read(u8 ch, int *vt_i, int *vt_f);
void func_temp_init(void);
void func_temp_read(s8 *temp_i, s8 *temp_f);
void func_temp_convert(u8 *buf, s8 *temp_c, s8 *temp_f);

#endif /* __VOLT_TEMP_H__ */

