﻿#ifndef __page_menu_table_H
#define __page_menu_table_H



#ifdef __cplusplus
extern "C" {
#endif

    /*********************
    *      INCLUDES
    *********************/
#include "lvgl.h"
    /*********************
    *      DEFINES
    *********************/

    /**********************
    *      TYPEDEFS
    **********************/

    /**********************
    * GLOBAL PROTOTYPES
    **********************/
    extern lv_indev_t* indev_keypad;
    void page_scan_table_create(void);
    /**********************
    *      MACROS
    **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif




#endif
