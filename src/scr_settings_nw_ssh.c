#include <stdio.h>

#include "lvgl/lvgl.h"
#include "scr_settings_nw_ssh.h"
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

static lv_obj_t* btn_ssh;
static bool running = false;

/* Function prototypes ********************************************************/
static void menu_cb(lv_event_t* e);
static void btn_ssh_cb(lv_event_t* e);
static void msg_box_ssh_cb(lv_event_t* e);

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
			tt_obj_btn_toggle_set_state(btn_ssh, nw_services->ssh);
			if (!running) {
				running = true;
				LV_LOG_USER("SSH Settings Page Opened");
			}
		}
	} else if (code == LV_EVENT_CLICKED) {
		if (running) {
			running = false;
			LV_LOG_USER("SSH Settings Page Closed");
		}
	}
}

static void btn_ssh_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* btn = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		char msg[100];
		sprintf(msg, "Are you sure you want to " TT_COLOR_GREEN_NE_STR
				" %s# SSH?", (lv_obj_get_state(btn) & LV_STATE_CHECKED) ?
				"enable" : "disable");
		tt_obj_msg_box_create("SSH service", msg, NULL, msg_box_ssh_cb);
	}
}

static void msg_box_ssh_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			bool enable = (lv_obj_get_state(btn_ssh) & LV_STATE_CHECKED) ? true : false;
			if (enable) {
				controller_post_start_ssh();
			} else {
				controller_post_stop_ssh();
			}
			// Update the model state immediately for UI consistency
			models_nw_services_t* nw_services = (models_nw_services_t*)models_get_nw_services();
			if (nw_services) {
				nw_services->ssh = enable;
			}
		} else {
			const models_nw_services_t* nw_services = models_get_nw_services();
			tt_obj_btn_toggle_set_state(btn_ssh, nw_services->ssh);
		}
		lv_msgbox_close(obj);
	}
}

void scr_settings_nw_ssh_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* settings_cont = tt_obj_menu_page_create(menu, btn, menu_cb,
			"SSH");

    lv_obj_t* settings_nw_cont = tt_obj_cont_create(settings_cont);
    btn_ssh = tt_obj_btn_toggle_create(settings_nw_cont, btn_ssh_cb, "SSH");
}
