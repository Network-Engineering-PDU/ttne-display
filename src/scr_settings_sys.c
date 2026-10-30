#include <stdio.h>
#include <stdlib.h>

#include "i18n.h"
#include "lvgl/lvgl.h"

#include "scr_settings_menu.h"
#include "scr_settings_vis.h"
#include "scr_settings_sys.h"
#include "scr_settings_sys_info.h"
#include "scr_settings_sys_datetime.h"
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
#include "scr_support.h"
#include "scr_settings_sys_users.h"

static lv_obj_t* l_menu;

/* Function definitions *******************************************************/

void scr_settings_sys_create(lv_obj_t* menu, lv_obj_t* btn) {
    l_menu = menu;

    /* Create the settings page directly */
    lv_obj_t* settings_sys_page = tt_obj_menu_page_create(l_menu, btn, NULL, tr("System Setup"));

    /* Apply Flex layout directly to the page. 
       In LVGL, menu pages usually have a 'scrollable' part or are objects themselves 
       that can act as containers.
    */
    lv_obj_set_flex_flow(settings_sys_page, LV_FLEX_FLOW_ROW_WRAP);

    /* HUB buttons: Parent is now 'settings_page' instead of 'cont' */
    lv_obj_t* btn_current = tt_obj_btn_mtx_create(settings_sys_page, NULL, tr("Rated Current"), ASSET("r_current.png"));

    lv_obj_t* btn_info = tt_obj_btn_mtx_create(settings_sys_page, NULL, tr("Info"), ASSET("info.png"));

    lv_obj_t* btn_support = tt_obj_btn_mtx_create(settings_sys_page, NULL, tr("Support"), ASSET("support.png"));

    lv_obj_t* btn_datetime = tt_obj_btn_mtx_create(settings_sys_page, NULL, tr("Date & Time"), ASSET("date_time.png"));

    lv_obj_t* btn_users = tt_obj_btn_mtx_create(settings_sys_page, NULL, tr("Users"), ASSET("sys_setup.png"));

    /* Navigation links remain the same */
    scr_current_create(l_menu, btn_current);
    scr_settings_sys_info_create(l_menu, btn_info);
    scr_support_create(l_menu, btn_support);
    scr_settings_sys_datetime_create(l_menu, btn_datetime);
    //scr_settings_sys_users_create(l_menu, btn_users);

}
