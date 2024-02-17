/**
 ******************************************************************************
 * @file		clock_rtc.c
 * @author	Vu Quang Bung
 * @date		26 June 2023
 ******************************************************************************
 **/
/*-----------------------------------------------------------------------------*/
/* Header inclusions */
/*-----------------------------------------------------------------------------*/
#include "clock_rtc.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant definitions */
/*-----------------------------------------------------------------------------*/
static const char *TAG = "RTC";
/*-----------------------------------------------------------------------------*/
/* Local Macro definitions */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Data type definitions */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Global variables */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Function prototypes */
/*-----------------------------------------------------------------------------*/
void write_rtc_time(const struct tm *timeinfo);
void Get_current_date_time(void);
/*-----------------------------------------------------------------------------*/
/* Function implementations */
/*-----------------------------------------------------------------------------*/
void read_rtc_time(struct tm *timeinfo)
{
    esp_err_t ret = nvs_open("rtc_data", NVS_READONLY, &nvs_handle_1);
    if (ret == ESP_OK)
    {
        size_t required_size;
        ret = nvs_get_blob(nvs_handle_1, "timeinfo", NULL, &required_size);
        if (ret == ESP_OK && required_size == sizeof(struct tm))
        {
            ret = nvs_get_blob(nvs_handle_1, "timeinfo", timeinfo, &required_size);
        }
        nvs_close(nvs_handle_1);
    }
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read RTC time: %s", esp_err_to_name(ret));
    }
}

void write_rtc_time(const struct tm *timeinfo)
{
    esp_err_t ret = nvs_open("rtc_data", NVS_READWRITE, &nvs_handle_1);
    if (ret == ESP_OK)
    {
        ret = nvs_set_blob(nvs_handle_1, "timeinfo", timeinfo, sizeof(struct tm));
        if (ret == ESP_OK)
        {
            ret = nvs_commit(nvs_handle_1);
        }
        nvs_close(nvs_handle_1);
    }
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write RTC time: %s", esp_err_to_name(ret));
    }
}
