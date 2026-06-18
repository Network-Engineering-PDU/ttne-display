#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "scr_outlet_data.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define TIMER_REFRESH_RATE 2000 // ms

typedef enum {
	FUSE_UNK = 0,
	FUSE_OK  = 1,
	FUSE_NOK = 2,
	FUSE_NA  = 3,
} fuse_t;

/* Global variables ***********************************************************/

static bool running = false;

static int outlet_id;
static char title[50];

static lv_obj_t* btn_en;
static lv_obj_t* data_cont;
static lv_obj_t* lbl_relay_license;
static lv_obj_t* lbl_data_license;

static lv_obj_t* lbl_v;
static lv_obj_t* lbl_i;
static lv_obj_t* lbl_p;
static lv_obj_t* lbl_q;
static lv_obj_t* lbl_s;
static lv_obj_t* lbl_pf;
static lv_obj_t* lbl_f; 
static lv_obj_t* lbl_ph;
static lv_obj_t* lbl_e;
static lv_obj_t* lbl_c;
static lv_obj_t* lbl_fuse;

static bool out_sw_status;
static bool data_request_pending;
static bool license_request_pending;

static lv_timer_t* timer;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void out_data_timer_cb(lv_timer_t* timer);
static void btn_toggle_cb(lv_event_t* e);
static void msg_box_outlet_cb(lv_event_t* e);

static void outlet_data_refresh_cb(int err, void* userdata);
static void outlet_license_refresh_cb(int err, void* userdata);
static void outlet_set_cb(int err, void* userdata);
static void apply_outlet_status(void);
static void apply_outlet_data(void);
static void apply_license(void);
static const char* get_fuse_str(fuse_t fuse_code);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

		if (code == LV_EVENT_VALUE_CHANGED) {
			lv_obj_t* curr_page = lv_event_get_user_data(e);
			lv_obj_t* page = lv_menu_get_cur_main_page(obj);
			if (curr_page == page) {
				apply_outlet_status();
				if (timer == NULL) {
					timer = lv_timer_create(out_data_timer_cb,
							TIMER_REFRESH_RATE, NULL);
				}
				if (!license_request_pending &&
						backend_license_refresh(outlet_license_refresh_cb,
								NULL) == 0) {
					license_request_pending = true;
				}
				out_data_timer_cb(timer);
				if (!running) {
				running = true;
				LV_LOG_USER("Outlet data open");
			}
		}
		} else if (code == LV_EVENT_CLICKED) {
			if (running) {
				running = false;
				if (timer != NULL) {
					lv_timer_del(timer);
					timer = NULL;
				}
				LV_LOG_USER("Outlet data close");
			}
		}
}

static void out_data_timer_cb(lv_timer_t* timer)
{
	(void)timer;
	if (data_request_pending) {
		return;
	}
	if (backend_outlet_data_refresh(outlet_id, outlet_data_refresh_cb,
			NULL) == 0) {
		data_request_pending = true;
	}
}

static void outlet_data_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	data_request_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("Outlet data", "Could not refresh outlet data", 1);
		return;
	}
	apply_outlet_data();
}

static void outlet_license_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	license_request_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("Outlet data", "Could not read license", 1);
		return;
	}
	apply_license();
}

static void outlet_set_cb(int err, void* userdata)
{
	bool requested_status = (bool)(intptr_t)userdata;
	if (err != 0) {
		tt_obj_btn_toggle_set_state(btn_en, out_sw_status);
		tt_obj_info_box_create("Outlet", "Could not update outlet", 1);
		return;
	}
	out_sw_status = requested_status;
	tt_obj_btn_toggle_set_state(btn_en, out_sw_status);
}

static void apply_outlet_status(void)
{
	app_state_snapshot_t snapshot;
	int index = outlet_id - 1;

	app_state_get_snapshot(&snapshot);
	if (index >= 0 && index < snapshot.outlet_count &&
			index < APP_STATE_MAX_OUTLETS) {
		out_sw_status = snapshot.outlets[index].status;
		tt_obj_btn_toggle_set_state(btn_en, out_sw_status);
	}
}

static void apply_outlet_data(void)
{
	app_state_snapshot_t snapshot;
	char label[96];
	app_state_get_snapshot(&snapshot);
	const app_state_outlet_data_t* out_data = &snapshot.outlet_data;

	if (!out_data->valid || out_data->outlet_id != outlet_id) {
		return;
	}

	snprintf(label, sizeof(label), "Voltage: #%06X %.1f V" ,
			TT_COLOR_GREEN_NE, out_data->voltage);
	lv_label_set_text(lbl_v, label);
	snprintf(label, sizeof(label), "Current: #%06X %.1f A" ,
			TT_COLOR_GREEN_NE, out_data->current);
	lv_label_set_text(lbl_i, label);
	snprintf(label, sizeof(label), "Active power: #%06X %.1f W" ,
			TT_COLOR_GREEN_NE, out_data->active_power);
	lv_label_set_text(lbl_p, label);
	snprintf(label, sizeof(label), "Reactive power: #%06X %.2f VAr" ,
			TT_COLOR_GREEN_NE, out_data->reactive_power);
	lv_label_set_text(lbl_q, label);
	snprintf(label, sizeof(label), "Apparent power: #%06X %.2f VA" ,
			TT_COLOR_GREEN_NE, out_data->apparent_power);
	lv_label_set_text(lbl_s, label);
	snprintf(label, sizeof(label), "Power factor: #%06X %.2f" ,
			TT_COLOR_GREEN_NE, out_data->power_factor);
	lv_label_set_text(lbl_pf, label);
	snprintf(label, sizeof(label), "Frequency: #%06X %.2f Hz" ,
			TT_COLOR_GREEN_NE, out_data->frequency);
	lv_label_set_text(lbl_f, label);
	snprintf(label, sizeof(label), "Phase: #%06X %.1f deg" ,
			TT_COLOR_GREEN_NE, out_data->phase);
	lv_label_set_text(lbl_ph, label);
	snprintf(label, sizeof(label), "Energy: #%06X %.1f Wh" ,
			TT_COLOR_GREEN_NE, out_data->energy);
	lv_label_set_text(lbl_e, label);
	snprintf(label, sizeof(label), "Connector: #%06X %s" ,
			TT_COLOR_GREEN_NE, out_data->conn);
	lv_label_set_text(lbl_c, label);
	snprintf(label, sizeof(label), "Fuse: #%06X %s" ,
			TT_COLOR_GREEN_NE, get_fuse_str(out_data->fuse));
	lv_label_set_text(lbl_fuse, label);
}

static void btn_toggle_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* btn = lv_event_get_current_target(e);

	if (code == LV_EVENT_CLICKED) {
		char title[50];
		char msg[100];
		sprintf(title, "Outlet %d", outlet_id);
		sprintf(msg, "Are you sure you want to " TT_COLOR_GREEN_NE_STR
				" %s outlet %d#?",
				(lv_obj_get_state(btn) & LV_STATE_CHECKED) ?
				"enable" : "disable", outlet_id);
		tt_obj_msg_box_create(title, msg, NULL, msg_box_outlet_cb);
	}
}

static void msg_box_outlet_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			bool requested_status =
					(lv_obj_get_state(btn_en) & LV_STATE_CHECKED) != 0;
			if (backend_outlet_set(outlet_id - 1, requested_status,
					outlet_set_cb, (void*)(intptr_t)requested_status) != 0) {
				tt_obj_btn_toggle_set_state(btn_en, out_sw_status);
				tt_obj_info_box_create("Outlet", "Outlet update unavailable", 1);
			}
		} else {
			tt_obj_btn_toggle_set_state(btn_en, out_sw_status);
		}
		lv_msgbox_close(obj);
	}
}

/* Function definitions *******************************************************/

static void apply_license(void)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);

	if (strcmp(snapshot.license_type, "A1") == 0) {
		LV_LOG_USER("A1");
		lv_obj_add_flag(btn_en, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(data_cont, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_relay_license, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_data_license, LV_OBJ_FLAG_HIDDEN);
	} else if (strcmp(snapshot.license_type, "A2") == 0) {
		LV_LOG_USER("A2");
		lv_obj_add_flag(btn_en, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(data_cont, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_relay_license, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_data_license, LV_OBJ_FLAG_HIDDEN);
	} else if (strcmp(snapshot.license_type, "B1") == 0) {
		LV_LOG_USER("B1");
		lv_obj_clear_flag(btn_en, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(data_cont, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_relay_license, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_data_license, LV_OBJ_FLAG_HIDDEN);
	} else if (strcmp(snapshot.license_type, "B2") == 0) {
		LV_LOG_USER("B2");
		lv_obj_clear_flag(btn_en, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(data_cont, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_relay_license, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_data_license, LV_OBJ_FLAG_HIDDEN);
	} else {
		LV_LOG_USER("An error has occured");
	}
}

static const char* get_fuse_str(fuse_t fuse_code)
{
	switch (fuse_code) {
	case FUSE_UNK:
		return "UNKNOWN";
	case FUSE_OK:
		return "PRESENT"; // OK
	case FUSE_NOK:
		return "NO VOLTAGE";
	case FUSE_NA:
		return "NOT AVAILABLE";
	default:
		return "FUSE ERROR";
	}
}

/* Public functions ***********************************************************/

lv_obj_t* scr_outlet_data_create(lv_obj_t* menu)
{
	sprintf(title, "TTNULL");
	lv_obj_t* outlet_data_cont = tt_obj_menu_page_create(menu, NULL, menu_cb,
			title);

	btn_en = tt_obj_btn_toggle_create(outlet_data_cont, btn_toggle_cb,
			"Input enabled");
	lbl_relay_license = tt_obj_label_color_create(outlet_data_cont,
			"Control outlet upgrading your license");

	data_cont = tt_obj_cont_create(outlet_data_cont);

	lbl_v = tt_obj_label_color_create(data_cont, "");
	lbl_i = tt_obj_label_color_create(data_cont, "");
	lbl_p = tt_obj_label_color_create(data_cont, "");
	lbl_q = tt_obj_label_color_create(data_cont, "");
	lbl_s = tt_obj_label_color_create(data_cont, "");
	lbl_pf = tt_obj_label_color_create(data_cont, "");
	lbl_f = tt_obj_label_color_create(data_cont, "");
	lbl_ph = tt_obj_label_color_create(data_cont, "");
	lbl_e = tt_obj_label_color_create(data_cont, "");
	lbl_c = tt_obj_label_color_create(data_cont, "");
	lbl_fuse = tt_obj_label_color_create(data_cont, "");
	lbl_data_license = tt_obj_label_color_create(outlet_data_cont,
			"Read outlet data upgrading your license");

	return lv_obj_get_parent(outlet_data_cont);
}

void scr_outlet_data_set_out(int l_outlet)
{
	outlet_id = l_outlet;
	sprintf(title, "Outlets / Outlet %d", outlet_id);
}
