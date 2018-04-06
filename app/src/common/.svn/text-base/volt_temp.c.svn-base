/*
 * Voltage and temperature monitoring functions.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */
 
#include <includes.h>


/* Global variables */
static XAdcPs XAdcInstance;

/* Local functions */
static int XAdcFractionToInt(float fp_num);
static float XAdcRawToTemp(u32 data);


void volt_temp_read(void)
{
    int vt_i = 0;
    int vt_f = 0;
    static XTime time;
    static bool  time_to_read = TRUE;

    if (time_to_read)
    {
        /* Write functional module temperature to common area */
        func_temp_read((s8 *)&vt_i, (s8 *)&vt_f);
        pCommon->func_mod_temp_ex[FUNC_MOD_PCB_TEMP] = (vt_i << 16) | vt_f;
        if ((s8)vt_f >= 50)
            ++vt_i;
        pCommon->func_mod_temp[FUNC_MOD_PCB_TEMP] = vt_i;
        if ((s8)vt_i > pCommon->func_mod_temp_max[FUNC_MOD_PCB_TEMP])
            pCommon->func_mod_temp_max[FUNC_MOD_PCB_TEMP] = vt_i;
        if ((s8)vt_i < pCommon->func_mod_temp_min[FUNC_MOD_PCB_TEMP])
            pCommon->func_mod_temp_min[FUNC_MOD_PCB_TEMP] = vt_i;

        /* Write interface module temperature to common area */
        if_volt_temp_read(XADCPS_CH_VPVN, &vt_i, &vt_f);
        pCommon->if_mod_temp_ex[IF_MOD_PCB_TEMP] = (vt_i << 16) | vt_f;
        if (vt_f >= 500)
            ++vt_i;
        if (vt_i > 127)
            vt_i = 127;
        pCommon->if_mod_temp[IF_MOD_PCB_TEMP] = vt_i;
        if ((s8)vt_i > pCommon->if_mod_temp_max[IF_MOD_PCB_TEMP])
            pCommon->if_mod_temp_max[IF_MOD_PCB_TEMP] = vt_i;
        if ((s8)vt_i < pCommon->if_mod_temp_min[IF_MOD_PCB_TEMP])
            pCommon->if_mod_temp_min[IF_MOD_PCB_TEMP] = vt_i;

        /* Write Zynq Core temperature to common area */
        if_volt_temp_read(XADCPS_CH_TEMP, &vt_i, &vt_f);
        pCommon->if_mod_temp_ex[IF_MOD_CORE_TEMP] = (vt_i << 16) | vt_f;
        if (vt_f >= 500)
            ++vt_i;
        if (vt_i > 127)
            vt_i = 127;
        pCommon->if_mod_temp[IF_MOD_CORE_TEMP] = vt_i;
        if ((s8)vt_i > pCommon->if_mod_temp_max[IF_MOD_CORE_TEMP])
            pCommon->if_mod_temp_max[IF_MOD_CORE_TEMP] = vt_i;
        if ((s8)vt_i < pCommon->if_mod_temp_min[IF_MOD_CORE_TEMP])
            pCommon->if_mod_temp_min[IF_MOD_CORE_TEMP] = vt_i;

        /* Write Zynq Core voltage to common area */
        if_volt_temp_read(XADCPS_CH_VCCPINT, &vt_i, &vt_f);
        pCommon->zynq_core_volt = (vt_i << 16) | vt_f;

        /* Write Zynq Aux voltage to common area */
        if_volt_temp_read(XADCPS_CH_VCCPAUX, &vt_i, &vt_f);
        pCommon->zynq_aux_volt = (vt_i << 16) | vt_f;

        /* Write Zynq DDR voltage to common area */
        if_volt_temp_read(XADCPS_CH_VCCPDRO, &vt_i, &vt_f);
        pCommon->zynq_ddr_volt = (vt_i << 16) | vt_f;

        /* Restart timer */
        time_to_read = FALSE;
        time = get_timer(0);
    }

    /* Time to read? */
    if (get_timer(time) > VOLT_TEMP_READ_INTERVAL)
        time_to_read = TRUE;
}

/**************************** Interface Module *****************************/

XStatus if_temp_volt_init(void)
{
    XAdcPs_Config   *cfg = NULL;
    XStatus         rc;

    /* Lookup XADC configuration */
    cfg = XAdcPs_LookupConfig(XPAR_XADCPS_0_DEVICE_ID);
    if (!cfg)
        return XST_FAILURE;

    /* Initialize XADC driver */
    memset(&XAdcInstance, 0, sizeof(XAdcInstance));
    rc = XAdcPs_CfgInitialize(&XAdcInstance, cfg, cfg->BaseAddress);
    if (rc)
        return rc;

    /* Setup Channel Sequencer to read desired channels */
    XAdcPs_SetSequencerMode(&XAdcInstance, XADCPS_SEQ_MODE_SAFE);
    XAdcPs_SetAvg(&XAdcInstance, XADCPS_AVG_16_SAMPLES);
    rc = XAdcPs_SetSeqChEnables(&XAdcInstance, XADCPS_SEQ_CH_TEMP | XADCPS_SEQ_CH_VPVN |
         XADCPS_SEQ_CH_VCCPINT | XADCPS_SEQ_CH_VCCPAUX | XADCPS_SEQ_CH_VCCPDRO);
    XAdcPs_SetSequencerMode(&XAdcInstance, XADCPS_SEQ_MODE_ONEPASS);

    /* Initialize MAX/MIN temp in common area */
    pCommon->if_mod_temp_max[IF_MOD_CORE_TEMP] = -128;
    pCommon->if_mod_temp_max[IF_MOD_PCB_TEMP]  = -128;
    pCommon->if_mod_temp_min[IF_MOD_CORE_TEMP] = 127;
    pCommon->if_mod_temp_min[IF_MOD_PCB_TEMP]  = 127;

    return rc;
}


void if_volt_temp_read(u8 ch, int *vt_i, int *vt_f)
{
    u32     data;
    float   vt;

    /* Sequence channel select registers */
    XAdcPs_SetSequencerMode(&XAdcInstance, XADCPS_SEQ_MODE_SAFE);
    XAdcPs_SetSequencerMode(&XAdcInstance, XADCPS_SEQ_MODE_ONEPASS);

    /* Read data from ADC registers */
    data = XAdcPs_GetAdcData(&XAdcInstance, ch);

    /* Convert it to voltage/temperature */
    if (ch == XADCPS_CH_TEMP)
        vt = XAdcPs_RawToTemperature(data);
    else if (ch == XADCPS_CH_VPVN)
        vt = XAdcRawToTemp(data);
    else
        vt = XAdcPs_RawToVoltage(data);

    if (vt_i)
        *vt_i = (int)vt;
    if (vt_f)
        *vt_f = XAdcFractionToInt(vt);
}


static int XAdcFractionToInt(float fp_num)
{
    float   temp;

    if (fp_num < 0)
        temp = -fp_num;
    else
        temp = fp_num;

    return(((int)((temp - (float)((int)temp)) * (1000.0f))));
}


/*
 * Converts from the A/D reading to degrees C for a LM60 temp. sensor followed by a 2/3 gain.
 * Returns degrees C.
 *
 * Contributed by Paul Feldman
 * ( I didn't know Paul could write software ;-) )
 */
static float XAdcRawToTemp(u32 data)
{
    float temp;

    /* Convert to float, subtract offset, scale by gain */
    temp = ((float)(data * LM60_INT2FLOAT) - LM60_ZERO_OFFSET) / LM60_GAIN;

    return temp;
}

/**************************** Functional Module *****************************/

void func_temp_init(void)
{
    u8  buf[2];

    /* Program T_Upper register */
    buf[0] = (u8)(TEMP_UPPER_VAL >> 4);
    buf[1] = (u8)(TEMP_UPPER_VAL << 4);
    i2c_dev_write(I2C_TEMP_FUNC_MOD_ADDR, TEMP_UPPER_REG, buf, 2, TRUE);

    /* Program T_Lower register */
    buf[0] = (u8)(TEMP_LOWER_VAL >> 4) | TEMP_LT_0_BIT;
    buf[1] = (u8)(TEMP_LOWER_VAL << 4);
    i2c_dev_write(I2C_TEMP_FUNC_MOD_ADDR, TEMP_LOWER_REG, buf, 2, TRUE);

    /* Program T_Crit register */
    buf[0] = (u8)(TEMP_CRIT_VAL >> 4);
    buf[1] = (u8)(TEMP_CRIT_VAL << 4);
    i2c_dev_write(I2C_TEMP_FUNC_MOD_ADDR, TEMP_CRIT_REG, buf, 2, TRUE);

    /* Initialize MAX/MIN temp in common area */
    pCommon->func_mod_temp_max[FUNC_MOD_PCB_TEMP] = -128;
    pCommon->func_mod_temp_min[FUNC_MOD_PCB_TEMP] = 127;
}


void func_temp_read(s8 *temp_i, s8 *temp_f)
{
    u8  buf[2] = { 0 };

    /* Read current temp */
    i2c_dev_read(I2C_TEMP_FUNC_MOD_ADDR, TEMP_A_REG, buf, 2, FALSE);

    /* Convert it to degrees in Celsius */
    func_temp_convert(buf, temp_i, temp_f);
}


void func_temp_convert(u8 *buf, s8 *temp_i, s8 *temp_f)
{
    if (!buf)
        return;

    if (temp_i)
    {
        *temp_i = (buf[0] << 4) + (buf[1] >> 4);
        /* MCP9843 datasheet is wrong; <0C temperature samples already come back as negative values
        if (buf[0] & TEMP_LT_0_BIT) // TA < 0C
            *temp_i = 256 - *temp_i;
        */
    }
    if (temp_f)
        *temp_f = ((buf[1] & TEMP_FRACT_MASK) * 625) / 100;
}

