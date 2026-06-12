#include "lvgl/lvgl.h"

#include "scr_settings_menu.h"
#include "scr_settings_vis.h"
#include "scr_settings_nw_menu.h"
#include "scr_settings_sys.h"
#include "scr_settings_upd.h"
#include "tt_obj.h"
#include "utils.h"

void scr_settings_menu_create(lv_obj_t* l_menu, lv_obj_t* btn)
{
	lv_obj_t* settings_cont = tt_obj_menu_page_create(l_menu, btn, NULL,
			"Settings");

	lv_obj_set_flex_flow(settings_cont, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_align(settings_cont, LV_FLEX_ALIGN_CENTER,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* btn_vis = tt_obj_btn_mtx_create(settings_cont, NULL,
			"Visual", ASSET("visualization.png"));
	lv_obj_t* btn_nw = tt_obj_btn_mtx_create(settings_cont, NULL,
			"Networks", ASSET("network.png"));
	lv_obj_t* btn_sys = tt_obj_btn_mtx_create(settings_cont, NULL,
			"Sys Setup", ASSET("sys_setup.png"));
	lv_obj_t* btn_update = tt_obj_btn_mtx_create(settings_cont, NULL,
			"Sys Update", ASSET("sys_update.png"));

    /* Navigation links remain the same */
    scr_settings_vis_create(l_menu, btn_vis);
    scr_settings_nw_menu_create(l_menu, btn_nw);
    scr_settings_sys_create(l_menu, btn_sys);
    scr_settings_update_create(l_menu, btn_update);
}
