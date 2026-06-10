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

/* Function definitions *******************************************************/

void scr_settings_menu_create(lv_obj_t* l_menu, lv_obj_t* btn) 
{

    /* Create the settings page directly */
    lv_obj_t* settings_page = tt_obj_menu_page_create(l_menu, btn, NULL, "Settings");

    /* Apply Flex layout directly to the page. 
       In LVGL, menu pages usually have a 'scrollable' part or are objects themselves 
       that can act as containers.
    */
    lv_obj_set_flex_flow(settings_page, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(
        settings_page,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    /* HUB buttons: Parent is now 'settings_page' instead of 'cont' */
    lv_obj_t* btn_vis = tt_obj_btn_mtx_create(settings_page, NULL, "Visual", ASSET("eye.png"));

    lv_obj_t* btn_nw = tt_obj_btn_mtx_create(settings_page, NULL, "Networks", ASSET("network.png"));

    lv_obj_t* btn_sys = tt_obj_btn_mtx_create(settings_page, NULL, "Sys Setup", ASSET("menu.png"));

    lv_obj_t* btn_update = tt_obj_btn_mtx_create(settings_page, NULL, "Sys Update", ASSET("menu.png"));

    //lv_obj_t* btn_login = tt_obj_btn_mtx_create(settings_page, NULL, "Login", ASSET("menu.png"));

    /* Navigation links remain the same */
    scr_settings_vis_create(l_menu, btn_vis);
    scr_settings_nw_menu_create(l_menu, btn_nw);
    scr_settings_sys_create(l_menu, btn_sys);
    scr_settings_update_create(l_menu, btn_update);
    //scr_login_create(l_menu, btn_login);
}
