/**
 * @file lv_port_disp_templ.c
 *
 */

 /*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include "../../lvgl.h"
#include "lcd.h"
#include "capi_video.h"
#include "driver/gpio.h"
#include "hardware/rx5808.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
/*********************
 *      DEFINES
 *********************/
#define DISP_BUF_SIZE        (MY_DISP_HOR_RES * MY_DISP_VER_RES)
#define DAC_VIDEO_SWITCH     19
#define DAC_VIDEO_PIN     25

lv_color_t lv_disp_buf1[DISP_BUF_SIZE];
lv_color_t lv_disp_buf2[DISP_BUF_SIZE];
//static lv_color_t lv_disp_buf3[240*140];
lv_disp_drv_t *disp_drv_spi;
lv_disp_t *default_disp = NULL;
bool g_dac_video_render = false;
bool g_dac_video_sync = false;
uint8_t refresh_times = 0;
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

//static SemaphoreHandle_t osd_ready_semap;


void esp32_video(void *param);
/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static void IRAM_ATTR composite_rounder_cb(lv_disp_drv_t * disp_drv, lv_area_t * area);
void IRAM_ATTR composite_monitor_cb(lv_disp_drv_t *disp_drv, uint32_t time_ms, uint32_t px_num);
bool get_video_switch(void) {
    return g_dac_video_render;
}
void IRAM_ATTR video_composite_switch(bool flag) {
    g_dac_video_render = flag;
    if(g_dac_video_render) {
        // 暂停rx5808
        RX5808_Pause();
        // 注册A/V信号输出
        esp32_video_start((bool)RX5808_Get_OSD_Format());
        refresh_times = 1;
	    gpio_set_level(DAC_VIDEO_SWITCH, 0);
        return;
    }
	gpio_set_level(DAC_VIDEO_SWITCH, 1);
    esp32_video_stop();
	gpio_reset_pin(DAC_VIDEO_PIN);
    gpio_set_direction(DAC_VIDEO_PIN, GPIO_MODE_INPUT);/*  */
    RX5808_Resume();
}

void video_composite_sync_switch(bool flag) {
    g_dac_video_sync = flag;
}
void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
	gpio_reset_pin(DAC_VIDEO_SWITCH);
    gpio_set_direction(DAC_VIDEO_SWITCH, GPIO_MODE_OUTPUT);/*  */
	gpio_set_level(DAC_VIDEO_SWITCH, 1);
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    static lv_disp_draw_buf_t draw_buf_dsc_2;
    lv_disp_draw_buf_init(&draw_buf_dsc_2, lv_disp_buf1, lv_disp_buf2, DISP_BUF_SIZE);   /*Initialize the display buffer*/
    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;
    disp_drv.rounder_cb = composite_rounder_cb;
    disp_drv.monitor_cb = composite_monitor_cb;
    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_2;

    /*Required for Example 3)*/
    //disp_drv.full_refresh = 1

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    disp_drv_spi = &disp_drv;
    default_disp = lv_disp_drv_register(disp_drv_spi);
    //video_composite_switch(true);

    //  osd_ready_semap=xSemaphoreCreateBinary();
	// 	if( osd_ready_semap == NULL ) { 
    //         assert(false);
    //         return;
    //     }
	// 	xTaskCreatePinnedToCore( (TaskFunction_t)esp32_video,
	//                           "esp32_video", 
	// 						  1024, 
	// 						  NULL,
	// 						   3,
	// 						    NULL, 
	// 							1);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
}
#include "SPI.h"
/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
uint16_t videoframe_cnt = 0;
static void IRAM_ATTR disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/
    //DAC flush
    if(g_dac_video_render) {
        lv_color_t *color_p_dac = color_p;
        for(int y = area->y1; y <= area->y2; ++y) {
            for(int x = area->x1; x <= area->x2; ++x) {
                esp32_video_set_color(x+50, y+80, 
                    lv_color_to8(*color_p_dac));
                ++color_p_dac;
            }
        }
    }
    //LCD flush
    esp_err_t ret;
    spi_transaction_t t;
	uint16_t height,width;
	width=area->x2-area->x1+1; 			//??????????????
	height=area->y2-area->y1+1;			//????
    LCD_set_region(area->x1,area->y1,area->x2,area->y2);    
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=width*height*16;       //Command is 8 bits
    //t.user=(void*)1;                //D/C needs to be set to 0
	t.tx_buffer=color_p;
    gpio_set_level(PIN_NUM_DC, 1);
 
    ret=spi_device_polling_transmit(my_spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

// void esp32_video(void *param)
// {
//   while(1)
//   {
//      xSemaphoreTake(osd_ready_semap,portMAX_DELAY);
//      esp32_video_sync();
//   }
// }
void IRAM_ATTR composite_monitor_cb(lv_disp_drv_t * disp_drv, uint32_t time_ms, uint32_t px_num) {
    // 每次切换OSD显示时, 都需要强制绘制屏幕
    if(refresh_times) {
        lv_obj_invalidate(lv_scr_act());
        --refresh_times;
    }
    if(g_dac_video_render && g_dac_video_sync) {
            esp32_video_sync();
            //xSemaphoreGive(osd_ready_semap); 
    }
}
void IRAM_ATTR composite_rounder_cb(lv_disp_drv_t * disp_drv, lv_area_t * area) {
    // // RBG16bit的情况下，保证每次刷新的范围都32bit对齐
    // area->x1 &= ~1;
    // area->x2 |= 1;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
