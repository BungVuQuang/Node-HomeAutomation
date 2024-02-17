#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "rom/ets_sys.h"
#include <esp_log.h>
#include "HD44780.h"

#define SDA_PIN 21
#define SCL_PIN 22

// Pin mappings
// P0 -> RS
// P1 -> RW
// P2 -> E
// P3 -> Backlight
// P4 -> D4
// P5 -> D5
// P6 -> D6
// P7 -> D7
static esp_err_t I2C_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_flags = 0,
        .master.clk_speed = 400000};
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    return ESP_OK;
}

static uint8_t u8LCD_Buff[8]; // bo nho dem luu lai toan bo
static uint8_t u8LcdTmp;

#define MODE_4_BIT 0x28
#define CLR_SCR 0x01
#define DISP_ON 0x0C
#define CURSOR_ON 0x0E
#define CURSOR_HOME 0x80

static void I2C_LCD_Write_4bit(uint8_t u8Data);
static void I2C_LCD_FlushVal(void);
static void I2C_LCD_WriteCmd(uint8_t u8Cmd);

void I2C_LCD_Delay_Ms(uint8_t timeDelay)
{
    ets_delay_us(timeDelay * 1000);
}

void I2C_LCD_FlushVal(void)
{
    uint8_t i;

    for (i = 0; i < 8; ++i)
    {
        u8LcdTmp >>= 1;
        if (u8LCD_Buff[i])
        {
            u8LcdTmp |= 0x80;
        }
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, I2C_LCD_ADDR | I2C_MASTER_WRITE, 1));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, u8LcdTmp, 1));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 2000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void I2C_LCD_Init(void)
{
    uint8_t i;

    I2C_LCD_Delay_Ms(50);

    I2C_init();

    for (i = 0; i < 8; ++i)
    {
        u8LCD_Buff[i] = 0;
    }

    I2C_LCD_FlushVal();

    u8LCD_Buff[LCD_RS] = 0;
    I2C_LCD_FlushVal();

    u8LCD_Buff[LCD_RW] = 0;
    I2C_LCD_FlushVal();

    I2C_LCD_Write_4bit(0x03);
    I2C_LCD_Delay_Ms(5);

    I2C_LCD_Write_4bit(0x03);
    I2C_LCD_Delay_Ms(1);

    I2C_LCD_Write_4bit(0x03);
    I2C_LCD_Delay_Ms(1);

    I2C_LCD_Write_4bit(MODE_4_BIT >> 4);
    I2C_LCD_Delay_Ms(1);

    I2C_LCD_WriteCmd(MODE_4_BIT);
    I2C_LCD_WriteCmd(DISP_ON);
    //I2C_LCD_WriteCmd(CURSOR_ON);
    I2C_LCD_WriteCmd(CLR_SCR);
}

void I2C_LCD_Write_4bit(uint8_t u8Data)
{
    // 4 bit can ghi chinh la 4 5 6 7
    // dau tien gan LCD_E=1
    // ghi du lieu
    // sau do gan LCD_E=0

    if (u8Data & 0x08)
    {
        u8LCD_Buff[LCD_D7] = 1;
    }
    else
    {
        u8LCD_Buff[LCD_D7] = 0;
    }
    if (u8Data & 0x04)
    {
        u8LCD_Buff[LCD_D6] = 1;
    }
    else
    {
        u8LCD_Buff[LCD_D6] = 0;
    }
    if (u8Data & 0x02)
    {
        u8LCD_Buff[LCD_D5] = 1;
    }
    else
    {
        u8LCD_Buff[LCD_D5] = 0;
    }
    if (u8Data & 0x01)
    {
        u8LCD_Buff[LCD_D4] = 1;
    }
    else
    {
        u8LCD_Buff[LCD_D4] = 0;
    }

    u8LCD_Buff[LCD_EN] = 1;
    I2C_LCD_FlushVal();

    u8LCD_Buff[LCD_EN] = 0;
    I2C_LCD_FlushVal();
}

// void LCD_WaitBusy(void)
// {
// 	char temp;

// 	//dau tien ghi tat ca 4 bit thap bang 1
// 	u8LCD_Buff[LCD_D4] = 1;
// 	u8LCD_Buff[LCD_D5] = 1;
// 	u8LCD_Buff[LCD_D6] = 1;
// 	u8LCD_Buff[LCD_D7] = 1;
// 	I2C_LCD_FlushVal();

// 	u8LCD_Buff[LCD_RS] = 0;
// 	I2C_LCD_FlushVal();

// 	u8LCD_Buff[LCD_RW] = 1;
// 	I2C_LCD_FlushVal();

// 	do {
// 		u8LCD_Buff[LCD_EN] = 1;
// 		I2C_LCD_FlushVal();
// 		I2C_Read(I2C_LCD_ADDR + 1, &temp, 1);

// 		u8LCD_Buff[LCD_EN] = 0;
// 		I2C_LCD_FlushVal();
// 		u8LCD_Buff[LCD_EN] = 1;
// 		I2C_LCD_FlushVal();
// 		u8LCD_Buff[LCD_EN] = 0;
// 		I2C_LCD_FlushVal();
// 	} while (temp & 0x08);
// }

void I2C_LCD_WriteCmd(uint8_t u8Cmd)
{

    I2C_LCD_Delay_Ms(1);

    u8LCD_Buff[LCD_RS] = 0;
    I2C_LCD_FlushVal();

    u8LCD_Buff[LCD_RW] = 0;
    I2C_LCD_FlushVal();

    I2C_LCD_Write_4bit(u8Cmd >> 4);
    I2C_LCD_Write_4bit(u8Cmd);
}

void LCD_Write_Chr(char chr)
{

    I2C_LCD_Delay_Ms(1);
    u8LCD_Buff[LCD_RS] = 1;
    I2C_LCD_FlushVal();
    u8LCD_Buff[LCD_RW] = 0;
    I2C_LCD_FlushVal();
    I2C_LCD_Write_4bit(chr >> 4);
    I2C_LCD_Write_4bit(chr);
}

void I2C_LCD_Puts(char *sz)
{

    while (1)
    {
        if (*sz)
        {
            LCD_Write_Chr(*sz++);
        }
        else
        {
            break;
        }
    }
}

void I2C_LCD_Clear(void)
{

    I2C_LCD_WriteCmd(CLR_SCR);
}

void I2C_LCD_NewLine(void)
{

    I2C_LCD_WriteCmd(0xc0);
}

void I2C_LCD_GotoXY(unsigned char x, unsigned char y)
{
    unsigned char address;
    if (!y)
        address = (0x80 + x);
    else
        address = (0xC0 + x);
    I2C_LCD_WriteCmd(address);
}
void I2C_LCD_BackLight(uint8_t u8BackLight)
{

    if (u8BackLight)
    {
        u8LCD_Buff[LCD_BL] = 1;
    }
    else
    {
        u8LCD_Buff[LCD_BL] = 0;
    }
    I2C_LCD_FlushVal();
}

// void LCD_init(uint8_t addr, uint8_t dataPin, uint8_t clockPin, uint8_t cols, uint8_t rows)
// {
//     LCD_addr = addr;
//     SDA_pin = dataPin;
//     SCL_pin = clockPin;
//     LCD_cols = cols;
//     LCD_rows = rows;
//     I2C_init();
//     vTaskDelay(100 / portTICK_RATE_MS);                                 // Initial 40 mSec delay

//     // Reset the LCD controller
//     LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND);                   // First part of reset sequence
//     vTaskDelay(10 / portTICK_RATE_MS);                                  // 4.1 mS delay (min)
//     LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND);                   // second part of reset sequence
//     ets_delay_us(200);                                                  // 100 uS delay (min)
//     LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND);                   // Third time's a charm
//     LCD_writeNibble(LCD_FUNCTION_SET_4BIT, LCD_COMMAND);                // Activate 4-bit mode
//     ets_delay_us(80);                                                   // 40 uS delay (min)

//     // --- Busy flag now available ---
//     // Function Set instruction
//     LCD_writeByte(LCD_FUNCTION_SET_4BIT, LCD_COMMAND);                  // Set mode, lines, and font
//     ets_delay_us(80);

//     // Clear Display instruction
//     LCD_writeByte(LCD_CLEAR, LCD_COMMAND);                              // clear display RAM
//     vTaskDelay(2 / portTICK_RATE_MS);                                   // Clearing memory takes a bit longer

//     // Entry Mode Set instruction
//     LCD_writeByte(LCD_ENTRY_MODE, LCD_COMMAND);                         // Set desired shift characteristics
//     ets_delay_us(80);

//     LCD_writeByte(LCD_DISPLAY_ON, LCD_COMMAND);                         // Ensure LCD is set to on
// }

// void LCD_setCursor(uint8_t col, uint8_t row)
// {
//     if (row > LCD_rows - 1) {
//         ESP_LOGE(tag, "Cannot write to row %d. Please select a row in the range (0, %d)", row, LCD_rows-1);
//         row = LCD_rows - 1;
//     }
//     uint8_t row_offsets[] = {LCD_LINEONE, LCD_LINETWO, LCD_LINETHREE, LCD_LINEFOUR};
//     LCD_writeByte(LCD_SET_DDRAM_ADDR | (col + row_offsets[row]), LCD_COMMAND);
// }

// void LCD_writeChar(char c)
// {
//     LCD_writeByte(c, LCD_WRITE);                                        // Write data to DDRAM
// }

// void LCD_writeStr(char* str)
// {
//     while (*str) {
//         LCD_writeChar(*str++);
//     }
// }

// void LCD_home(void)
// {
//     LCD_writeByte(LCD_HOME, LCD_COMMAND);
//     vTaskDelay(2 / portTICK_RATE_MS);                                   // This command takes a while to complete
// }

// void LCD_clearScreen(void)
// {
//     LCD_writeByte(LCD_CLEAR, LCD_COMMAND);
//     vTaskDelay(2 / portTICK_RATE_MS);                                   // This command takes a while to complete
// }

// static void LCD_writeNibble(uint8_t nibble, uint8_t mode)
// {
//     uint8_t data = (nibble & 0xF0) | mode | LCD_BACKLIGHT;
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//     ESP_ERROR_CHECK(i2c_master_start(cmd));
//     ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (LCD_addr << 1) | I2C_MASTER_WRITE, 1));
//     ESP_ERROR_CHECK(i2c_master_write_byte(cmd, data, 1));
//     ESP_ERROR_CHECK(i2c_master_stop(cmd));
//     ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS));
//     i2c_cmd_link_delete(cmd);

//     LCD_pulseEnable(data);                                              // Clock data into LCD
// }

// static void LCD_writeByte(uint8_t data, uint8_t mode)
// {
//     LCD_writeNibble(data & 0xF0, mode);
//     LCD_writeNibble((data << 4) & 0xF0, mode);
// }

// static void LCD_pulseEnable(uint8_t data)
// {
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//     ESP_ERROR_CHECK(i2c_master_start(cmd));
//     ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (LCD_addr << 1) | I2C_MASTER_WRITE, 1));
//     ESP_ERROR_CHECK(i2c_master_write_byte(cmd, data | LCD_ENABLE, 1));
//     ESP_ERROR_CHECK(i2c_master_stop(cmd));
//     ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS));
//     i2c_cmd_link_delete(cmd);
//     ets_delay_us(1);

//     cmd = i2c_cmd_link_create();
//     ESP_ERROR_CHECK(i2c_master_start(cmd));
//     ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (LCD_addr << 1) | I2C_MASTER_WRITE, 1));
//     ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (data & ~LCD_ENABLE), 1));
//     ESP_ERROR_CHECK(i2c_master_stop(cmd));
//     ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS));
//     i2c_cmd_link_delete(cmd);
//     ets_delay_us(500);
// }