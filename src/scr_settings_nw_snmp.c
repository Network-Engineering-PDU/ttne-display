#include "lvgl/lvgl.h"
#include "scr_settings_nw_snmp.h"
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

/* Global variables ***********************************************************/
static lv_obj_t* btn_snmp;
static bool running = false;

/* Function prototypes ********************************************************/
static void menu_cb(lv_event_t* e);
static void btn_snmp_cb(lv_event_t* e);
static void msg_box_snmp_cb(lv_event_t* e);

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
			tt_obj_btn_toggle_set_state(btn_snmp, nw_services->snmp);
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
		sprintf(msg, "Are you sure you want to " TT_COLOR_GREEN_NE_STR
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
			if (lv_obj_get_state(btn_snmp) & LV_STATE_CHECKED) {
				controller_post_start_snmp();
			} else {
				controller_post_stop_snmp();
			}
		} else {
			const models_nw_services_t* nw_services = models_get_nw_services();
			tt_obj_btn_toggle_set_state(btn_snmp, nw_services->snmp);
		}
		lv_msgbox_close(obj);
	}
}

void scr_settings_nw_snmp_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* settings_cont = tt_obj_menu_page_create(menu, btn, menu_cb,
			"SNMP Settings");

    lv_obj_t* settings_nw_cont = tt_obj_cont_create(settings_cont);
    btn_snmp = tt_obj_btn_toggle_create(settings_nw_cont, btn_snmp_cb, "SNMP");
}
