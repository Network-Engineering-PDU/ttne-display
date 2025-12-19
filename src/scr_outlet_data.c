#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_outlet_data.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"

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

static models_out_sw_t out_sw;

static lv_timer_t* timer;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void out_data_timer_cb(lv_timer_t* timer);
static void btn_toggle_cb(lv_event_t* e);
static void msg_box_outlet_cb(lv_event_t* e);

static void read_license();
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
			controller_get_out_sw();
			const models_out_sw_t* current_out_sw =
					models_get_out_sw_id(outlet_id);
			memcpy(&out_sw, current_out_sw, sizeof(models_out_sw_t));
			tt_obj_btn_toggle_set_state(btn_en, out_sw.status);
			timer = lv_timer_create(out_data_timer_cb,
					TIMER_REFRESH_RATE, NULL);
			read_license();
			out_data_timer_cb(timer);
			if (!running) {
				running = true;
				LV_LOG_USER("Outlet data open");
			}
		}
	} else if (code == LV_EVENT_CLICKED) {
		if (running) {
			running = false;
			lv_timer_del(timer);
			LV_LOG_USER("Outlet data close");
		}
	}
}

static void out_data_timer_cb(lv_timer_t* timer)
{
	(void)timer;
	controller_get_out_data(outlet_id);
	const models_out_data_t* out_data = models_get_out_data();
	char label[50];
	sprintf(label, "Voltage: #%06X %.1f V" ,
			TT_COLOR_GREEN_NE, out_data->voltage);
	lv_label_set_text(lbl_v, label);
	sprintf(label, "Current: #%06X %.1f A" ,
			TT_COLOR_GREEN_NE, out_data->current);
	lv_label_set_text(lbl_i, label);
	sprintf(label, "Active power: #%06X %.1f W" ,
			TT_COLOR_GREEN_NE, out_data->active_power);
	lv_label_set_text(lbl_p, label);
	sprintf(label, "Reactive power: #%06X %.2f VAr" ,
			TT_COLOR_GREEN_NE, out_data->reactive_power);
	lv_label_set_text(lbl_q, label);
	sprintf(label, "Apparent power: #%06X %.2f VA" ,
			TT_COLOR_GREEN_NE, out_data->apparent_power);
	lv_label_set_text(lbl_s, label);
	sprintf(label, "Power factor: #%06X %.2f" ,
			TT_COLOR_GREEN_NE, out_data->power_factor);
	lv_label_set_text(lbl_pf, label);
	sprintf(label, "Frequency: #%06X %.2f Hz" ,
			TT_COLOR_GREEN_NE, out_data->frequency);
	lv_label_set_text(lbl_f, label);
	sprintf(label, "Phase: #%06X %.1f deg" ,
			TT_COLOR_GREEN_NE, out_data->phase);
	lv_label_set_text(lbl_ph, label);
	sprintf(label, "Energy: #%06X %.1f Wh" ,
			TT_COLOR_GREEN_NE, out_data->energy);
	lv_label_set_text(lbl_e, label);
	sprintf(label, "Connector: #%06X %s" ,
			TT_COLOR_GREEN_NE, out_data->conn);
	lv_label_set_text(lbl_c, label);
	sprintf(label, "Fuse: #%06X %s" ,
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
			out_sw.status = lv_obj_get_state(btn_en) & LV_STATE_CHECKED;
			controller_put_out_sw(&out_sw, outlet_id-1);
		} else {
			tt_obj_btn_toggle_set_state(btn_en, out_sw.status);
		}
		lv_msgbox_close(obj);
	}
}

/* Function definitions *******************************************************/

static void read_license()
{
	controller_get_license();
	const models_license_t* license = models_get_license();
	LV_LOG_USER("License readed: %s", license->type_id);
	if (strcmp(license->type_id, "A1") == 0) {
		LV_LOG_USER("A1");
		lv_obj_add_flag(btn_en, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(data_cont, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_relay_license, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_data_license, LV_OBJ_FLAG_HIDDEN);
	} else if (strcmp(license->type_id, "A2") == 0) {
		LV_LOG_USER("A2");
		lv_obj_add_flag(btn_en, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(data_cont, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_relay_license, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_data_license, LV_OBJ_FLAG_HIDDEN);
	} else if (strcmp(license->type_id, "B1") == 0) {
		LV_LOG_USER("B1");
		lv_obj_clear_flag(btn_en, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(data_cont, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_relay_license, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_data_license, LV_OBJ_FLAG_HIDDEN);
	} else if (strcmp(license->type_id, "B2") == 0) {
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
