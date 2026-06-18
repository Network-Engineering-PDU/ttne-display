#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "scr_sensors_data.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define TIMER_REFRESH_RATE 1000 // ms

/* Global variables ***********************************************************/

static bool running = false;
static bool refresh_pending;

static int sensor_id;
static char title[96];
static lv_obj_t* sensor_data_page;

static lv_obj_t* lbl_mac;
static lv_obj_t* lbl_name;
static lv_obj_t* card_info;
static lv_obj_t* card_temp;
static lv_obj_t* card_humd;
static lv_obj_t* card_pres;
static lv_obj_t* card_rssi;
static lv_obj_t* card_bat;

static lv_timer_t* timer;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void sensor_data_timer_cb(lv_timer_t* timer);
static void sensor_data_refresh_cb(int err, void* userdata);
static void sensor_data_apply(const app_state_sensor_data_t* sensor_data);
static void sensor_data_apply_snapshot(void);
static void sensor_data_stop_timer(void);

const char* f_int(char* buffer, int num, int max, int min);
const char* f_float(char* buffer, float num, const char* format);

/* Callbacks ******************************************************************/

static void sensor_data_stop_timer(void)
{
	if (timer != NULL) {
		lv_timer_del(timer);
		timer = NULL;
	}
	refresh_pending = false;
	running = false;
}

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			sensor_data_stop_timer();
			timer = lv_timer_create(sensor_data_timer_cb,
					TIMER_REFRESH_RATE, NULL);
			sensor_data_timer_cb(timer);
			running = true;
		}
	} else if (code == LV_EVENT_CLICKED) {
		if (running) {
			sensor_data_stop_timer();
		}
	}
}

static void sensor_data_apply(const app_state_sensor_data_t* sensor_data)
{
	char txt[160];
	char buff[50];
	const char* name = sensor_data->name;
	const char* kind = sensor_data->kind;

	if (kind != NULL && kind[0] != '\0') {
		snprintf(txt, sizeof(txt), "%s (%s)", name, kind);
		lv_label_set_text(lbl_name, txt);
	} else {
		lv_label_set_text(lbl_name, name);
	}

	lv_label_set_text(lbl_mac, sensor_data->mac);

	snprintf(txt, sizeof(txt), "#%06X %s# C", TT_COLOR_GREEN_NE,
			f_float(buff, sensor_data->temp, "%.2f"));
	tt_obj_card_set_text(card_temp, txt);
	snprintf(txt, sizeof(txt), "#%06X %s# %%RH", TT_COLOR_GREEN_NE,
			f_float(buff, sensor_data->humd, "%.1f"));
	tt_obj_card_set_text(card_humd, txt);
	snprintf(txt, sizeof(txt), "#%06X %s# hPa", TT_COLOR_GREEN_NE,
			f_float(buff, sensor_data->pres, "%.1f"));
	tt_obj_card_set_text(card_pres, txt);
	snprintf(txt, sizeof(txt), "#%06X %s# dBm", TT_COLOR_GREEN_NE,
			f_int(buff, sensor_data->rssi, 0, -200));
	tt_obj_card_set_text(card_rssi, txt);

	if (sensor_data->bat_pct > 0 && sensor_data->bat_pct <= 100) {
		snprintf(txt, sizeof(txt), "#%06X %d# %%",
				TT_COLOR_GREEN_NE, sensor_data->bat_pct);
	} else {
		snprintf(txt, sizeof(txt), "#%06X %s# mV", TT_COLOR_GREEN_NE,
				f_int(buff, sensor_data->bat_mv, 5000, 0));
	}
	tt_obj_card_set_text(card_bat, txt);

	if (name != NULL && name[0] != '\0') {
		snprintf(title, sizeof(title), "Sensors / %s", name);
		if (sensor_data_page != NULL) {
			lv_menu_set_page_title(sensor_data_page, title);
		}
	}
}

static void sensor_data_timer_cb(lv_timer_t* timer)
{
	(void)timer;
	if (refresh_pending || sensor_id < 0) {
		return;
	}

	if (backend_sensor_data_refresh(sensor_id, sensor_data_refresh_cb, NULL) == 0) {
		refresh_pending = true;
	}
}

static void sensor_data_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	refresh_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("Sensors", "Could not refresh sensor data", 1);
		return;
	}
	sensor_data_apply_snapshot();
}

static void sensor_data_apply_snapshot(void)
{
	app_state_snapshot_t snapshot;

	app_state_get_snapshot(&snapshot);
	if (snapshot.sensor_data.valid &&
			snapshot.sensor_data.sensor_index == sensor_id) {
		sensor_data_apply(&snapshot.sensor_data);
	}
}

/* Function definitions *******************************************************/

const char* f_int(char* buffer, int num, int max, int min)
{
	if (num > min && num < max) {
		snprintf(buffer, 50, "%d", num);
	} else {
		strcpy(buffer, "N/A");
	}
	return buffer;
}

const char* f_float(char* buffer, float num, const char* format)
{
	if (num == num) {
		snprintf(buffer, 50, format, num);
	} else {
		strcpy(buffer, "N/A");
	}
	return buffer;
}

/* Public functions ***********************************************************/

lv_obj_t* scr_sensors_data_create(lv_obj_t* menu)
{
	snprintf(title, sizeof(title), "Sensors");
	lv_obj_t* sensor_data_cont = tt_obj_menu_page_create(menu, NULL, menu_cb,
			title);
	sensor_data_page = lv_obj_get_parent(sensor_data_cont);

	lv_obj_set_flex_flow(sensor_data_cont, LV_FLEX_FLOW_ROW_WRAP);

	card_info = tt_obj_card_create(sensor_data_cont, "", NULL);
	lbl_mac = tt_obj_label_create(card_info, "");
	lv_obj_align(lbl_mac, LV_ALIGN_TOP_MID, 0, 0);
	lbl_name = tt_obj_label_create(card_info, "");
	lv_obj_align(lbl_name, LV_ALIGN_CENTER, 0, 0);
	card_rssi = tt_obj_card_create(sensor_data_cont, "", ASSET("rssi.png"));
	card_bat = tt_obj_card_create(sensor_data_cont, "", ASSET("bat.png"));
	card_temp = tt_obj_card_create(sensor_data_cont, "", ASSET("temp.png"));
	card_humd = tt_obj_card_create(sensor_data_cont, "", ASSET("humd.png"));
	card_pres = tt_obj_card_create(sensor_data_cont, "", ASSET("pres.png"));

	return lv_obj_get_parent(sensor_data_cont);
}

void scr_sensors_data_set_sensor(int l_sensor)
{
	sensor_id = l_sensor - 1;

	snprintf(title, sizeof(title), "Sensors / Sensor %d", l_sensor);
	if (sensor_data_page != NULL) {
		lv_menu_set_page_title(sensor_data_page, title);
	}
}
