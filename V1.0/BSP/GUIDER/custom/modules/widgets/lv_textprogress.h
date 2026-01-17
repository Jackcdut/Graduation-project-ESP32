/*
* Copyright 2023 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef LV_TEXTPROGRESS_H
#define LV_TEXTPROGRESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct {
    lv_obj_t obj;
    lv_obj_t * bar;
    lv_obj_t * label;
    int32_t min_value;
    int32_t max_value;
    int32_t cur_value;
    uint8_t decimal;
} lv_textprogress_t;

extern const lv_obj_class_t lv_textprogress_class;

lv_obj_t * lv_textprogress_create(lv_obj_t * parent);

void lv_textprogress_set_value(lv_obj_t * obj, int32_t value);

void lv_textprogress_set_range_value(lv_obj_t * obj, int32_t min, int32_t max, int32_t cur, uint32_t anim_time);

void lv_textprogress_set_decimal(lv_obj_t * obj, uint8_t decimal);

int32_t lv_textprogress_get_value(lv_obj_t * obj);

#ifdef __cplusplus
}
#endif

#endif /* LV_TEXTPROGRESS_H */

