#include "lvgl/lvgl.h"
#include "scr_settings_nw_blue.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"
#include "config.h"
#include "screen.h"
#include "ttne_display.h"
#include "runbg.h"

static lv_obj_t* bluetooth_enable_cbx;

void scr_settings_nw_blue_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* blue_page = tt_obj_menu_page_create(menu, btn, NULL, "Bluetooth");
    lv_obj_t* cont = tt_obj_cont_create(blue_page);

    /* Bluetooth Enable */
    tt_obj_label_create(cont, "Bluetooth enable");
    bluetooth_enable_cbx = tt_obj_checkbox_create(cont, "", NULL);

    /* OK / Cancel buttons */
    lv_obj_t* btn_row = lv_obj_create(cont);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    tt_obj_btn_std_create(btn_row, NULL, "OK");
    tt_obj_btn_std_create(btn_row, NULL, "Cancel");
}
