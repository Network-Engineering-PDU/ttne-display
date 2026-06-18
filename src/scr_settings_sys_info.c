#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"

#include "scr_settings_menu.h"
#include "scr_settings_vis.h"
#include "scr_settings_sys.h"
#include "scr_settings_upd.h"
#include "scr_settings_nw_menu.h"
#include "scr_current.h"
#include "scr_login.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "config.h"
#include "screen.h"
#include "ttne_display.h"

void scr_settings_sys_info_create(lv_obj_t* menu, lv_obj_t* btn) {
    /* Create the settings page directly */
    lv_obj_t* settings_sys_info_page = tt_obj_menu_page_create(menu, btn, NULL, "System Info");
    lv_obj_set_flex_flow(settings_sys_info_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(settings_sys_info_page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* model_label = lv_label_create(settings_sys_info_page);
    lv_label_set_text(model_label, "Model: PowerIT Easy");
    lv_obj_set_style_text_font(model_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(model_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(model_label, lv_pct(100));
    lv_obj_set_style_text_color(model_label, lv_color_white(), 0);
    lv_obj_set_style_pad_left(model_label, 10, 0);
    lv_obj_set_style_pad_top(model_label, 10, 0);
    
}
