#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"

#include "scr_settings_menu.h"
#include "scr_settings_vis.h"
#include "scr_settings_sys.h"
#include "scr_settings_upd.h"
#include "scr_settings_nw_menu.h"
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

#define TIMER_ROT 5000 // ms
#define TIMER_CHECK_UPDATE 1000 // ms

#ifdef SIMULATOR_ENABLED
#define MOUNT_DIR "/home/guille"
#else
#define MOUNT_DIR "/run/mount"
#endif

/* Global variables ***********************************************************/
static lv_obj_t* menu;

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			LV_LOG_USER("Settings menu cb");
		}
	}
}

/* Function definitions *******************************************************/

void scr_settings_menu_create(lv_obj_t* l_menu, lv_obj_t* btn) 
{
    menu = l_menu;

    /* Create the settings page directly */
    lv_obj_t* settings_page = tt_obj_menu_page_create(menu, btn, NULL, "Settings");

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
    lv_obj_t* btn_vis = tt_obj_btn_mtx_create(settings_page, NULL, "Visual", ASSET("visual.png"));

    lv_obj_t* btn_nw = tt_obj_btn_mtx_create(settings_page, NULL, "Networks", ASSET("network.png"));

    lv_obj_t* btn_sys = tt_obj_btn_mtx_create(settings_page, NULL, "Sys Setup", ASSET("menu.png"));

    lv_obj_t* btn_update = tt_obj_btn_mtx_create(settings_page, NULL, "Sys Update", ASSET("menu.png"));

    /* Navigation links remain the same */
    scr_current_create(menu, btn_vis);
    //scr_settings_vis_create(menu, btn_vis);
    scr_settings_nw_menu_create(menu, btn_nw);
    
    scr_login_create(menu, btn_sys);
    //scr_settings_sys_create(menu, btn_sys);
    scr_settings_update_create(menu, btn_update);
}
