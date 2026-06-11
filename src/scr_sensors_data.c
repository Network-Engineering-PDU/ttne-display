#include <stdio.h>
#include <math.h>

#include "lvgl/lvgl.h"

#include "scr_sensors_data.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"

#define TIMER_REFRESH_RATE 1000 // ms

/* Global variables ***********************************************************/

static bool running = false;

static int sensor_id;
static char title[50];
static char sensor_mac[64];
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
static void sensor_data_apply(const models_sensor_live_t* live,
		const models_sensor_t* stored);
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
			LV_LOG_USER("Sensor data open");
		}
	} else if (code == LV_EVENT_CLICKED) {
		if (running) {
			sensor_data_stop_timer();
			LV_LOG_USER("Sensor data close");
		}
	}
}

static void sensor_data_apply(const models_sensor_live_t* live,
		const models_sensor_t* stored)
{
	char txt[50];
	char buff[50];
	float temp = live->temp;
	float humd = live->humd;
	float pres = live->pres;
	int rssi = live->rssi;
	int bat_mv = live->bat_mv;
	int bat_pct = live->bat_pct;
	const char* name = live->name;
	const char* kind = live->kind;

	if (stored != NULL) {
		if (name == NULL || name[0] == '\0') {
			name = stored->name;
		}
		if (!isnan(stored->last_data.temp) && isnan(temp)) {
			temp = stored->last_data.temp;
		}
		if (!isnan(stored->last_data.humd) && isnan(humd)) {
			humd = stored->last_data.humd;
		}
		if (!isnan(stored->last_data.pres) && isnan(pres)) {
			pres = stored->last_data.pres;
		}
		if (rssi <= -200 && stored->last_data.rssi > -200) {
			rssi = stored->last_data.rssi;
		}
		if (bat_mv <= 0 && stored->last_data.bat > 0) {
			if (stored->last_data.bat < 20.0f) {
				bat_mv = (int)(stored->last_data.bat * 1000.0f);
			} else {
				bat_mv = (int)stored->last_data.bat;
			}
		}
	}

	if (kind != NULL && kind[0] != '\0') {
		snprintf(txt, sizeof(txt), "%s (%s)", name, kind);
		lv_label_set_text(lbl_name, txt);
	} else {
		lv_label_set_text(lbl_name, name);
	}

	lv_label_set_text(lbl_mac, live->mac);

	sprintf(txt, "#%06X %s# C", TT_COLOR_GREEN_NE,
			f_float(buff, temp, "%.2f"));
	tt_obj_card_set_text(card_temp, txt);
	sprintf(txt, "#%06X %s# %%RH", TT_COLOR_GREEN_NE,
			f_float(buff, humd, "%.1f"));
	tt_obj_card_set_text(card_humd, txt);
	sprintf(txt, "#%06X %s# hPa", TT_COLOR_GREEN_NE,
			f_float(buff, pres, "%.1f"));
	tt_obj_card_set_text(card_pres, txt);
	sprintf(txt, "#%06X %s# dBm", TT_COLOR_GREEN_NE,
			f_int(buff, rssi, 0, -200));
	tt_obj_card_set_text(card_rssi, txt);

	if (bat_pct > 0 && bat_pct <= 100) {
		sprintf(txt, "#%06X %d# %%", TT_COLOR_GREEN_NE, bat_pct);
	} else {
		sprintf(txt, "#%06X %s# mV", TT_COLOR_GREEN_NE,
				f_int(buff, bat_mv, 5000, 0));
	}
	tt_obj_card_set_text(card_bat, txt);
}

static void sensor_data_timer_cb(lv_timer_t* timer)
{
	(void)timer;
	int len;
	const models_sensor_t* sensors = models_get_sensor(&len);
	const models_sensor_t* stored = NULL;

	if (sensor_id < 0 || sensor_id >= len) {
		return;
	}

	stored = &sensors[sensor_id];
	if (sensor_mac[0] == '\0' && stored->mac != NULL) {
		snprintf(sensor_mac, sizeof(sensor_mac), "%s", stored->mac);
	}

	if (controller_get_sensor_live(sensor_mac)) {
		sensor_data_apply(models_get_sensor_live(), stored);
		return;
	}

	controller_get_sensors();
	sensors = models_get_sensor(&len);
	if (sensor_id < len) {
		const models_sensor_live_t fallback = {
			.mac = sensors[sensor_id].mac,
			.kind = "",
			.name = sensors[sensor_id].name,
			.temp = sensors[sensor_id].last_data.temp,
			.humd = sensors[sensor_id].last_data.humd,
			.pres = sensors[sensor_id].last_data.pres,
			.rssi = sensors[sensor_id].last_data.rssi,
			.bat_mv = (sensors[sensor_id].last_data.bat < 20.0f)
					? (int)(sensors[sensor_id].last_data.bat * 1000.0f)
					: (int)sensors[sensor_id].last_data.bat,
			.bat_pct = -1,
		};
		sensor_data_apply(&fallback, NULL);
	}
}

/* Function definitions *******************************************************/

const char* f_int(char* buffer, int num, int max, int min)
{
	if (num > min && num < max) {
		sprintf(buffer, "%d", num);
	} else {
		strcpy(buffer, "N/A");
	}
	return buffer;
}

const char* f_float(char* buffer, float num, const char* format)
{
	if (num == num) {
		sprintf(buffer, format, num);
	} else {
		strcpy(buffer, "N/A");
	}
	return buffer;
}

/* Public functions ***********************************************************/

lv_obj_t* scr_sensors_data_create(lv_obj_t* menu)
{
	sprintf(title, "Sensors");
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
	int len;
	const models_sensor_t* sensors = models_get_sensor(&len);

	sensor_id = l_sensor - 1;
	sensor_mac[0] = '\0';

	if (sensor_id >= 0 && sensor_id < len &&
			sensors[sensor_id].name != NULL &&
			sensors[sensor_id].name[0] != '\0') {
		snprintf(title, sizeof(title), "Sensors / %s",
				sensors[sensor_id].name);
	} else {
		snprintf(title, sizeof(title), "Sensors / Sensor %d", l_sensor);
	}
	if (sensor_data_page != NULL) {
		lv_menu_set_page_title(sensor_data_page, title);
	}
}
