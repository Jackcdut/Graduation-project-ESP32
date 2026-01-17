/*
* Copyright 2023 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lv_textprogress.h"
#include <stdio.h>

static void lv_textprogress_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_textprogress_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

const lv_obj_class_t lv_textprogress_class = {
    .constructor_cb = lv_textprogress_constructor,
    .destructor_cb = lv_textprogress_destructor,
    .width_def = LV_DPI_DEF * 2,
    .height_def = LV_DPI_DEF / 10,
    .instance_size = sizeof(lv_textprogress_t),
    .base_class = &lv_obj_class
};

static void lv_textprogress_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_textprogress_t * textprogress = (lv_textprogress_t *)obj;

    /* Create bar */
    textprogress->bar = lv_bar_create(obj);
    lv_obj_set_size(textprogress->bar, LV_PCT(100), LV_PCT(100));
    lv_obj_center(textprogress->bar);
    lv_bar_set_value(textprogress->bar, 0, LV_ANIM_OFF);

    /* Create label */
    textprogress->label = lv_label_create(obj);
    lv_obj_center(textprogress->label);
    lv_label_set_text(textprogress->label, "0%");
    lv_obj_set_style_text_color(textprogress->label, lv_color_white(), 0);

    /* Initialize values */
    textprogress->min_value = 0;
    textprogress->max_value = 100;
    textprogress->cur_value = 0;
    textprogress->decimal = 0;
}

static void lv_textprogress_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_UNUSED(obj);
}

lv_obj_t * lv_textprogress_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_textprogress_class, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_textprogress_set_value(lv_obj_t * obj, int32_t value)
{
    lv_textprogress_t * textprogress = (lv_textprogress_t *)obj;
    if(textprogress == NULL) return;

    textprogress->cur_value = value;

    /* Clamp value to range */
    if(value < textprogress->min_value) value = textprogress->min_value;
    if(value > textprogress->max_value) value = textprogress->max_value;

    /* Calculate percentage */
    int32_t range = textprogress->max_value - textprogress->min_value;
    int32_t percent = 0;
    if(range != 0) {
        percent = ((value - textprogress->min_value) * 100) / range;
    }

    /* Update bar */
    lv_bar_set_value(textprogress->bar, percent, LV_ANIM_OFF);

    /* Update label */
    char buf[16];
    if(textprogress->decimal == 0) {
        lv_snprintf(buf, sizeof(buf), "%d%%", (int)percent);
    } else {
        lv_snprintf(buf, sizeof(buf), "%d.%d%%", (int)percent, 0);
    }
    lv_label_set_text(textprogress->label, buf);
}

void lv_textprogress_set_range_value(lv_obj_t * obj, int32_t min, int32_t max, int32_t cur, uint32_t anim_time)
{
    lv_textprogress_t * textprogress = (lv_textprogress_t *)obj;
    if(textprogress == NULL) return;

    textprogress->min_value = min;
    textprogress->max_value = max;

    lv_bar_set_range(textprogress->bar, 0, 100);
    
    if(anim_time == 0) {
        lv_textprogress_set_value(obj, cur);
    } else {
        /* If animation is needed, set it up */
        lv_textprogress_set_value(obj, cur);
    }
}

void lv_textprogress_set_decimal(lv_obj_t * obj, uint8_t decimal)
{
    lv_textprogress_t * textprogress = (lv_textprogress_t *)obj;
    if(textprogress == NULL) return;

    textprogress->decimal = decimal;
}

int32_t lv_textprogress_get_value(lv_obj_t * obj)
{
    lv_textprogress_t * textprogress = (lv_textprogress_t *)obj;
    if(textprogress == NULL) return 0;

    return textprogress->cur_value;
}

