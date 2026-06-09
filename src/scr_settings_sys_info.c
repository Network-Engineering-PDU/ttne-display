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
#include "models.h"
#include "controller.h"
#include "config.h"
#include "screen.h"
#include "ttne_display.h"
#include "runbg.h"

static lv_obj_t* l_menu;
char str[100];

/* Function definitions *******************************************************/

void scr_settings_sys_info_create(lv_obj_t* menu, lv_obj_t* btn) {

    l_menu = menu;
    /* Create the settings page directly */
    lv_obj_t* settings_sys_info_page = tt_obj_menu_page_create(l_menu, btn, NULL, "Settings");

    /* Apply Flex layout directly to the page. 
       In LVGL, menu pages usually have a 'scrollable' part or are objects themselves 
       that can act as containers.
    */
    lv_obj_set_flex_flow(settings_sys_info_page, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(
        settings_sys_info_page,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    sprintf(str, "Model: %s", "PowerIT Easy");
    
    /* Create and display the model label */
    lv_obj_t* model_label = lv_label_create(settings_sys_info_page);
    lv_label_set_text(model_label, str);
    
}
