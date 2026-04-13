#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_settings_menu.h"
#include "scr_settings.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"

/* Global variables ***********************************************************/

static lv_obj_t* menu;
static lv_obj_t* settings_page;


/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void btn_visualization_cb(lv_event_t* e);
static void btn_networks_cb(lv_event_t* e);
static void btn_system_setup_cb(lv_event_t* e);
static void btn_pdu_update_cb(lv_event_t* e);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			LV_LOG_USER("Settings cb");
		}
	}
}


static void btn_visualization_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		// Navigate to Visualization settings
		LV_LOG_USER("Visualization settings");
		lv_menu_set_page(menu, settings_page);
	}
}

static void btn_networks_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		// Navigate to Networks settings
		LV_LOG_USER("Networks settings");
	}
}

static void btn_system_setup_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		// Navigate to System setup settings
		LV_LOG_USER("System setup settings");
	}
}

static void btn_pdu_update_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		// Navigate to PDU update settings
		LV_LOG_USER("PDU update settings");
	}
}

/* Public functions ***********************************************************/

void scr_settings_menu_create(lv_obj_t* l_menu, lv_obj_t* btn)
{
	menu = l_menu;

    lv_obj_t* info_cont = tt_obj_menu_page_create(menu, btn, menu_cb, "Info");
	lv_obj_t* info_product_cont = tt_obj_cont_create(info_cont);

	lv_obj_t* settings_menu_page = lv_menu_page_create(menu, NULL);
	lv_obj_t* settings_cont = lv_menu_cont_create(settings_menu_page);
	lv_obj_set_flex_flow(settings_cont, LV_FLEX_FLOW_ROW_WRAP);

	lv_menu_set_page_title_static(settings_menu_page, "Settings");

	lv_obj_t* btn_viz = tt_obj_btn_mtx_create(settings_cont, btn_visualization_cb, "Visual", NULL);
	lv_obj_t* btn_nw = tt_obj_btn_mtx_create(settings_cont, btn_networks_cb, "Networks", NULL);
	lv_obj_t* btn_sys = tt_obj_btn_mtx_create(settings_cont, btn_system_setup_cb, "Sys setup", NULL);

	lv_obj_t* btn_pdu = tt_obj_btn_mtx_create(settings_cont, btn_pdu_update_cb, "Sys update", NULL);

	// Create the detailed settings page
	//settings_page = scr_settings_create(menu, btn);
}
