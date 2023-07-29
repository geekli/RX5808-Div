#include "lcd.h"
#include "SPI.h"
#include "freertos/task.h"
#include <string.h>
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "../../lvgl.h"
#include "hwvers.h"

#define ST7735_LCD_BACKLIGHT_MIN 10
#define ST7735_LCD_BACKLIGHT_MAX 100

volatile uint8_t st7735_lcd_backlight = ST7735_LCD_BACKLIGHT_MAX;
volatile uint8_t st7735_lcd_inversion = false;

inline void LCD_Writ_Bus(uint8_t dat)
{

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); // Zero out the transaction
    t.length = 8;			  // Command is 8 bits
    t.tx_buffer = &dat;		  // The data is the cmd itself
    // t.user=(void*)1;                //D/C needs to be set to 0
    ret = spi_device_polling_transmit(my_spi, &t); // Transmit!
    assert(ret == ESP_OK);						   // Should have had no issues.
}

void LCD_WR_DATA8(uint8_t da)
{
    LCD_Writ_Bus(da);
}
void LCD_WR_DATA(uint16_t dat)
{
    LCD_Writ_Bus(dat >> 8);
    LCD_Writ_Bus(dat);
}

void LCD_WR_REG(uint8_t da)
{
    gpio_set_level(PIN_NUM_DC, 0);
    LCD_Writ_Bus(da);
    gpio_set_level(PIN_NUM_DC, 1);
}

// 设置显示区域
void LCD_set_region(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    if (USE_HORIZONTAL == 0 || USE_HORIZONTAL == 1)
    {
#ifdef USE_ST7735S
        x1 = x1 + 24;
        x2 = x2 + 24;    
#else
        x1 = x1 + 26;
        x2 = x2 + 26;
        y1 = y1 + 1;
        y2 = y2 + 1;
#endif
    }

    if (USE_HORIZONTAL >= 2) // 不明白这里为什么会需要偏移
    { 
        x1 = x1 + 1;
        x2 = x2 + 1;
        y1 = y1 + 26;
        y2 = y2 + 26;
    }
    LCD_WR_REG(0x2a); // 列地址设置
    LCD_WR_DATA(x1);
    LCD_WR_DATA(x2);
    LCD_WR_REG(0x2b); // 行地址设置
    LCD_WR_DATA(y1);
    LCD_WR_DATA(y2);
    LCD_WR_REG(0x2c); // 储存器写
}

void pwm_init()
{
    mcpwm_pin_config_t pin_config = {
        .mcpwm0a_out_num = -1,
        .mcpwm0b_out_num = PIN_NUM_BCKL};
    mcpwm_set_pin(MCPWM_UNIT_0, &pin_config);
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 10000; // frequency = 1000Hz
    pwm_config.cmpr_a = 0.0;	  // duty cycle of PWMxA = 60.0%
    pwm_config.cmpr_b = 0.0;	  // duty cycle of PWMxb = 50.0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config); // Configure PWM0A & PWM0B with above settings

    // mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, 0, 100);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, 1, 0.0);
}

void LCD_Init(void)
{

    spi_init();
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    // gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_DC, 1);
    gpio_set_level(PIN_NUM_RST, 1);
    //gpio_set_level(PIN_NUM_BCKL, 0);
    pwm_init();

    // gpio_set_direction(SPI_NUM_CS, GPIO_MODE_OUTPUT);
    // Reset the display
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(10 / portTICK_RATE_MS);

    LCD_WR_REG(0x11);   // Sleep out
    vTaskDelay(10 / portTICK_RATE_MS); // Delay 120ms

    LCD_WR_REG(0xB1); // Normal mode
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB2); // Idle mode
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB3); // Partial mode
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB4); // Dot inversion
    LCD_WR_DATA8(0x03);
    LCD_WR_REG(0xC0); // AVDD GVDD
    LCD_WR_DATA8(0xAB);
    LCD_WR_DATA8(0x0B);
    LCD_WR_DATA8(0x04);
    LCD_WR_REG(0xC1);	// VGH VGL
    LCD_WR_DATA8(0xC5); // C0
    LCD_WR_REG(0xC2);	// Normal Mode
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x00);
    LCD_WR_REG(0xC3); // Idle
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0x6A);
    LCD_WR_REG(0xC4); // Partial+Full
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0xEE);
    LCD_WR_REG(0xC5); // VCOM
    LCD_WR_DATA8(0x0F);
    LCD_WR_REG(0xE0); // positive gamma
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x0E);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x10);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x02);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x0F);
    LCD_WR_DATA8(0x25);
    LCD_WR_DATA8(0x36);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x10);
    LCD_WR_REG(0xE1); // negative gamma
    LCD_WR_DATA8(0x0A);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x0F);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x02);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x0F);
    LCD_WR_DATA8(0x25);
    LCD_WR_DATA8(0x35);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x10);

    LCD_WR_REG(0xFC);
    LCD_WR_DATA8(0x80);

    LCD_WR_REG(0x3A);
    LCD_WR_DATA8(0x05);
    LCD_WR_REG(0x36);
    if (USE_HORIZONTAL == 0)
        LCD_WR_DATA8(0x08);
    else if (USE_HORIZONTAL == 1)
        LCD_WR_DATA8(0xC8);
    else if (USE_HORIZONTAL == 2)
        LCD_WR_DATA8(0x78);
    else
        LCD_WR_DATA8(0xA8);
    if (st7735_lcd_inversion) {
        LCD_WR_REG(0x21); // Display inversion
    }
    LCD_WR_REG(0x29); // Display on
    LCD_WR_REG(0x2A); // Set Column Address
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x1A);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x69);
    LCD_WR_REG(0x2B); // Set Page Address
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x01);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0xA0);
    LCD_WR_REG(0x2C);
    
    //LCD_SET_BLK(10);
    //printf("set led 0\n");
    //LCD_Fill(0, 0, 160, 40, RED);
    //vTaskDelay(2000 / portTICK_RATE_MS);
    //LCD_Clear();

    vTaskDelay(10 / portTICK_RATE_MS);
    //gpio_set_level(PIN_NUM_BCKL, 0);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, 1, st7735_lcd_backlight);
}

void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
    uint16_t i, j;
    LCD_set_region(xsta, ysta, xend - 1, yend - 1); // 设置显示范围
    for (i = ysta; i < yend; i++)
    {
        for (j = xsta; j < xend; j++)
        {
            LCD_WR_DATA(color);
        }
    }
}

extern lv_color_t lv_disp_buf1[];
void LCD_Clear()
{

    LCD_set_region(0, 0, 159, 79); // 设置显示范围

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));	 // Zero out the transaction
    t.length = 2 * 8 * 160 * 80; // Command is 8 bits
    t.tx_buffer = lv_disp_buf1;	 // The data is the cmd itself
    // t.user=(void*)1;                //D/C needs to be set to 0
    ret = spi_device_polling_transmit(my_spi, &t); // Transmit!
    assert(ret == ESP_OK);						   // Should have had no issues.
}

void LCD_SET_BLK(int8_t light)
{
    if (light <= ST7735_LCD_BACKLIGHT_MAX && light >= ST7735_LCD_BACKLIGHT_MIN)
    {
        st7735_lcd_backlight = light;
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, 1, st7735_lcd_backlight);
    }
}

uint16_t LCD_GET_BLK(void)
{
    return st7735_lcd_backlight;
}
// 反色 -只设置状态，预览，不保存
void LCD_set_invert(bool invert)
{
    if (invert) {
        LCD_WR_REG(0x21); // Display inversion
        return;
    }
    LCD_WR_REG(0x20); // Display inversion off
}
void LCD_set_invert_state(bool invert)
{
    LCD_set_invert(invert);
    st7735_lcd_inversion = invert;
}
uint16_t LCD_is_invert()
{
    return st7735_lcd_inversion;
}