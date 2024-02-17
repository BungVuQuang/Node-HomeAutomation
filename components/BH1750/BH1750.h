#ifndef _BH1750_H_
#define _BH1750_H_

#include <stdint.h>
#include "driver/i2c.h"
/**
 * Possible chip addresses
 */
#define I2C_ADDRESS_BH1750 0x23
#define BH1750_ADDR_HI 0x5c //!< ADDR pin high
#define BH1750_ERR -1
#define BH1750_OK 0x00

/**
 * @brief BH1750 R/W Command Instruction Codes
 */
#define OPECODE_RESET 0x07
#define OPECODE_MEAS_TIME_HIGH_BIT 0x40
#define OPECODE_MEAS_TIME_LOW_BIT 0x60

/**
 * @brief Other BH1750 macros
 */
#define RES_MODE_SET_DELAY_TIME_MS 180
#define H_RES_MODE_MEASUREMENT_TIME_MS 120
#define L_RES_MODE_MEASUREMENT_TIME_MS 16
#define DEFAULT_MEAS_TIME_REG_VAL 0x45

typedef struct
{
    uint8_t i2c_addr;
    uint16_t meas_time;
    uint16_t meas_time_mul;
    uint8_t mtreg_val;
} bh1750_dev_t;

typedef enum
{
    BH1750_CONT_H_RES_MODE = 0x10,
    BH1750_CONT_H_RES_MODE2 = 0x11,
    BH1750_CONT_L_RES_MODE = 0x13,
    BH1750_ONETIME_H_RES_MODE = 0x20,
    BH1750_ONETIME_H_RES_MODE2 = 0x21,
    BH1750_ONETIME_L_RES_MODE = 0x23,
} bh1750_res_mode_t;

typedef enum
{
    BH1750_POWER_DOWN = 0x00,
    BH1750_POWER_ON = 0x01,
} bh1750_power_mode_t;

/**
 * @brief Reset BH1750
 */
int16_t bh1750_i2c_dev_reset(bh1750_dev_t dev);

/**
 * @brief Set power mode
 */
int16_t bh1750_i2c_set_power_mode(bh1750_dev_t dev, bh1750_power_mode_t mode);

/**
 * @brief Set resolution mode
 */
int16_t bh1750_i2c_set_resolution_mode(bh1750_dev_t *dev, bh1750_res_mode_t mode);

/**
 * @brief Set measurement time register value
 */
int16_t bh1750_i2c_set_mtreg_val(bh1750_dev_t *dev, float sens);

/**
 * @brief Read sensor data in lux
 */
int16_t bh1750_i2c_read_data(bh1750_dev_t dev, uint16_t *dt);

/**
 * @brief User implementation for I2C initialization
 */
int16_t bh1750_i2c_hal_init(bh1750_dev_t *dev_1);

/**
 * @brief User implementation for I2C read
 */
int16_t bh1750_i2c_hal_read(uint8_t address, uint8_t *data, uint16_t count);

/**
 * @brief User implementation for I2C write
 */
int16_t bh1750_i2c_hal_write(uint8_t address, uint8_t *data, uint16_t count);

/**
 * @brief User implementation for milliseconds delay
 */
void bh1750_i2c_hal_ms_delay(uint32_t ms);

#endif /* _BH1750_H_ */
