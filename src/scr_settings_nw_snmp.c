#include <stdio.h>

#include "lvgl/lvgl.h"
#include "scr_settings_nw_snmp.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "app/app_state.h"
#include "backend/backend.h"
#include "config.h"
#include "screen.h"
#include "ttne_display.h"

/* Global variables ***********************************************************/
static lv_obj_t* btn_snmp;
static bool running = false;
static bool refresh_pending;

/* Function prototypes ********************************************************/
static void menu_cb(lv_event_t* e);
static void btn_snmp_cb(lv_event_t* e);
static void msg_box_snmp_cb(lv_event_t* e);
static void services_refresh_cb(int err, void* userdata);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(menu);
		if (curr_page == page) {
			if (!refresh_pending &&
					backend_network_services_refresh(services_refresh_cb, NULL) == 0) {
				refresh_pending = true;
			}
			if (!running) {
				running = true;
				LV_LOG_USER("Settings open");
			}
		}
	} else if (code == LV_EVENT_CLICKED) {
		if (running) {
			running = false;
			LV_LOG_USER("Settings close");
		}
	}
}

static void btn_snmp_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* btn = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		char msg[100];
		snprintf(msg, sizeof(msg), "Are you sure you want to " TT_COLOR_GREEN_NE_STR
				" %s# SNMP?", (lv_obj_get_state(btn) & LV_STATE_CHECKED) ?
				"enable" : "disable");
		tt_obj_msg_box_create("SNMP service", msg, NULL, msg_box_snmp_cb);
	}
}

static void msg_box_snmp_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			bool enable = (lv_obj_get_state(btn_snmp) & LV_STATE_CHECKED) ? true : false;
			backend_network_service_set("snmp", enable, services_refresh_cb, NULL);
		} else {
			app_state_snapshot_t snapshot;
			app_state_get_snapshot(&snapshot);
			tt_obj_btn_toggle_set_state(btn_snmp, snapshot.nw_services.snmp);
		}
		lv_msgbox_close(obj);
	}
}

static void services_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	refresh_pending = false;
	if (err != 0) {
		return;
	}
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	if (snapshot.nw_services.valid) {
		tt_obj_btn_toggle_set_state(btn_snmp, snapshot.nw_services.snmp);
	}
}

void scr_settings_nw_snmp_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* settings_cont = tt_obj_menu_page_create(menu, btn, menu_cb,
			"SNMP");

    lv_obj_t* settings_nw_cont = tt_obj_cont_create(settings_cont);
    btn_snmp = tt_obj_btn_toggle_create(settings_nw_cont, btn_snmp_cb, "SNMP");
}
