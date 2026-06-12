#include "lvgl/lvgl.h"

#include "scr_settings_menu.h"
#include "scr_settings_nw.h"
#include "scr_settings_upd.h"
#include "tt_obj.h"
#include "utils.h"

static void create_placeholder_page(lv_obj_t* menu, lv_obj_t* btn,
		char* title, char* message);

static void create_placeholder_page(lv_obj_t* menu, lv_obj_t* btn,
		char* title, char* message)
{
	lv_obj_t* cont = tt_obj_menu_page_create(menu, btn, NULL, title);
	tt_obj_label_color_create(cont, message);
}

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

	create_placeholder_page(l_menu, btn_vis, "Settings / Visual",
			"Visual settings");
	scr_settings_nw_create(l_menu, btn_nw);
	create_placeholder_page(l_menu, btn_sys, "Settings / Sys Setup",
			"System setup");
	scr_settings_update_create(l_menu, btn_update);
}
