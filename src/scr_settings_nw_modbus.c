#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "scr_settings_nw_modbus.h"
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
#include "scr_keyboard.h"

/* Global variables ***********************************************************/
static lv_obj_t* txt_modbus_addr;
static lv_obj_t* btn_modbus;
static bool running = false;

/* Function prototypes ********************************************************/
static void btn_modbus_cb(lv_event_t* e);
static void msg_box_modbus_cb(lv_event_t* e);
static void txt_modbus_addr_cb(lv_event_t* e);

/* Callbacks ******************************************************************/
static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(menu);
		if (curr_page == page) {
			controller_get_nw_services();
			const models_nw_services_t* nw_services = models_get_nw_services();
			tt_obj_btn_toggle_set_state(btn_modbus, nw_services->modbus);
			controller_get_modbus();
			const models_modbus_t* modbus = models_get_modbus();
			char modbus_addr_str[10];
			sprintf(modbus_addr_str, "%d", modbus->addr);
			lv_textarea_set_text(txt_modbus_addr, modbus_addr_str);
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
		sprintf(msg, "Are you sure you want to " TT_COLOR_GREEN_NE_STR
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
			if (lv_obj_get_state(btn_modbus) & LV_STATE_CHECKED) {
				controller_post_start_modbus();
			} else {
				controller_post_stop_modbus();
			}
		} else {
			const models_nw_services_t* nw_services = models_get_nw_services();
			tt_obj_btn_toggle_set_state(btn_modbus, nw_services->modbus);
		}
		lv_msgbox_close(obj);
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
		models_modbus_t modbus;
		modbus.addr = atoi(lv_textarea_get_text(txt_modbus_addr));
		if (modbus.addr < 0 || modbus.addr > 255) {
			tt_obj_info_box_create("ERROR",
					"Modbus address must be in the interval [0-255]", 1);
			return;
		}
		controller_put_modbus(&modbus);
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
