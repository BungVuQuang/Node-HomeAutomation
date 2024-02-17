/**
 ******************************************************************************
 * @file		peripherals.h
 * @author	Vu Quang Bung
 * @date		26 June 2023
 ******************************************************************************
 **/
#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_
/*-----------------------------------------------------------------------------*/
/* Header inclusions */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "ConfigType.h"
#include "app.h"
#include "ble_mesh_handle.h"
/*-----------------------------------------------------------------------------*/
/* Constant definitions  */
/*-----------------------------------------------------------------------------*/
#define BUTTON_RL1_PIN 34
#define BUTTON_RL2_PIN 35
#define BUTTON_RL3_PIN 36
#define BUTTON_RL4_PIN 39

#define RL1_OUT_PIN 32
#define RL2_OUT_PIN 16
#define RL3_OUT_PIN 17
#define RL4_OUT_PIN 33

#define DHT11_PIN 19
#define SR505_PIN 18
#define INFRATED_PIN 23
/*-----------------------------------------------------------------------------*/
/* Data type definitions */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Macro definitions  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Global variables  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Function prototypes  */
/*-----------------------------------------------------------------------------*/
/**
 *  @brief Config các chân đầu vào
 *
 *  @return None
 */
void input_create(int pin);

/**
 *  @brief Config các chân đầu vào
 *
 */
void IRAM_ATTR gpio_interrupt_handler(void *args);
void IRAM_ATTR gpio_interrupt_handler2(void *args);
void IRAM_ATTR gpio_interrupt_handler3(void *args);
void IRAM_ATTR gpio_interrupt_handler4(void *args);
void IRAM_ATTR gpio_interrupt_SR505_handler(void *args);
/**
 *  @brief Config các chân đầu vào
 *
 *  @return None
 */
void Show_Backup_Task(void *params);
#endif /* _PERIPHERALS_H_ */
       //********************************* END OF FILE ********************************/
       //******************************************************************************/