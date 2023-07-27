#include "lcd.h"
#include "beep.h"
#include "SPI.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "freertos/semphr.h"
#include <string.h>
#include "hwvers.h"

volatile uint8_t beep_en = 1; // off
bool is_inited = false;
static SemaphoreHandle_t beep_semap;

void beep_task(void *param)
{
	while (1)
	{
		xSemaphoreTake(beep_semap, portMAX_DELAY);
		beep_on_off(1);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		beep_on_off(0);
	}
}

void PWM_Enable()
{
	if (!is_inited)
	{
		is_inited = true;
		/*
		 * 设置LEDC的定时器的配置
		 */
		ledc_timer_config_t ledc_timer_ls = {
			.duty_resolution = LEDC_TIMER_13_BIT, // 设置分辨率,最大为2^13-1
			.freq_hz = 4000,					  // PWM信号频率
			.speed_mode = LEDC_LOW_SPEED_MODE,	  // 定时器模式（“高速”或“低速”）
			.timer_num = LEDC_TIMER_0,			  // 设置定时器源（0-3）
			.clk_cfg = LEDC_AUTO_CLK,			  // 配置LEDC时钟源（这里是自动选择）
		};
		// 初始化ledc的定时器配置
		ledc_timer_config(&ledc_timer_ls);
		/*
		 * 通过选择为 LEDC 控制器的每个通道准备单独的配置：
		 * - 控制器的通道号
		 * - 输出占空比，初始设置为 0
		 * - LEDC 连接到的 GPIO 编号
		 * - 速度模式，高或低
		 * - 为LEDC通道指定定时器
		 *   注意: 如果不同通道使用一个定时器，那么这些通道的频率和占空比分辨率将相同
		 */
		ledc_channel_config_t ledc_channel = {
			.channel = LEDC_CHANNEL_1,
			.duty = 4096,
			.gpio_num = Beep_Pin_Num,
			.speed_mode = LEDC_LOW_SPEED_MODE,
			.hpoint = 0,
			.timer_sel = LEDC_TIMER_0};
		// 初始化ledc的通道
		ledc_channel_config(&ledc_channel);
	}
	else
	{
		// 设置占空比
		ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, 4096);
		ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
		ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
	}
}
void PWM_Disable()
{
	ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
	ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
}

void Beep_Init()
{

#if Beep_Is_Src == 1
	gpio_set_direction(Beep_Pin_Num, GPIO_MODE_OUTPUT);
#endif
	// 	if(beep_en==1) {
	// #if Beep_Is_Src == 1
	// 		gpio_set_level(Beep_Pin_Num, 1);
	// #else
	// 		PWM_Enable();
	// #endif
	// 		//vTaskDelay(200 / portTICK_PERIOD_MS);
	// #if Beep_Is_Src == 1
	// 		gpio_set_level(Beep_Pin_Num, 0);
	// #else
	// 		PWM_Disable();
	// #endif
	//  	}
	beep_semap = xSemaphoreCreateCounting(3, 0);
	if (beep_semap == NULL)
	{
		assert(false);
		return;
	}
	xTaskCreatePinnedToCore((TaskFunction_t)beep_task,
							"beep_task",
							768,
							NULL,
							1,
							NULL,
							1);
	beep_turn_on();
}

void beep_set_enable_disable(uint8_t en)
{
	if (en)
	{
		beep_en = 1;
	}
	else
	{
		beep_en = 0;
	}
}

uint16_t beep_get_status()
{
	return beep_en;
}

void beep_on_off(uint8_t on_off)
{
	// if(beep_en==0)
	//	return ;
	if (on_off)
	{
#if Beep_Is_Src == 1
		gpio_set_level(Beep_Pin_Num, 1);
#else
		PWM_Enable();
#endif
	}
	else
	{
#if Beep_Is_Src == 1
		gpio_set_level(Beep_Pin_Num, 0);
#else
		PWM_Disable();
#endif
	}
}

void beep_turn_on(void)
{
	if (beep_en == 1)
	{
		xSemaphoreGive(beep_semap);
	}
}

void beep_set_tone(uint16_t tone)
{
	// 	#if Passive_Buzzer == 1
	// 	uint32_t psc=480000/tone;
	//   TIM4->PSC=(uint16_t)psc-1;
	// 	#endif
}
