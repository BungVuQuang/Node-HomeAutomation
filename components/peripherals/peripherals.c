/**
 ******************************************************************************
 * @file		peripherals.c
 * @author	Vu Quang Bung
 * @date		26 June 2023
 * Copyright (C) 2023 - Bung.VQ
 ******************************************************************************
 **/
/*-----------------------------------------------------------------------------*/
/* Header inclusions */
/*-----------------------------------------------------------------------------*/
#include "peripherals.h"
/*-----------------------------------------------------------------------------*/
/* Local Constant definitions */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Macro definitions */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Data type definitions */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Global variables */
/*-----------------------------------------------------------------------------*/
xQueueHandle interputQueue;
volatile uint8_t srs505_Flag = 0;
/*-----------------------------------------------------------------------------*/
/* Function prototypes */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Function implementations */
/*-----------------------------------------------------------------------------*/
/**
 *  @brief Config các chân đầu vào
 *
 */
void input_create(int pin)
{
    gpio_config_t io_conf;
    // disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL << pin);
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
}

/**
 *  @brief Config các chân đầu vào
 *
 */
void IRAM_ATTR gpio_interrupt_handler(void *args) // hàm ISR ngắt ngoài
{
    int pinNumber = (int)args;
    xQueueSendFromISR(interputQueue, &pinNumber, NULL);
}

/**
 *  @brief Config các chân đầu vào
 *
 */
void IRAM_ATTR gpio_interrupt_handler2(void *args) // hàm ISR ngắt ngoài
{
    int pinNumber = (int)args;
    xQueueSendFromISR(interputQueue, &pinNumber, NULL);
}

/**
 *  @brief Config các chân đầu vào
 *
 */
void IRAM_ATTR gpio_interrupt_handler3(void *args) // hàm ISR ngắt ngoài
{
    int pinNumber = (int)args;
    xQueueSendFromISR(interputQueue, &pinNumber, NULL);
}

/**
 *  @brief Config các chân đầu vào
 *
 */
void IRAM_ATTR gpio_interrupt_handler4(void *args) // hàm ISR ngắt ngoài
{
    int pinNumber = (int)args;
    xQueueSendFromISR(interputQueue, &pinNumber, NULL);
}

/**
 *  @brief Config các chân đầu vào
 *
 */
void IRAM_ATTR gpio_interrupt_SR505_handler(void *args) // hàm ISR ngắt ngoài
{
    int pinNumber = (int)args;
    xQueueSendFromISR(interputQueue, &pinNumber, NULL);
}

/**
 *  @brief Config các chân đầu vào
 *
 */
void Show_Backup_Task(void *params)
{
    int pinNumber = 0;
    uint16_t dataLight = 0;
    // char data_temper[15] = "hello ack";
    while (true)
    {
        if (xQueueReceive(interputQueue, &pinNumber, portMAX_DELAY)) // chờ có items
        {
            printf("Da VAO INTERRUPT\n");
            memset(dataTxMesh, 0, 30);
            if (pinNumber == 0)
            {
                printf("Da an Button 0\n");
                // esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                //                                ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(data_temper), (uint8_t *)data_temper);
                // store.param_po.model->pub->publish_addr = 0xFFFF;
                // esp_ble_mesh_model_publish(store.param_po.model, ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, sizeof(data_temper), (uint8_t *)data_temper, ROLE_NODE);
            }
            else if (pinNumber == BUTTON_RL1_PIN)
            {
                printf("Da an Button BUTTON_RL1_PIN\n");
                if (Sensor_State.led1 == 0)
                {
                    sprintf(dataTxMesh, "led1 1");
                    Sensor_State.led1 = 1;
                    gpio_set_level(RL1_OUT_PIN, 0);
                }
                else
                {
                    sprintf(dataTxMesh, "led1 0");
                    Sensor_State.led1 = 0;
                    gpio_set_level(RL1_OUT_PIN, 1);
                }
                esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                   ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
                // xEventGroupSetBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT);
                nvs_save_Info(my_handler, "STATE", &Sensor_State, sizeof(Sensor_State));
                // esp_restart();
            }
            else if (pinNumber == BUTTON_RL2_PIN)
            {
                printf("Da an Button BUTTON_RL2_PIN\n");
                if (Sensor_State.led2 == 0)
                {
                    sprintf(dataTxMesh, "led2 1");
                    Sensor_State.led2 = 1;
                    gpio_set_level(RL2_OUT_PIN, 0);
                }
                else
                {
                    sprintf(dataTxMesh, "led2 0");
                    Sensor_State.led2 = 0;
                    gpio_set_level(RL2_OUT_PIN, 1);
                }
                esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                   ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
                // xEventGroupSetBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT);
                nvs_save_Info(my_handler, "STATE", &Sensor_State, sizeof(Sensor_State));
            }
            else if (pinNumber == BUTTON_RL3_PIN)
            {
                printf("Da an Button BUTTON_RL3_PIN\n");
                if (Sensor_State.led3 == 0)
                {
                    sprintf(dataTxMesh, "led3 1");
                    Sensor_State.led3 = 1;
                    gpio_set_level(RL3_OUT_PIN, 0);
                }
                else
                {
                    sprintf(dataTxMesh, "led3 0");
                    Sensor_State.led3 = 0;
                    gpio_set_level(RL3_OUT_PIN, 1);
                }
                esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                   ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
                // xEventGroupSetBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT);
                nvs_save_Info(my_handler, "STATE", &Sensor_State, sizeof(Sensor_State));
            }
            else if (pinNumber == BUTTON_RL4_PIN)
            {
                printf("Da an Button BUTTON_RL4_PIN\n");
                if (Sensor_State.led4 == 0)
                {
                    sprintf(dataTxMesh, "led4 1");
                    Sensor_State.led4 = 1;
                    gpio_set_level(RL4_OUT_PIN, 0);
                }
                else
                {
                    sprintf(dataTxMesh, "led4 0");
                    Sensor_State.led4 = 0;
                    gpio_set_level(RL4_OUT_PIN, 1);
                }
                esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                   ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
                // xEventGroupSetBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT);
                nvs_save_Info(my_handler, "STATE", &Sensor_State, sizeof(Sensor_State));
            }
            else if (pinNumber == SR505_PIN)
            {
                // if (srs505_Flag == 0)
                // {
                //     Sensor_State.led1 = 1;
                //     gpio_set_level(RL1_OUT_PIN, 0);
                //     srs505_Flag = 1;
                // }
                // else if (srs505_Flag == 1)
                // {
                //     Sensor_State.led1 = 0;
                //     gpio_set_level(RL1_OUT_PIN, 1);
                //     srs505_Flag = 0;
                // }
                bh1750_i2c_read_data(dev_1, &dataLight);
                printf("Lux:%d\n", dataLight);
                // if (dataLight < 150)
                // {
                //     if (Sensor_State.led1 == 0)
                //     {
                //         sprintf(dataTxMesh, "led1 1");
                //         Sensor_State.led1 = 1;
                //         gpio_set_level(RL1_OUT_PIN, 0);
                //     }
                //     else
                //     {
                //         sprintf(dataTxMesh, "led1 0");
                //         Sensor_State.led1 = 0;
                //         gpio_set_level(RL1_OUT_PIN, 1);
                //     }
                //     esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                //                                        ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
                //     // xEventGroupSetBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT);
                //     nvs_save_Info(my_handler, "STATE", &Sensor_State, sizeof(Sensor_State));
                // }
                if (dataLight < 150)
                {
                    if (Sensor_State.led3 == 0)
                    {
                        sprintf(dataTxMesh, "led3 1");
                        Sensor_State.led3 = 1;
                        gpio_set_level(RL3_OUT_PIN, 0);
                    }
                    else
                    {
                        sprintf(dataTxMesh, "led3 0");
                        Sensor_State.led3 = 0;
                        gpio_set_level(RL3_OUT_PIN, 1);
                    }
                    esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                       ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
                    // xEventGroupSetBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT);
                    nvs_save_Info(my_handler, "STATE", &Sensor_State, sizeof(Sensor_State));
                }
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}