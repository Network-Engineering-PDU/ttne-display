#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_sensors.h"
#include "scr_sensors_data.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "models.h"
#include "controller.h"

#define MAX_SENSORS 8
#define TIMER_SCAN 6000 // ms
#define MAX_SCAN_RETRIES 10 // 10*6000ms = 60 seconds scan timout


/* Global variables ***********************************************************/

static lv_obj_t* menu;

static lv_obj_t* loader_scr;

static lv_timer_t* timer;

static lv_obj_t* btn_sensor[MAX_SENSORS];
static lv_obj_t* lbl_mac[MAX_SENSORS];

static lv_obj_t* sensors_data_page;
static lv_obj_t* lbl_no_sensors;

static int old_len;
static int scan_retries;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void cancel_loader_cb(lv_event_t* e);
static void add_sensor_cb(lv_event_t* e);
static void btn_sensor_cb(lv_event_t* e);
static void timer_scan_cb(lv_timer_t* timer);

static void update_sensors();

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			update_sensors();
		}
	}
}


static void cancel_loader_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		LV_LOG_USER("CANCEL");
		scan_retries = MAX_SCAN_RETRIES;
		timer_scan_cb(timer);
	}
}

static void add_sensor_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		int len;
		models_get_sensor(&len);
		if (len == MAX_SENSORS) {
			tt_obj_info_box_create("ERROR", "Max number of sensor reached", 1);
			return;
		}
		old_len = len;
		scan_retries = 0;
		controller_post_start_scan();
		timer = lv_timer_create(timer_scan_cb, TIMER_SCAN,
				lv_scr_act());
		loader_scr = tt_obj_loader_create("Searching sensors...",
				cancel_loader_cb);
		lv_scr_load(loader_scr);
	}
}

static void btn_sensor_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	int sensor_id = (long)lv_event_get_user_data(e);

	if (code == LV_EVENT_CLICKED) {
		scr_sensors_data_set_sensor(sensor_id);
		lv_menu_set_page(menu, sensors_data_page);	
	}
}

static void timer_scan_cb(lv_timer_t* timer)
{
	lv_obj_t* scr = timer->user_data;

	controller_get_sensors();
	int len;
	const models_sensor_t* sensors = models_get_sensor(&len);
	// old_len global or user_data?
	if (scan_retries++ >= MAX_SCAN_RETRIES) {
		controller_post_stop_scan();
		lv_timer_del(timer);
		lv_scr_load(scr);
		lv_obj_del(loader_scr);
		tt_obj_info_box_create("New sensor", "No sensor found", 1);
	}
	if (len != old_len) {
		models_sensor_t new_sensor = sensors[len-1];
		char msg[100];
		sprintf(msg, "Found new sensor with mac\n%s", new_sensor.mac);
		lv_timer_del(timer);
		lv_scr_load(scr);
		lv_obj_del(loader_scr);
		tt_obj_info_box_create("New sensor", msg, 0);
		update_sensors();
	}
	LV_LOG_USER("Searching sensors. Retries: %d (old_len=%d, len=%d)",
			scan_retries, old_len, len);
}

/* Function definitions *******************************************************/

static void update_sensors()
{
	controller_get_sensors();
	int len;
	const models_sensor_t* sensors = models_get_sensor(&len);
	if (len == 0) {
		lv_obj_clear_flag(lbl_no_sensors, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(lbl_no_sensors, LV_OBJ_FLAG_HIDDEN);
	}
	for (int i = 0; i < MAX_SENSORS; i++) {
		if (i < len) {
			lv_obj_clear_flag(btn_sensor[i], LV_OBJ_FLAG_HIDDEN);
			lv_label_set_text(lbl_mac[i], sensors[i].mac);			
		} else {
			lv_obj_add_flag(btn_sensor[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
}

/* Public functions ***********************************************************/

void scr_sensors_create(lv_obj_t* l_menu, lv_obj_t* btn)
{
	menu = l_menu;

	lv_obj_t* sensors_cont = tt_obj_menu_page_create(menu, btn, menu_cb, "Sensors");
	sensors_data_page = scr_sensors_data_create(menu);
	tt_obj_btn_perc_create(sensors_cont, add_sensor_cb, "ADD SENSOR", 100);
	lbl_no_sensors = tt_obj_label_color_create(sensors_cont, "No sensors found");
	lv_obj_add_flag(lbl_no_sensors, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_flex_flow(sensors_cont, LV_FLEX_FLOW_ROW_WRAP);

	for (int i = 0; i < MAX_SENSORS; i++) {
		char str[10];
		sprintf(str, "SENSOR %d", i + 1);
		btn_sensor[i] = tt_obj_btn_perc_create(sensors_cont, NULL, str, 48);
		lv_obj_add_event_cb(btn_sensor[i], btn_sensor_cb, LV_EVENT_ALL,
				(void*)(long)(i + 1));
		lv_obj_t* lbl_name = lv_obj_get_child(btn_sensor[i], 0);
		lv_obj_align(lbl_name, LV_ALIGN_TOP_MID, 0, 0);
		lbl_mac[i] = lv_label_create(btn_sensor[i]);
		lv_obj_align(lbl_mac[i], LV_ALIGN_BOTTOM_MID, 0, 0);
		lv_obj_set_scroll_dir(btn_sensor[i], LV_DIR_NONE);
	}
}