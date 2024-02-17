#include "BH1750.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include "math.h"

/*  I2C User Defines  */
#define I2C_MASTER_SCL_IO           21      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           22      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000


int16_t bh1750_i2c_hal_init(bh1750_dev_t *dev_1)
{
    int16_t err = BH1750_OK;

    //User implementation here

    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,    //Disable this if I2C lines have pull up resistor in place
        .scl_pullup_en = GPIO_PULLUP_ENABLE,    //Disable this if I2C lines have pull up resistor in place
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    //i2c_param_config(i2c_master_port, &conf);

    //err = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    dev_1->i2c_addr = I2C_ADDRESS_BH1750;
    dev_1->mtreg_val = DEFAULT_MEAS_TIME_REG_VAL;

    /* Perform device reset */
    err = bh1750_i2c_dev_reset(*dev_1); 

    err += bh1750_i2c_set_power_mode(*dev_1, BH1750_POWER_ON);

    /* Change measurement time with  50% optical window transmission rate */
    err += bh1750_i2c_set_mtreg_val(dev_1, 50);

    /* Configure device */
    err += bh1750_i2c_set_resolution_mode(dev_1, BH1750_CONT_H_RES_MODE);
    return BH1750_OK;
}

int16_t bh1750_i2c_hal_read(uint8_t address, uint8_t *data, uint16_t count)
{
    int16_t err = BH1750_OK;

    //User implementation here

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, address << 1 | I2C_MASTER_READ, 1);
	i2c_master_read(cmd, data, count, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);
	err = i2c_master_cmd_begin(I2C_NUM_0, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

    return err == BH1750_OK ? BH1750_OK : BH1750_ERR;
}

int16_t bh1750_i2c_hal_write(uint8_t address, uint8_t *data, uint16_t count)
{
    int16_t err = BH1750_OK;

    //User implementation here

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write(cmd, data, count, 1);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(I2C_NUM_0, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return err == BH1750_OK ? BH1750_OK : BH1750_ERR;
}

void bh1750_i2c_hal_ms_delay(uint32_t ms) {
    //User implementation here
    vTaskDelay(pdMS_TO_TICKS(ms));
}

int16_t bh1750_i2c_dev_reset(bh1750_dev_t dev)
{
    uint8_t data = OPECODE_RESET;
    int16_t err = bh1750_i2c_hal_write(dev.i2c_addr, &data, 1);
    
    return err;
}

int16_t bh1750_i2c_set_power_mode(bh1750_dev_t dev, bh1750_power_mode_t mode)
{
    uint8_t data = (uint8_t)mode;
    int16_t err = bh1750_i2c_hal_write(dev.i2c_addr, &data, 1);
    
    return err;
}

int16_t bh1750_i2c_set_resolution_mode(bh1750_dev_t *dev, bh1750_res_mode_t mode)
{
    uint8_t data = (uint8_t)mode;
    int16_t err = bh1750_i2c_hal_write(dev->i2c_addr, &data, 1);

    if(err != BH1750_OK)
        return err;

    if(mode == BH1750_CONT_L_RES_MODE || mode == BH1750_ONETIME_L_RES_MODE)
        dev->meas_time = L_RES_MODE_MEASUREMENT_TIME_MS;
    else
        dev->meas_time = H_RES_MODE_MEASUREMENT_TIME_MS;

    bh1750_i2c_hal_ms_delay(RES_MODE_SET_DELAY_TIME_MS);
    
    return err;
}

int16_t bh1750_i2c_set_mtreg_val(bh1750_dev_t *dev, float sens)
{
    uint8_t data;
    int16_t err;
    sens /= 100;
    if(sens > 100 || ((dev->mtreg_val * ((uint8_t) (1 / sens))) > 0xFF))
        return BH1750_ERR;
    
    uint8_t mtreg = dev->mtreg_val * (uint8_t) (1 / sens);

    /* Send high bits */
    data = OPECODE_MEAS_TIME_HIGH_BIT | (mtreg >> 5);
    err = bh1750_i2c_hal_write(dev->i2c_addr, &data, 1);

    if(err != BH1750_OK)
        return err;

    /* Send low bits */
    data = OPECODE_MEAS_TIME_LOW_BIT | (mtreg & 0x1F);
    err = bh1750_i2c_hal_write(dev->i2c_addr, &data, 1);

    if(err != BH1750_OK)
        return err;

    dev->meas_time_mul = (uint8_t) (1 / sens);
    dev->mtreg_val = mtreg;

    return err;
}

int16_t bh1750_i2c_read_data(bh1750_dev_t dev, uint16_t *dt)
{
    uint8_t data[2];
    int16_t err = bh1750_i2c_hal_read(dev.i2c_addr, data, 2);

    *dt = (data[0] << 8) | data[1];
    *dt = round(*dt / 1.2f);

    bh1750_i2c_hal_ms_delay(dev.meas_time * dev.meas_time_mul);

    return err;
}