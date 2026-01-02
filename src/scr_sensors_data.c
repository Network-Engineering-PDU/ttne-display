#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_sensors_data.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"

#define TIMER_REFRESH_RATE 60000 // ms

#define BAT_TH_MAX 3.1
#define BAT_TH_MED 2.9
#define BAT_TH_MIN 2.6

/* Global variables ***********************************************************/

static bool running = false;

static int sensor_id;
static char title[50];

static lv_obj_t* lbl_mac;
static lv_obj_t* lbl_name;
static lv_obj_t* lbl_dt;
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

const char* f_int(char* buffer, int num, int max, int min);
const char* f_float(char* buffer, float num, const char* format);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			timer = lv_timer_create(sensor_data_timer_cb,
					TIMER_REFRESH_RATE, NULL);
			sensor_data_timer_cb(timer);
			if (!running) {
				running = true;
				LV_LOG_USER("Sensor data open");
			}
		}
	} else if (code == LV_EVENT_CLICKED) {
		if (running) {
			running = false;
			lv_timer_del(timer);
			LV_LOG_USER("Sensor data close");
		}
	}
}

static void sensor_data_timer_cb(lv_timer_t* timer)
{
	(void)timer;
	controller_get_sensors();
	int len;
	const models_sensor_t* sensors = models_get_sensor(&len);
	char txt[50];
	char buff[50];
	models_sensor_t sensor = sensors[sensor_id];
	
	// TODO: Crear un boton de info al que le das y sale un infobox con toda la información restante?
	lv_label_set_text(lbl_mac, sensor.mac);
	lv_label_set_text(lbl_name, sensor.name);
	lv_label_set_text(lbl_dt, sensor.last_data.datetime);
	sprintf(txt, "#%06X %s# C", TT_COLOR_GREEN_NE,
			f_float(buff, sensor.last_data.temp, "%.2f"));
	tt_obj_card_set_text(card_temp, txt);
	sprintf(txt, "#%06X %s# %%RH", TT_COLOR_GREEN_NE,
			f_float(buff, sensor.last_data.humd, "%.1f"));
	tt_obj_card_set_text(card_humd, txt);
	sprintf(txt, "#%06X %s# hPa", TT_COLOR_GREEN_NE,
			f_float(buff, sensor.last_data.pres / 100, "%.2f"));
	tt_obj_card_set_text(card_pres, txt);
	sprintf(txt, "#%06X %s# dBm", TT_COLOR_GREEN_NE,
			f_int(buff, sensor.last_data.rssi, 0, -200));
	tt_obj_card_set_text(card_rssi, txt);
	sprintf(txt, "#%06X %s# mV", TT_COLOR_GREEN_NE,
			f_int(buff, sensor.last_data.bat, 5000, 0));
	tt_obj_card_set_text(card_bat, txt);
	//TODO: check bat and set img
	// if (bat >= BAT_TH_MAX) {
	// 	tt_obj_card_set_img(card_bat, ASSET("bat3.png"))
	// } else if (bat < BAT_TH_MAX && bat >= BAT_TH_MED) {
	// 	tt_obj_card_set_img(card_bat, ASSET("bat2.png"))
	// } else if (bat < BAT_TH_MED && bat >= BAT_TH_MIN) {
	// 	tt_obj_card_set_img(card_bat, ASSET("bat1.png"))
	// } else if (bat < BAT_TH_MIN) {
	// 	tt_obj_card_set_img(card_bat, ASSET("bat0.png"))
	// }
}

/* Function definitions *******************************************************/

const char* f_int(char* buffer, int num, int max, int min) {
	if (num > min && num < max) {
		sprintf(buffer, "%d", num);
	} else {
		strcpy(buffer, "N/A");
	}
	return buffer;
}

const char* f_float(char* buffer, float num, const char* format) {
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
	sprintf(title, "TTNULL");
	lv_obj_t* sensor_data_cont = tt_obj_menu_page_create(menu, NULL, menu_cb,
			title);

	lv_obj_set_flex_flow(sensor_data_cont, LV_FLEX_FLOW_ROW_WRAP);

	lbl_mac = tt_obj_label_create(card_info, "");
	lv_obj_align(lbl_mac, LV_ALIGN_TOP_MID, 0, 0);
	lbl_name = tt_obj_label_create(card_info, "");
	lv_obj_align(lbl_name, LV_ALIGN_CENTER, 0, 0);
	lbl_dt = tt_obj_label_create(card_info, "");
	lv_obj_align(lbl_dt, LV_ALIGN_BOTTOM_MID, 0, 0);
	card_info = tt_obj_card_create(sensor_data_cont, "", NULL);
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
	sprintf(title, "Sensors / Sensor %d", l_sensor);
}
