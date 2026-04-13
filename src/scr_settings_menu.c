#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"

#include "scr_settings_menu.h"
#include "scr_settings_vis.h"
#include "scr_settings_nw.h"
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

	lv_obj_t* settings_cont = tt_obj_menu_page_create(menu, btn, menu_cb, "Settings");
	lv_obj_t* settings_page = scr_sensors_data_create(menu);

    lv_obj_t* cont = tt_obj_cont_create(settings_page);

    /* Layout: center grid like Page 2 */
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(
        cont,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    /* HUB buttons */
    lv_obj_t* btn_vis = tt_obj_btn_mtx_create(cont, NULL, "Visual", ASSET("menu.png"));

    lv_obj_t* btn_nw = tt_obj_btn_mtx_create(cont, NULL, "Networks", ASSET("menu.png"));

    lv_obj_t* btn_sys = tt_obj_btn_mtx_create(cont, NULL, "Sys setup", ASSET("menu.png"));

    lv_obj_t* btn_update = tt_obj_btn_mtx_create(cont, NULL, "Sys update", ASSET("menu.png"));

    /* Navigation (will be implemented step by step) */
    scr_settings_vis_create(menu, btn_vis);
    scr_settings_nw_menu_create(menu, btn_nw);
    scr_settings_sys_create(menu, btn_sys);
    scr_settings_update_create(menu, btn_update);
}