/**
 ******************************************************************************
 * @file		app.c
 * @author	Vu Quang Bung
 * @date		26 June 2023
 ******************************************************************************
 **/
/*-----------------------------------------------------------------------------*/
/* Header inclusions */
/*-----------------------------------------------------------------------------*/
#include "app.h"
#include "panasonic_ir.h"
/*-----------------------------------------------------------------------------*/
/* Local Constant definitions */
/*-----------------------------------------------------------------------------*/
static const char *TAG = "APP";
const char *NVS_KEY_PERIOD = "PERIOD";
const char *NVS_KEY_STATE = "STATE";
const char *NVS_KEY_AIR = "AIR";
const char *NVS_KEY_ALARM = "ALARM";
const char *NVS_KEY_HEAPSIZE = "HEAPSIZE";
const char *NVS_KEY = "Node";
uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN] = {0x32, 0x10};
/*-----------------------------------------------------------------------------*/
/* Local Macro definitions */
/*-----------------------------------------------------------------------------*/
#define TWDT_TIMEOUT_S 40
#define TASK_WS_STACK_SIZE 2500
#define TASK_NETWORK_STACK_SIZE 2300
#define TASK_INTERUPT_STACK_SIZE 1024
#define TASK_TxRMT_STACK_SIZE 4096
#define TASK_RTC_STACK_SIZE 1800
/*========================AIR CONDITIONER ===================*/
#define AIR_STATE_OFF 0
#define AIR_STATE_ON 1

#define AIR_MODE_AUTO 0
#define AIR_MODE_DRY 1
#define AIR_MODE_COOL 2
#define AIR_MODE_HEAT 3
#define AIR_MODE_FAN 4

#define AIR_SWING_1 1
#define AIR_SWING_2 2
#define AIR_SWING_3 3
#define AIR_SWING_4 4
#define AIR_SWING_5 5
#define AIR_SWING_AUTO 0

#define AIR_FAN_1 1
#define AIR_FAN_2 2
#define AIR_FAN_3 3
#define AIR_FAN_4 4
#define AIR_FAN_5 5
#define AIR_FAN_AUTO 0

#define CHECK_ERROR_CODE(returned, expected) ({ \
    if (returned != expected)                   \
    {                                           \
        printf("TWDT ERROR\n");                 \
        abort();                                \
    }                                           \
})

/*-----------------------------------------------------------------------------*/
/* Local Data type definitions */
/*-----------------------------------------------------------------------------*/
i2c_dev_t dev;
struct dht11_reading dht_properties;
esp_timer_handle_t periodic_timer;
esp_timer_handle_t periodic_light_timer;
// AC_Properties_t AC_Properties = {0};
State_Command_Rx_t State_Command_Rx = {NOT_LEARNED};
State_LCD_t State_LCD = NORMAL_LCD;
size_t length = 0;
char row_one_lcd[20];
char row_two_lcd[20];

typedef struct
{
    int tPeriod;
    int hPeriod;
    int lPeriod;
} Sensor_Period_t;

/* ============== NVS SAVE VARIABLE ===========================================*/
Sensor_Period_t Sensor_Period;
int index1 = 0;
int index2 = 0;
uint16_t dataLight = 0;
struct Alarm alarms[10]; // Sử dụng một mảng để lưu trữ Min-Heap
int heapSize = -1;       // Kích thước của Min-Heap
struct panasonic_command panasonic_command_t = {
    .cmd = CMD_STATE,
    .mode = MODE_COOL,
    .swing = SWING_5,
    .fan = FAN_AUTO,
    .on_time = 0,       // Thời gian bật
    .off_time = 0,      // Thời gian tắt
    .time = 0,          // Thời gian
    .temp = 26,         // Nhiệt độ
    .on = false,        // Đang bật
    .on_timer = false,  // Bật theo thời gian
    .off_timer = false, // Tắt theo thời gian
    .no_time = false,   // Không có thời gian được đặt
};
/*-----------------------------------------------------------------------------*/
/* Global variables */
/*-----------------------------------------------------------------------------*/
uint8_t Air_Conditioner_Control = 0;

//===================IR HANDLE==============================
extern nvs_handle_t NVS_HANDLE;
extern xQueueHandle interputQueue;

static TaskHandle_t reset_wdt_task_handles;
TaskHandle_t xTaskInterrupt;
TaskHandle_t xTaskRtcHandle;
StaticTask_t xTaskWsHandle;
StaticTask_t xTaskNetworkHandle;
// StaticTask_t xTaskTxRMTHandle;
//  static StackType_t xStackInterrupt[TASK_INTERUPT_STACK_SIZE];
static StackType_t xStackWs[TASK_WS_STACK_SIZE];
// static StackType_t xStackRTC[TASK_RTC_STACK_SIZE];
//  StaticTask_t xTaskNetworkHandle;
static StackType_t xStackNetwork[TASK_NETWORK_STACK_SIZE];
// static StackType_t xStackTxRMT[TASK_TxRMT_STACK_SIZE];
DeviceStateNoInternet_t deviceStateNoInternet[6];
/*-----------------------------------------------------------------------------*/
/* Function prototypes */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Function implementations */
/*-----------------------------------------------------------------------------*/
/**
 *  @brief lưu lại thông tin wifi từ nvs
 *
 */
esp_err_t nvs_save_Info(nvs_handle_t c_handle, const char *key, const void *value, size_t length)
{
    esp_err_t err;
    nvs_open("storage0", NVS_READWRITE, &c_handle);
    // strcpy(wifi_info.SSID, "anhbung");
    nvs_set_blob(c_handle, key, value, length);
    err = nvs_commit(c_handle);
    if (err != ESP_OK)
        return err;

    // Close
    nvs_close(c_handle);
    return err;
}
/**
 *  @brief Lấy lại trong tin wifi từ nvs
 *
 */
esp_err_t nvs_get_Info(nvs_handle_t c_handle, const char *key, void *out_value)
{
    esp_err_t err;
    err = nvs_open("storage0", NVS_READWRITE, &c_handle);
    if (err != ESP_OK)
        return err;
    size_t required_size = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(c_handle, key, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;
    if (required_size == 0)
    {
        printf("Nothing saved yet!\n");
    }
    else
    {
        nvs_get_blob(c_handle, key, out_value, &required_size);

        err = nvs_commit(c_handle);
        if (err != ESP_OK)
            return err;

        // Close
        nvs_close(c_handle);
    }
    return err;
}

void xTaskHandelInfrated(void *arg)
{
    rmt_rx_config();
    rmt_tx_config();
    rmt_item32_t *items = NULL;
    // define ringbuffer handle
    RingbufHandle_t rb;
    size_t rx_size = 0;
    bool repeat = false;
    uint32_t addr = 0;
    while (1)
    {
        // xEventGroupWaitBits(g_control_event_group, // đợi đến khi có tin nhắn từ node gửi đến
        //                     INFRATE_RECIVE_MESSAGE,
        //                     pdFALSE,
        //                     pdFALSE,
        //                     portMAX_DELAY);
        // get the ring buffer handle
        rmt_get_ringbuf_handle(RMT_CHANNEL_0, &rb);

        // get items, if there are any
        items = (rmt_item32_t *)xRingbufferReceive(rb, &rx_size, 10);
        if (items)
        {
            if (ir_parser->input(ir_parser, items, 34) == ESP_OK)
            {
                if (ir_parser->get_scan_code(ir_parser, &addr, &cmd, &repeat) == ESP_OK)
                {
                    // xEventGroupSetBits(g_control_event_group, INFRATE_RECIVE_MESSAGE);
                    //  ESP_LOGI(TAG, "addr: 0x%04x cmd: 0x%04x  lenght: %d", addr, cmd, 34);
                    if (cmd == BUTTON_0)
                    {
                        ESP_LOGI(TAG, " Đã ấn button 0");
                    }
                    else if (cmd == BUTTON_1)
                    {
                        memset(dataTxMesh, 0, 30);
                        ESP_LOGI(TAG, " Đã ấn button 1");
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
                        xEventGroupSetBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT);
                        nvs_save_Info(my_handler, "STATE", &Sensor_State, sizeof(Sensor_State));
                    }
                    else if (cmd == BUTTON_2)
                    {
                        memset(dataTxMesh, 0, 30);
                        ESP_LOGI(TAG, " Đã ấn button 2");
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
                        xEventGroupSetBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT);
                        nvs_save_Info(my_handler, "STATE", &Sensor_State, sizeof(Sensor_State));
                    }
                    else if (cmd == BUTTON_3)
                    {
                        memset(dataTxMesh, 0, 30);
                        ESP_LOGI(TAG, " Đã ấn button 3");
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
                        xEventGroupSetBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT);
                        nvs_save_Info(my_handler, "STATE", &Sensor_State, sizeof(Sensor_State));
                    }
                    else if (cmd == BUTTON_4)
                    {
                        memset(dataTxMesh, 0, 30);
                        ESP_LOGI(TAG, " Đã ấn button 4");
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
                        xEventGroupSetBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT);
                        nvs_save_Info(my_handler, "STATE", &Sensor_State, sizeof(Sensor_State));
                    }
                    else if (cmd == BUTTON_5)
                    {
                        ESP_LOGI(TAG, " Đã ấn button 5");
                    }
                    else if (cmd == BUTTON_6)
                    {
                        ESP_LOGI(TAG, " Đã ấn button 6");
                        xEventGroupClearBits(g_control_event_group, CLOCK_TASK_RUN);
                        I2C_LCD_Clear();
                        memset(row_one_lcd, 0, 20);
                        memset(row_two_lcd, 0, 20);
                        sprintf(row_one_lcd, "pAnhSang:%d p", Sensor_Period.lPeriod);
                        I2C_LCD_GotoXY(0, 0);
                        I2C_LCD_Puts(row_one_lcd);
                    }
                    else if (cmd == BUTTON_7)
                    {
                        ESP_LOGI(TAG, " Đã ấn button 7");
                        I2C_LCD_Clear();
                        xEventGroupClearBits(g_control_event_group, CLOCK_TASK_RUN);
                        memset(row_one_lcd, 0, 20);
                        memset(row_two_lcd, 0, 20);
                        sprintf(row_one_lcd, "pNhietDo:%d p", Sensor_Period.tPeriod);
                        sprintf(row_two_lcd, "pDoAm   :%d p", Sensor_Period.hPeriod);
                        I2C_LCD_GotoXY(0, 0);
                        I2C_LCD_Puts(row_one_lcd);
                        I2C_LCD_GotoXY(0, 1);
                        I2C_LCD_Puts(row_two_lcd);
                    }
                    else if (cmd == BUTTON_8)
                    {
                        I2C_LCD_Clear();
                        xEventGroupClearBits(g_control_event_group, CLOCK_TASK_RUN);
                        memset(row_one_lcd, 0, 20);
                        memset(row_two_lcd, 0, 20);
                        sprintf(row_one_lcd, "%s", alarms[0].time);
                        sprintf(row_two_lcd, "%s", alarms[0].date);
                        I2C_LCD_GotoXY(0, 0);
                        I2C_LCD_Puts(row_one_lcd);
                        I2C_LCD_GotoXY(0, 1);
                        I2C_LCD_Puts(row_two_lcd);
                    }
                    else if (cmd == BUTTON_9)
                    {
                        ESP_LOGI(TAG, " Đã ấn button 9");
                        xEventGroupClearBits(g_control_event_group, CLOCK_TASK_RUN);
                        memset(row_one_lcd, 0, 20);
                        memset(row_two_lcd, 0, 20);
                        I2C_LCD_Clear();
                        dht_properties = DHT11_read();
                        bh1750_i2c_read_data(dev_1, &dataLight);
                        sprintf(row_one_lcd, "Nhiet Do:%d.%d oC", dht_properties.temperature, dht_properties.dotTemperature);
                        sprintf(row_two_lcd, "DoAm:%d%% Lux:%d", dht_properties.humidity, dataLight);
                        I2C_LCD_GotoXY(0, 0);
                        I2C_LCD_Puts(row_one_lcd);
                        I2C_LCD_GotoXY(0, 1);
                        I2C_LCD_Puts(row_two_lcd);
                    }
                    else if (cmd == BUTTON_0)
                    {
                        ESP_LOGI(TAG, " Đã ấn button 0");
                        Air_Conditioner_Control = 0;
                        // xEventGroupSetBits(s_mesh_network_event_group, AIR_CONTROLER); // xoá Eventbit
                    }
                    else if (cmd == BUTTON_STAR)
                    {
                        ESP_LOGI(TAG, " Đã ấn button MODE");
                        I2C_LCD_Clear();
                        xEventGroupSetBits(g_control_event_group, CLOCK_TASK_RUN);
                        // switch (State_LCD)
                        // {
                        // case NORMAL_LCD:
                        //     State_LCD = UP_SETUP_LCD;
                        //     break;
                        // case UP_SETUP_LCD:
                        //     State_LCD = DOWN_SETUP_LCD;
                        //     break;
                        // case DOWN_SETUP_LCD:
                        //     State_LCD = OFF_SETUP_LCD;
                        //     break;
                        // case OFF_SETUP_LCD:
                        //     State_LCD = ONOFF_SETUP_LCD;
                        //     break;
                        // case ONOFF_SETUP_LCD:
                        //     State_LCD = NORMAL_LCD;
                        //     break;
                        // default:
                        //     break;
                        // }
                        // xEventGroupSetBits(g_control_event_group, SET_COMMAND_RECIVE_MESSAGE);
                    }
                    else if (cmd == BUTTON_THANG)
                    {
                        ESP_LOGI(TAG, " Đã ấn button THANG");
                        if (panasonic_command_t.on == true)
                        {
                            panasonic_command_t.on = false;
                            panasonic_transmit(&panasonic_command_t);
                            sprintf(dataTxMesh, "aOnOff 0");
                            esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                               ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
                            nvs_save_Info(my_handler, NVS_KEY_AIR, &panasonic_command_t, sizeof(panasonic_command_t));
                        }
                    }
                    else if (cmd == BUTTON_UP)
                    {
                        ESP_LOGI(TAG, " Đã ấn button UP");
                        if (panasonic_command_t.temp < 30)
                        {
                            panasonic_command_t.temp = panasonic_command_t.temp + 1;
                            panasonic_transmit(&panasonic_command_t);
                            sprintf(dataTxMesh, "aChange %d", panasonic_command_t.temp);
                            esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                               ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
                            nvs_save_Info(my_handler, NVS_KEY_AIR, &panasonic_command_t, sizeof(panasonic_command_t));
                        }
                    }
                    else if (cmd == BUTTON_DOWN)
                    {
                        ESP_LOGI(TAG, " Đã ấn button DOWN");
                        if (panasonic_command_t.temp > 16)
                        {
                            panasonic_command_t.temp = panasonic_command_t.temp - 1;
                            panasonic_transmit(&panasonic_command_t);
                            sprintf(dataTxMesh, "aChange %d", panasonic_command_t.temp);
                            esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                               ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
                            nvs_save_Info(my_handler, NVS_KEY_AIR, &panasonic_command_t, sizeof(panasonic_command_t));
                        }
                    }
                    else if (cmd == BUTTON_OK)
                    {
                        ESP_LOGI(TAG, " Đã ấn button OK");
                        panasonic_command_t.on = true;
                        panasonic_transmit(&panasonic_command_t);
                        sprintf(dataTxMesh, "aOnOff 1");
                        esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                           ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
                        nvs_save_Info(my_handler, NVS_KEY_AIR, &panasonic_command_t, sizeof(panasonic_command_t));
                    }
                    else if (cmd == BUTTON_LEFT)
                    {
                        ESP_LOGI(TAG, " Đã ấn button LEFT");
                    }
                    else if (cmd == BUTTON_RIGHT)
                    {
                        ESP_LOGI(TAG, " Đã ấn button RIGHT");
                    }
                }
            }
            // free up data space
            vRingbufferReturnItem(rb, (void *)items);
        }
        // xEventGroupClearBits(g_control_event_group, INFRATE_RECIVE_MESSAGE);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void getClock(void *pvParameters)
{
    // Initialize RTC
    i2c_dev_t dev;
    if (ds1307_init_desc(&dev, I2C_NUM_0, 21, 22) != ESP_OK)
    {
        ESP_LOGE(pcTaskGetName(0), "Could not init device descriptor.");
        while (1)
        {
            vTaskDelay(1);
        }
    }
    struct Alarm timeCurrentAlarm;
    int isAlarm = 0;
    while (1)
    {
        xEventGroupWaitBits(g_control_event_group, // đợi đến khi có tin nhắn từ node gửi đến
                            CLOCK_TASK_RUN,
                            pdFALSE,
                            pdFALSE,
                            portMAX_DELAY);
        struct tm time;
        if (ds1307_get_time(&dev, &time) != ESP_OK)
        {
            ESP_LOGE(pcTaskGetName(0), "Could not get time.");
            while (1)
            {
                vTaskDelay(1);
            }
        }
        // I2C_LCD_Clear();
        sprintf(timeCurrentAlarm.time, "%02d:%02d:%02d", time.tm_hour, time.tm_min, time.tm_sec);
        sprintf(timeCurrentAlarm.date, "%02d/%02d/%04d", time.tm_mday, time.tm_mon + 1, time.tm_year);
        I2C_LCD_GotoXY(0, 0);
        I2C_LCD_Puts(timeCurrentAlarm.time);
        I2C_LCD_GotoXY(0, 1);
        I2C_LCD_Puts(timeCurrentAlarm.date);
        // printf("Alarm:%d %s %d %s %s\n, heapsize:%d\n", alarms[0].alarmID, alarms[0].device, alarms[0].state, alarms[0].time, alarms[0].date, heapSize);
        isAlarm = compareDateTime(&alarms[0], &timeCurrentAlarm);
        if (time.tm_min == 31)
        {
            esp_restart();
        }
        if ((isAlarm == -1 || isAlarm == 0) && heapSize >= 0)
        {
            // printf("Da den alarm\n");
            if (strstr(alarms[0].device, "led1") != NULL)
            {
                if (alarms[0].state == 1)
                {
                    Sensor_State.led1 = 1;
                    sprintf(dataTxMesh, "led1 1");
                    gpio_set_level(RL1_OUT_PIN, 0);
                    nvs_save_Info(my_handler, NVS_KEY_STATE, &Sensor_State, sizeof(Sensor_State));
                }
                else
                {
                    Sensor_State.led1 = 0;
                    gpio_set_level(RL1_OUT_PIN, 1);
                    sprintf(dataTxMesh, "led1 0");
                    nvs_save_Info(my_handler, NVS_KEY_STATE, &Sensor_State, sizeof(Sensor_State));
                }
            }
            else if (strstr(alarms[0].device, "led2") != NULL)
            {
                if (alarms[0].state == 1)
                {
                    Sensor_State.led2 = 1;
                    sprintf(dataTxMesh, "led2 1");
                    gpio_set_level(RL2_OUT_PIN, 0);
                    nvs_save_Info(my_handler, NVS_KEY_STATE, &Sensor_State, sizeof(Sensor_State));
                }
                else
                {
                    Sensor_State.led2 = 0;
                    sprintf(dataTxMesh, "led2 0");
                    gpio_set_level(RL2_OUT_PIN, 1);
                    nvs_save_Info(my_handler, NVS_KEY_STATE, &Sensor_State, sizeof(Sensor_State));
                }
            }
            else if (strstr(alarms[0].device, "led3") != NULL)
            {
                if (alarms[0].state == 1)
                {
                    Sensor_State.led3 = 1;
                    sprintf(dataTxMesh, "led3 1");
                    gpio_set_level(RL3_OUT_PIN, 0);
                    nvs_save_Info(my_handler, NVS_KEY_STATE, &Sensor_State, sizeof(Sensor_State));
                }
                else
                {
                    Sensor_State.led3 = 0;
                    sprintf(dataTxMesh, "led3 0");
                    gpio_set_level(RL3_OUT_PIN, 1);
                    nvs_save_Info(my_handler, NVS_KEY_STATE, &Sensor_State, sizeof(Sensor_State));
                }
            }
            else if (strstr(alarms[0].device, "led2") != NULL)
            {
                if (alarms[0].state == 1)
                {
                    Sensor_State.led4 = 1;
                    sprintf(dataTxMesh, "led4 1");
                    gpio_set_level(RL4_OUT_PIN, 0);
                    nvs_save_Info(my_handler, NVS_KEY_STATE, &Sensor_State, sizeof(Sensor_State));
                }
                else
                {
                    sprintf(dataTxMesh, "led4 0");
                    Sensor_State.led4 = 0;
                    gpio_set_level(RL4_OUT_PIN, 1);
                    nvs_save_Info(my_handler, NVS_KEY_STATE, &Sensor_State, sizeof(Sensor_State));
                }
            }
            else if (strstr(alarms[0].device, "air") != NULL)
            {
                if (alarms[0].state == 1)
                {
                    panasonic_command_t.on = true;
                    panasonic_transmit(&panasonic_command_t);
                    sprintf(dataTxMesh, "aOnOff 1");
                    nvs_save_Info(my_handler, NVS_KEY_AIR, &panasonic_command_t, sizeof(panasonic_command_t));
                }
                else
                {
                    panasonic_command_t.on = false;
                    panasonic_transmit(&panasonic_command_t);
                    sprintf(dataTxMesh, "aOnOff 0");
                    nvs_save_Info(my_handler, NVS_KEY_AIR, &panasonic_command_t, sizeof(panasonic_command_t));
                }
            }
            esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                               ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
            isAlarm = 1;
            deleteAlarm(&alarms, &heapSize, alarms[0].alarmID);
            nvs_save_Info(my_handler, NVS_KEY_ALARM, &alarms, sizeof(alarms));
            nvs_save_Info(my_handler, NVS_KEY_HEAPSIZE, &heapSize, sizeof(heapSize));
        }
        else
        {
            // printf("Chua den alarm\n");
        }
        // ESP_LOGI(pcTaskGetName(0), "%04d-%02d-%02d %02d:%02d:%02d",
        //          time.tm_year, time.tm_mon + 1,
        //          time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/**
 *  @brief Task này thực hiện Publish bản tin khi có tin nhắn gửi đến
 *
 */
void xTaskReceiveMessageNetwork(void *pvParameters)
{
    struct Alarm newAlarm;
    char *token = NULL;
    char *tokenDate = NULL;
    char *tokenTime1 = NULL;
    char *tokenTime2 = NULL;
    int mode_value = 0;
    int fan_value = 0;
    int swing_value = 0;
    int day, month, year, hour, minute, second;
    while (1)
    {
        xEventGroupWaitBits(s_mesh_network_event_group, // đợi đến khi có tin nhắn từ node gửi đến
                            MESH_MESSAGE_ARRIVE_BIT,
                            pdFALSE,
                            pdFALSE,
                            portMAX_DELAY);
        if (strstr(data_rx_mesh, "Period") != NULL)
        {
            if (strstr(data_rx_mesh, "temperature") != NULL)
            {
                token = strtok(data_rx_mesh, "Period temperature ");
                Sensor_Period.tPeriod = atoi(token);
                index1 = 1;
            }
            else if (strstr(data_rx_mesh, "humidity") != NULL)
            {
                token = strtok(data_rx_mesh, "Period humidity ");
                Sensor_Period.hPeriod = atoi(token);
                index1 = 1;
            }
            else if (strstr(data_rx_mesh, "light") != NULL)
            {
                token = strtok(data_rx_mesh, "Period light ");
                Sensor_Period.lPeriod = atoi(token);
                index2 = 1;
            }
            nvs_save_Info(my_handler, NVS_KEY_PERIOD, &Sensor_Period, sizeof(Sensor_Period));
        }
        else if (strstr(data_rx_mesh, "Air") != NULL)
        {
            strtok(data_rx_mesh, " ");
            token = strtok(NULL, " ");
            if (strstr(token, "onoff") != NULL)
            {
                char *value = strtok(NULL, " ");
                panasonic_command_t.on = (bool)atoi(value);
                panasonic_transmit(&panasonic_command_t);
            }
            else if (strstr(token, "change") != NULL)
            {
                char *value = strtok(NULL, " ");
                panasonic_command_t.temp = atoi(value);
                panasonic_transmit(&panasonic_command_t);
            }
            else if (strstr(token, "mode") != NULL)
            {
                char *value = strtok(NULL, " ");
                mode_value = atoi(value);
                if (mode_value == AIR_MODE_AUTO)
                {
                    panasonic_command_t.mode = MODE_AUTO;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_MODE_COOL)
                {
                    panasonic_command_t.mode = MODE_COOL;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_MODE_DRY)
                {
                    panasonic_command_t.mode = MODE_DRY;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_MODE_FAN)
                {
                    panasonic_command_t.mode = MODE_FAN;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_MODE_HEAT)
                {
                    panasonic_command_t.mode = MODE_HEAT;
                    panasonic_transmit(&panasonic_command_t);
                }
            }
            else if (strstr(token, "fan") != NULL)
            {
                char *value = strtok(NULL, " ");
                fan_value = atoi(value);
                if (mode_value == AIR_FAN_AUTO)
                {
                    panasonic_command_t.fan = FAN_AUTO;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_FAN_1)
                {
                    panasonic_command_t.fan = FAN_1;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_FAN_2)
                {
                    panasonic_command_t.fan = FAN_2;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_FAN_3)
                {
                    panasonic_command_t.fan = FAN_3;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_FAN_4)
                {
                    panasonic_command_t.fan = FAN_4;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_FAN_5)
                {
                    panasonic_command_t.fan = FAN_5;
                    panasonic_transmit(&panasonic_command_t);
                }
            }
            else if (strstr(token, "swing") != NULL)
            {
                char *value = strtok(NULL, " ");
                swing_value = atoi(value);
                if (mode_value == AIR_SWING_AUTO)
                {
                    panasonic_command_t.swing = SWING_AUTO;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_SWING_1)
                {
                    panasonic_command_t.swing = SWING_1;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_SWING_2)
                {
                    panasonic_command_t.swing = SWING_2;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_SWING_3)
                {
                    panasonic_command_t.swing = SWING_3;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_SWING_4)
                {
                    panasonic_command_t.swing = SWING_4;
                    panasonic_transmit(&panasonic_command_t);
                }
                else if (mode_value == AIR_SWING_5)
                {
                    panasonic_command_t.swing = SWING_5;
                    panasonic_transmit(&panasonic_command_t);
                }
            }
            nvs_save_Info(my_handler, NVS_KEY_AIR, &panasonic_command_t, sizeof(panasonic_command_t));
        }
        else if (strstr(data_rx_mesh, "Sync") != NULL)
        {
            strtok(NULL, " ");
            sprintf(dataTxMesh, "sync %d %d %d %d %d", Sensor_State.led1, Sensor_State.led2, Sensor_State.led3, Sensor_State.led4, (uint8_t)panasonic_command_t.on);
            esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                               ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
        }
        else if (strstr(data_rx_mesh, "RTC") != NULL)
        {
            xEventGroupClearBits(g_control_event_group, CLOCK_TASK_RUN);
            strtok(data_rx_mesh, " ");
            token = strtok(NULL, " ");
            tokenTime1 = strtok(NULL, " ");

            tokenDate = strtok(token, "/");
            day = atoi(tokenDate);
            tokenDate = strtok(NULL, "/");
            month = atoi(tokenDate);
            tokenDate = strtok(NULL, "/");
            year = atoi(tokenDate);

            tokenTime2 = strtok(tokenTime1, ":");
            hour = atoi(tokenTime2);
            tokenTime2 = strtok(NULL, ":");
            minute = atoi(tokenTime2);
            tokenTime2 = strtok(NULL, ":");
            second = atoi(tokenTime2);
            // sprintf(dataSync, "RTC %02d/%02d/%04d %02d:%02d:%02d", timeinfo_ntp.tm_mday, timeinfo_ntp.tm_mon, timeinfo_ntp.tm_year, timeinfo_ntp.tm_hour, timeinfo_ntp.tm_min, timeinfo_ntp.tm_sec);
            ds1307_init_desc(&dev, I2C_NUM_0, 21, 22);
            struct tm time_set = {
                .tm_year = year,
                .tm_mon = month, // 0-based
                .tm_mday = day,
                .tm_hour = hour,
                .tm_min = minute,
                .tm_sec = second};
            ds1307_set_time(&dev, &time_set);
            esp_restart();
            xEventGroupSetBits(g_control_event_group, CLOCK_TASK_RUN);

            printf("Dong Bo: %02d/%02d/%d %02d:%02d:%02d\n", day, month, year, hour, minute, second);
        }
        else if (strstr(data_rx_mesh, "State") != NULL)
        {
            strtok(data_rx_mesh, " ");
            token = strtok(NULL, " ");
            if (strstr(token, "led1") != NULL)
            {
                char *value = strtok(NULL, " ");
                Sensor_State.led1 = atoi(value);
                if (Sensor_State.led1 == 0)
                {
                    gpio_set_level(RL1_OUT_PIN, 1);
                }
                else
                {
                    gpio_set_level(RL1_OUT_PIN, 0);
                }
            }
            else if (strstr(token, "led2") != NULL)
            {
                char *value = strtok(NULL, " ");
                Sensor_State.led2 = atoi(value);
                if (Sensor_State.led2 == 0)
                {
                    gpio_set_level(RL2_OUT_PIN, 1);
                }
                else
                {
                    gpio_set_level(RL2_OUT_PIN, 0);
                }
            }
            else if (strstr(token, "led3") != NULL)
            {
                char *value = strtok(NULL, " ");
                Sensor_State.led3 = atoi(value);
                if (Sensor_State.led3 == 0)
                {
                    gpio_set_level(RL3_OUT_PIN, 1);
                }
                else
                {
                    gpio_set_level(RL3_OUT_PIN, 0);
                }
            }
            else if (strstr(token, "led4") != NULL)
            {
                char *value = strtok(NULL, " ");
                Sensor_State.led4 = atoi(value);
                if (Sensor_State.led4 == 0)
                {
                    gpio_set_level(RL4_OUT_PIN, 1);
                }
                else
                {
                    gpio_set_level(RL4_OUT_PIN, 0);
                }
            }
            nvs_save_Info(my_handler, NVS_KEY_STATE, &Sensor_State, sizeof(Sensor_State));
        }
        else if (strstr(data_rx_mesh, "Delete") != NULL)
        {
            strtok(data_rx_mesh, " ");
            token = strtok(NULL, " ");
            int id = token[6] - 48;
            deleteAlarm(alarms, &heapSize, id);
            nvs_save_Info(my_handler, NVS_KEY_ALARM, &alarms, sizeof(alarms));
            nvs_save_Info(my_handler, NVS_KEY_HEAPSIZE, &heapSize, sizeof(heapSize));
        }
        else if (strstr(data_rx_mesh, "Alarm") != NULL)
        {
            strtok(data_rx_mesh, " ");
            token = strtok(NULL, " ");
            newAlarm.alarmID = token[6] - 48;

            token = strtok(NULL, " ");
            strcpy(newAlarm.device, token);

            token = strtok(NULL, " ");
            newAlarm.state = token[0] - 48;

            token = strtok(NULL, " ");
            strcpy(newAlarm.date, token);

            token = strtok(NULL, " ");
            sprintf(newAlarm.time, "%s:00", token);

            printf("%s--%s--%d\n", newAlarm.date, newAlarm.time, heapSize);
            insertAlarm(alarms, &heapSize, newAlarm);
            nvs_save_Info(my_handler, NVS_KEY_ALARM, &alarms, sizeof(alarms));
            nvs_save_Info(my_handler, NVS_KEY_HEAPSIZE, &heapSize, sizeof(heapSize));
        }
        token = NULL;
        xEventGroupClearBits(s_mesh_network_event_group, MESH_MESSAGE_ARRIVE_BIT); // xoá Eventbit
        // vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

/**
 *  @brief Task này thực hiện Publish bản tin khi có tin nhắn gửi đến
 *
 */

void xTaskDataMeshTransmit(void *pvParameters)
{
    EventBits_t uxBits;
    while (1)
    {
        uxBits = xEventGroupWaitBits(s_mesh_network_event_group, // đợi đến khi có tin nhắn từ node gửi đến
                                     MESH_DATA_TRANSMIT,
                                     pdFALSE,
                                     pdFALSE,
                                     portMAX_DELAY);
        if ((uxBits & MESH_DATA_TRANSMIT) != 0)
        {
            esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                               ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
            // esp_timer_stop(periodic_mesh_timer);
            // esp_timer_start_periodic(periodic_mesh_timer, TIMER_PER_MINUTE / 50); // Function Callback được chạy mỗi 2ms
        }
        xEventGroupClearBits(s_mesh_network_event_group, MESH_DATA_TRANSMIT); // xoá Eventbit
    }
    vTaskDelete(NULL);
}

uint8_t dht11_callback = 0;
uint8_t light_callback = 0;
void xTaskHandleSensor(void *pvParameters)
{
    EventBits_t uxBits;
    while (1)
    {
        uxBits = xEventGroupWaitBits(s_mesh_network_event_group, // đợi đến khi có tin nhắn từ node gửi đến
                                     SENSOR_CALLBACK,
                                     pdFALSE,
                                     pdFALSE,
                                     portMAX_DELAY);
        if ((uxBits & SENSOR_CALLBACK) != 0)
        {
            printf("Đã vào xTaskHandleSensor\n");
            if (dht11_callback == 1)
            {
                dht11_callback = 0;
                // I2C_LCD_Clear();
                dht_properties = DHT11_read();
                // bh1750_i2c_read_data(dev_1, &dataLight);
                // sprintf(row_one_lcd, "Nhiet Do:%d.%d oC", dht_properties.temperature, dht_properties.dotTemperature);
                // sprintf(row_two_lcd, "DoAm:%d%% Lux:%d", dht_properties.humidity, dataLight);
                // I2C_LCD_GotoXY(0, 0);
                // I2C_LCD_Puts(row_one_lcd);
                // I2C_LCD_GotoXY(0, 1);
                // I2C_LCD_Puts(row_two_lcd);
                memset(dataTxMesh, 0, 30);
                sprintf(dataTxMesh, "tem %d.%d hum %d", dht_properties.temperature, dht_properties.dotTemperature, dht_properties.humidity);
                esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                   ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
            }
            else if (light_callback == 1)
            {
                light_callback = 0;
                // I2C_LCD_Clear();
                bh1750_i2c_read_data(dev_1, &dataLight);
                // sprintf(row_one_lcd, "Nhiet Do:%d.%d oC", dht_properties.temperature, dht_properties.dotTemperature);
                // sprintf(row_two_lcd, "DoAm:%d%% Lux:%d", dht_properties.humidity, dataLight);
                // I2C_LCD_GotoXY(0, 0);
                // I2C_LCD_Puts(row_one_lcd);
                // I2C_LCD_GotoXY(0, 1);
                // I2C_LCD_Puts(row_two_lcd);
                memset(dataTxMesh, 0, 30);
                sprintf(dataTxMesh, "light %d", dataLight);
                esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                                   ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
            }
        }
        xEventGroupClearBits(s_mesh_network_event_group, SENSOR_CALLBACK); // xoá Eventbit
    }
    vTaskDelete(NULL);
}

void Timer_Callback(void *pvParameter) // task xu ly hien thi led 7segment
{
    printf("Đã vào Timer_Callback\n");
    dht11_callback = 1;
    xEventGroupSetBits(s_mesh_network_event_group, SENSOR_CALLBACK); // xoá Eventbit
    // I2C_LCD_Clear();
    // dht_properties = DHT11_read();
    // bh1750_i2c_read_data(dev_1, &dataLight);
    // sprintf(row_one_lcd, "Nhiet Do:%d.%d oC", dht_properties.temperature, dht_properties.dotTemperature);
    // sprintf(row_two_lcd, "DoAm:%d%% Lux:%d", dht_properties.humidity, dataLight);
    // I2C_LCD_GotoXY(0, 0);
    // I2C_LCD_Puts(row_one_lcd);
    // I2C_LCD_GotoXY(0, 1);
    // I2C_LCD_Puts(row_two_lcd);
    // if (index1 == 1)
    {
        index1 = 0;
        esp_timer_stop(periodic_timer);
        esp_timer_start_periodic(periodic_timer, Sensor_Period.tPeriod * TIMER_PER_MINUTE + 21);
    }
    // memset(dataTxMesh, 0, 30);
    // sprintf(dataTxMesh, "tem %d.%d hum %d", dht_properties.temperature, dht_properties.dotTemperature, dht_properties.humidity);
    // esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
    //                                    ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
}
void Timer_Light_Callback(void *pvParameter) // task xu ly hien thi led 7segment
{
    printf("Đã vào Timer_Light_Callback\n");
    light_callback = 1;
    xEventGroupSetBits(s_mesh_network_event_group, SENSOR_CALLBACK); // xoá Eventbit
    // I2C_LCD_Clear();
    // bh1750_i2c_read_data(dev_1, &dataLight);
    // sprintf(row_one_lcd, "Nhiet Do:%d.%d oC", dht_properties.temperature, dht_properties.dotTemperature);
    // sprintf(row_two_lcd, "DoAm:%d%% Lux:%d", dht_properties.humidity, dataLight);
    // I2C_LCD_GotoXY(0, 0);
    // I2C_LCD_Puts(row_one_lcd);
    // I2C_LCD_GotoXY(0, 1);
    // I2C_LCD_Puts(row_two_lcd);
    if (index2 == 1)
    {
        index2 = 0;
        esp_timer_stop(periodic_light_timer);
        esp_timer_start_periodic(periodic_light_timer, Sensor_Period.lPeriod * TIMER_PER_MINUTE + 22);
    }
    // memset(dataTxMesh, 0, 30);
    // sprintf(dataTxMesh, "light %d", dataLight);
    // esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
    //                                    ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(dataTxMesh), (uint8_t *)dataTxMesh);
}

/*=================================RTC============================*/

void Ble_Mesh_Init(void)
{
    esp_err_t err;
    /*Khởi tạo Blutooth Controller*/
    err = bluetooth_init();
    if (err)
    {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }
    err = ble_mesh_nvs_open(&NVS_HANDLE); // lấy lại thông tin cấp phép nếu có
    if (err)
    {
        return;
    }
    ble_mesh_get_dev_uuid(dev_uuid); // tạo UUID cho sensor node

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err)
    {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }
}

// void System_Init(void)
// {
//     esp_err_t err;
//     /*Khởi tạo GPIO, ADC, NVS_Flash*/
//     ESP_LOGI(TAG, "Initializing...");
//     err = nvs_flash_init();
//     if (err == ESP_ERR_NVS_NO_FREE_PAGES)
//     {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         err = nvs_flash_init();
//     }

//     nvs_get_Info(my_handler, NVS_KEY_AIR, &panasonic_command_t);
//     nvs_get_Info(my_handler, NVS_KEY_STATE, &Sensor_State);
//     /*====================== EVENT GROUP INIT =================================*/
//     s_mesh_network_event_group = xEventGroupCreate();
//     g_control_event_group = xEventGroupCreate();
//     /*====================== SR505 INIT =================================*/
//     //  /*====================== RMT INIT =================================*/
//     vTaskDelay(pdMS_TO_TICKS(100));
//     xTaskCreate(xTaskHandelInfrated, "xTaskHandelInfrated", 6096, NULL, 9, NULL);
// }

void System_Init(void)
{
    esp_err_t err;
    /*Khởi tạo GPIO, ADC, NVS_Flash*/
    ESP_LOGI(TAG, "Initializing...");
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    // Thêm báo thức vào Min-Heap
    // struct Alarm newAlarm1 = {1, "led1", 1, "22:55:30", "19/11/2023"};
    // struct Alarm newAlarm2 = {2, "led2", 1, "22:57:00", "19/11/2023"};
    // struct Alarm newAlarm3 = {3, "led3", 1, "23:00:00", "19/11/2023"};

    // insertAlarm(alarms, &heapSize, newAlarm1);
    // insertAlarm(alarms, &heapSize, newAlarm2);
    // insertAlarm(alarms, &heapSize, newAlarm3);

    // nvs_save_Info(my_handler, NVS_KEY_ALARM, &alarms, sizeof(alarms));
    // nvs_save_Info(my_handler, NVS_KEY_HEAPSIZE, &heapSize, sizeof(heapSize));

    // Sensor_Period.tPeriod = 5;
    // Sensor_Period.lPeriod = 3;
    // Sensor_Period.hPeriod = 5;

    // nvs_save_Info(my_handler, NVS_KEY_PERIOD, &Sensor_Period, sizeof(Sensor_Period));
    // nvs_save_Info(my_handler, NVS_KEY_AIR, &panasonic_command_t, sizeof(panasonic_command_t));

    nvs_get_Info(my_handler, NVS_KEY_AIR, &panasonic_command_t);
    nvs_get_Info(my_handler, NVS_KEY_PERIOD, &Sensor_Period);
    nvs_get_Info(my_handler, NVS_KEY_ALARM, &alarms);
    nvs_get_Info(my_handler, NVS_KEY_HEAPSIZE, &heapSize);
    // heapSize = 0;
    //  deleteAlarm(&alarms, &heapSize, 2);
    //  nvs_save_Info(my_handler, NVS_KEY_ALARM, &alarms, sizeof(alarms));
    //  nvs_save_Info(my_handler, NVS_KEY_HEAPSIZE, &heapSize, sizeof(heapSize));
    printf("heapSize: %d\n", heapSize);
    printf("Alarm:%d %s %d %s %s\n", alarms[0].alarmID, alarms[0].device, alarms[0].state, alarms[0].time, alarms[0].date);
    // Sensor_State.led1 = 0;
    // Sensor_State.led2 = 0;
    // Sensor_State.led3 = 0;
    // Sensor_State.led4 = 0;
    // nvs_save_Info(my_handler, NVS_KEY_STATE, &Sensor_State, sizeof(Sensor_State));
    Ble_Mesh_Init();

    nvs_get_Info(my_handler, NVS_KEY_STATE, &Sensor_State);
    /*====================== EVENT GROUP INIT =================================*/
    s_mesh_network_event_group = xEventGroupCreate();
    g_control_event_group = xEventGroupCreate();
    /*====================== SR505 INIT =================================*/
    gpio_set_direction(SR505_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SR505_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(SR505_PIN, GPIO_INTR_ANYEDGE); // Cấu hình ngắt ngoài cho nút IO0
    /*====================== RELAY INIT =================================*/
    gpio_set_direction(RL1_OUT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RL2_OUT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RL3_OUT_PIN, GPIO_MODE_OUTPUT);
    // gpio_set_direction(RL4_OUT_PIN, GPIO_MODE_OUTPUT);

    gpio_set_level(RL1_OUT_PIN, 1 - Sensor_State.led1);
    gpio_set_level(RL2_OUT_PIN, 1 - Sensor_State.led2);
    gpio_set_level(RL3_OUT_PIN, 1 - Sensor_State.led3);
    // gpio_set_level(RL4_OUT_PIN, 1 - Sensor_State.led4);
    gpio_set_pull_mode(RL1_OUT_PIN, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(RL2_OUT_PIN, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(RL3_OUT_PIN, GPIO_PULLUP_ONLY);
    // gpio_set_pull_mode(RL4_OUT_PIN, GPIO_PULLUP_ONLY);
    input_create(BUTTON_RL1_PIN);
    input_create(BUTTON_RL2_PIN);
    input_create(BUTTON_RL3_PIN);
    // input_create(BUTTON_RL4_PIN);
    gpio_set_intr_type(BUTTON_RL1_PIN, GPIO_INTR_POSEDGE); // Cấu hình ngắt ngoài cho nút IO0
    gpio_set_intr_type(BUTTON_RL2_PIN, GPIO_INTR_POSEDGE); // Cấu hình ngắt ngoài cho nút IO0
    gpio_set_intr_type(BUTTON_RL3_PIN, GPIO_INTR_POSEDGE); // Cấu hình ngắt ngoài cho nút IO0
    // gpio_set_intr_type(BUTTON_RL4_PIN, GPIO_INTR_POSEDGE); // Cấu hình ngắt ngoài cho nút IO0
    interputQueue = xQueueCreate(1, sizeof(int)); // sử dụng queue cho ngắt
    // xTaskCreateStatic(&Show_Backup_Task, "Show_Backup_Task", 1024, NULL, 1, xStackInterrupt, &xTaskInterrupt);
    xTaskCreate(&Show_Backup_Task, "Show_Backup_Task", 2000, NULL, 5, NULL);               // task thực hiện ngắt ngoài
    gpio_install_isr_service(0);                                                           // khởi tạo ISR Service cho ngắt
    gpio_isr_handler_add(BUTTON_RL1_PIN, gpio_interrupt_handler, (void *)BUTTON_RL1_PIN);  // đăng ký hàm handler cho ngắt ngoài
    gpio_isr_handler_add(BUTTON_RL2_PIN, gpio_interrupt_handler2, (void *)BUTTON_RL2_PIN); // đăng ký hàm handler cho ngắt ngoài
    gpio_isr_handler_add(BUTTON_RL3_PIN, gpio_interrupt_handler3, (void *)BUTTON_RL3_PIN); // đăng ký hàm handler cho ngắt ngoài
    // gpio_isr_handler_add(BUTTON_RL4_PIN, gpio_interrupt_handler4, (void *)BUTTON_RL4_PIN); // đăng ký hàm handler cho ngắt ngoài
    gpio_isr_handler_add(SR505_PIN, gpio_interrupt_SR505_handler, (void *)SR505_PIN);
    /*====================== DHT11 INIT =================================*/
    DHT11_init(DHT11_PIN);
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &Timer_Callback, // CallBack function
        .name = "periodic"};
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, Sensor_Period.tPeriod * TIMER_PER_MINUTE + 21); // Function Callback được chạy mỗi 2ms

    /*====================== DS1307 INIT =================================*/
    I2C_LCD_Init();
    I2C_LCD_BackLight(1);
    // vTaskDelay(pdMS_TO_TICKS(2000));
    // ds1307_init_desc(&dev, I2C_NUM_0, 21, 22);
    // struct tm time_set = {
    //     .tm_year = 24 + 2000,
    //     .tm_mon = 0, // 0-based
    //     .tm_mday = 6,
    //     .tm_hour = 14,
    //     .tm_min = 23,
    //     .tm_sec = 10};
    // ds1307_set_time(&dev, &time_set);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xEventGroupSetBits(g_control_event_group, CLOCK_TASK_RUN);
    xTaskCreate(getClock, "getClock", 2048 * 3, NULL, 10, NULL);
    //  // /*====================== BH1750 INIT =================================*/
    bh1750_i2c_hal_init(&dev_1);
    const esp_timer_create_args_t periodic_light_timer_args = {
        .callback = &Timer_Light_Callback, // CallBack function
        .name = "periodicLight"};
    esp_timer_create(&periodic_light_timer_args, &periodic_light_timer);
    esp_timer_start_periodic(periodic_light_timer, Sensor_Period.lPeriod * TIMER_PER_MINUTE + 22); // Function Callback được chạy mỗi 2ms
    //  /*====================== RMT INIT =================================*/
    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreate(xTaskHandleSensor, "xTaskHandleSensor", 3096, NULL, 9, NULL);
    xTaskCreate(xTaskHandelInfrated, "xTaskHandelInfrated", 6096, NULL, 9, NULL);
    //  //================BLE Initalize===============================
    vTaskDelay(pdMS_TO_TICKS(100));
    // // xTaskCreate(xTaskReceiveMessageNetwork, "xTaskReceiveMessageNetwork", 2048 * 3, NULL, 2, NULL);
    xTaskCreateStatic(xTaskReceiveMessageNetwork, "xTaskReceiveMessageNetwork", TASK_WS_STACK_SIZE, NULL, 9, xStackWs, &xTaskWsHandle);
    // xTaskCreateStatic(xTaskDataMeshTransmit, "xTaskDataMeshTransmit", TASK_NETWORK_STACK_SIZE, NULL, 10, xStackNetwork, &xTaskNetworkHandle);
    //  // xTaskCreate(&xTaskDataMeshTransmit, "xTaskDataMeshTransmit", 2516, NULL, 10, NULL); // task thực hiện gửi lại bản tin nếu thất bại
}