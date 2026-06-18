#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "scr_settings_nw_modbus.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "app/app_state.h"
#include "backend/backend.h"
#include "config.h"
#include "screen.h"
#include "ttne_display.h"
#include "scr_keyboard.h"

/* Global variables ***********************************************************/
static lv_obj_t* txt_modbus_addr;
static lv_obj_t* btn_modbus;
static bool running = false;
static bool services_refresh_pending;
static bool modbus_refresh_pending;

/* Function prototypes ********************************************************/
static void btn_modbus_cb(lv_event_t* e);
static void msg_box_modbus_cb(lv_event_t* e);
static void txt_modbus_addr_cb(lv_event_t* e);
static void services_refresh_cb(int err, void* userdata);
static void modbus_refresh_cb(int err, void* userdata);

/* Callbacks ******************************************************************/
static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(menu);
		if (curr_page == page) {
			if (!services_refresh_pending &&
					backend_network_services_refresh(services_refresh_cb, NULL) == 0) {
				services_refresh_pending = true;
			}
			if (!modbus_refresh_pending &&
					backend_modbus_refresh(modbus_refresh_cb, NULL) == 0) {
				modbus_refresh_pending = true;
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

static void btn_modbus_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* btn = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		char msg[100];
		snprintf(msg, sizeof(msg), "Are you sure you want to " TT_COLOR_GREEN_NE_STR
				" %s# Modbus?", (lv_obj_get_state(btn) & LV_STATE_CHECKED) ?
				"enable" : "disable");
		tt_obj_msg_box_create("Modbus service", msg, NULL, msg_box_modbus_cb);
	}
}

static void msg_box_modbus_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			bool enable = (lv_obj_get_state(btn_modbus) & LV_STATE_CHECKED) != 0;
			backend_network_service_set("modbus", enable,
					services_refresh_cb, NULL);
		} else {
			app_state_snapshot_t snapshot;
			app_state_get_snapshot(&snapshot);
			tt_obj_btn_toggle_set_state(btn_modbus,
					snapshot.nw_services.modbus);
		}
		lv_msgbox_close(obj);
	}
}

static void services_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	services_refresh_pending = false;
	if (err != 0) {
		return;
	}
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	if (snapshot.nw_services.valid) {
		tt_obj_btn_toggle_set_state(btn_modbus, snapshot.nw_services.modbus);
	}
}

static void modbus_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	modbus_refresh_pending = false;
	if (err != 0) {
		return;
	}
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	if (snapshot.modbus.valid) {
		char modbus_addr_str[10];
		snprintf(modbus_addr_str, sizeof(modbus_addr_str), "%d",
				snapshot.modbus.addr);
		lv_textarea_set_text(txt_modbus_addr, modbus_addr_str);
	}
}

static void txt_modbus_addr_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_NUM);
		lv_scr_load(kb_scr);
	}
	if (code == LV_EVENT_READY) {
		// TODO: ensure txt_modbus_addr text is an integer
		int addr = atoi(lv_textarea_get_text(txt_modbus_addr));
		if (addr < 0 || addr > 255) {
			tt_obj_info_box_create("ERROR",
					"Modbus address must be in the interval [0-255]", 1);
			return;
		}
		backend_modbus_set_addr(addr, modbus_refresh_cb, NULL);
	}
}

void scr_settings_nw_modbus_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* settings_cont = tt_obj_menu_page_create(menu, btn, menu_cb,
            "Modbus");

    lv_obj_t* settings_nw_cont = tt_obj_cont_create(settings_cont);
    btn_modbus = tt_obj_btn_toggle_create(settings_nw_cont, btn_modbus_cb, "Modbus");
    tt_obj_label_create(settings_nw_cont, "Modbus address");
	txt_modbus_addr = tt_obj_txt_create(settings_nw_cont, "Modbus address", txt_modbus_addr_cb);
}
