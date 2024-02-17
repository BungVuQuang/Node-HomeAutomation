/**
 ******************************************************************************
 * @file		app.h
 * @author	Vu Quang Bung
 * @date		26 June 2023
 ******************************************************************************
 **/
#ifndef _APP_H_
#define _APP_H_
/*-----------------------------------------------------------------------------*/
/* Header inclusions */
/*-----------------------------------------------------------------------------*/
#include "common.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"
#include "peripherals.h"
#include "utilities.h"
#include "ble_mesh_handle.h"
#include "ConfigType.h"
#include "clock_rtc.h"
#include "BH1750.h"
#include "IrHandle.h"
#include "HD44780.h"
#include "AlarmHandle.h"
#include "DS1307.h"
#include "dht11.h"
/*-----------------------------------------------------------------------------*/
/* Constant definitions  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Data type definitions */
/*-----------------------------------------------------------------------------*/

typedef struct
{
       int led1;
       int led2;
       int led3;
       int led4;
} Sensor_State_t;
Sensor_State_t Sensor_State;
esp_timer_handle_t periodic_mesh_timer;
/*-----------------------------------------------------------------------------*/
/* Macro definitions  */
/*-----------------------------------------------------------------------------*/
#define TIMER_PER_MINUTE 60000000
/*-----------------------------------------------------------------------------*/
/* Global variables  */
/*-----------------------------------------------------------------------------*/
char dataTxMesh[60];
nvs_handle_t my_handler;
bh1750_dev_t dev_1;
EventGroupHandle_t s_mesh_network_event_group;
extern uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN];
/*-----------------------------------------------------------------------------*/
/* Function prototypes  */
/*-----------------------------------------------------------------------------*/
esp_err_t nvs_save_Info(nvs_handle_t c_handle, const char *key, const void *value, size_t length);
/**
 *  @brief Hàm này được gọi lại mỗi khi nhận được dữ liệu wifi local
 *
 */
void Ble_Mesh_Init(void);

/**
 *  @brief Hàm này được gọi lại mỗi khi nhận được dữ liệu wifi local
 *
 *  @param[in] data dữ liệu
 *  @param[in] len chiều dài dữ liệu
 *  @return None
 */
void RTC_Init(void);
void System_Init(void);
void ReTransmit_Task(void *pvParameter);
#endif /* _APP_H_ */
       /********************************* END OF FILE ********************************/
       /******************************************************************************/