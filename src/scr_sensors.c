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
static lv_obj_t* selection_scr;

static lv_timer_t* timer;

static lv_obj_t* btn_sensor[MAX_SENSORS];
static lv_obj_t* lbl_mac[MAX_SENSORS];
static lv_obj_t* btn_found_sensor[MAX_SENSORS];

static lv_obj_t* sensors_data_page;
static lv_obj_t* lbl_no_sensors;

static int scan_retries;
static int initial_sensor_count;
static lv_obj_t* current_main_scr;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void cancel_loader_cb(lv_event_t* e);
static void add_sensor_cb(lv_event_t* e);
static void btn_sensor_cb(lv_event_t* e);
static void btn_found_sensor_cb(lv_event_t* e);
static void timer_scan_cb(lv_timer_t* timer);

static void update_sensors();
static void show_found_sensors_selection(int new_sensor_count);

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
		initial_sensor_count = len;
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

static void btn_found_sensor_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	int sensor_idx = (long)lv_event_get_user_data(e);

	if (code == LV_EVENT_CLICKED) {
		/* Sensor selected - load the sensor list and return to main screen */
		controller_get_sensors();
		lv_scr_load(current_main_scr);
		lv_obj_del(selection_scr);
		update_sensors();
	}
}

static void timer_scan_cb(lv_timer_t* timer)
{
	lv_obj_t* scr = timer->user_data;

	controller_get_sensors();
	int len;
	const models_sensor_t* sensors = models_get_sensor(&len);
	int new_sensor_count = len - initial_sensor_count;

	if (scan_retries++ >= MAX_SCAN_RETRIES) {
		/* Scan timeout - show results */
		controller_post_stop_scan();
		lv_timer_del(timer);
		lv_scr_load(scr);
		lv_obj_del(loader_scr);

		if (new_sensor_count > 0) {
			show_found_sensors_selection(new_sensor_count);
		} else {
			tt_obj_info_box_create("New sensor", "No sensor found", 1);
		}
	}

	LV_LOG_USER("Searching sensors. Retries: %d (initial=%d, current=%d, new=%d)",
			scan_retries, initial_sensor_count, len, new_sensor_count);
}

static void show_found_sensors_selection(int new_sensor_count)
{
	int total_len;
	const models_sensor_t* sensors = models_get_sensor(&total_len);

	/* Create selection screen */
	selection_scr = lv_obj_create(NULL);
	lv_obj_set_size(selection_scr, LV_PCT(100), LV_PCT(100));
	lv_obj_set_flex_flow(selection_scr, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_scroll_dir(selection_scr, LV_DIR_VER);

	/* Title */
	lv_obj_t* title = lv_label_create(selection_scr);
	lv_label_set_text(title, "Found Sensors:");
	lv_obj_set_width(title, LV_PCT(100));
	lv_obj_set_height(title, 40);

	/* Create buttons for each newly found sensor */
	int start_idx = total_len - new_sensor_count;
	for (int i = 0; i < new_sensor_count && i < MAX_SENSORS; i++) {
		const models_sensor_t* sensor = &sensors[start_idx + i];
		char btn_text[100];
		snprintf(btn_text, sizeof(btn_text), "MAC: %s", sensor->mac);

		btn_found_sensor[i] = tt_obj_btn_perc_create(selection_scr, NULL, btn_text, 100);
		lv_obj_add_event_cb(btn_found_sensor[i], btn_found_sensor_cb, LV_EVENT_CLICKED,
				(void*)(long)i);
		lv_obj_set_height(btn_found_sensor[i], 50);
	}

	/* Back button */
	lv_obj_t* btn_back = tt_obj_btn_perc_create(selection_scr, NULL, "Cancel", 100);
	lv_obj_set_height(btn_back, 50);
	lv_obj_add_event_cb(btn_back, cancel_loader_cb, LV_EVENT_CLICKED, NULL);

	current_main_scr = lv_scr_act();
	lv_scr_load(selection_scr);
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
			char display_text[100];
			
			// Display sensor name or MAC address
			if (sensors[i].name && strlen(sensors[i].name) > 0) {
				snprintf(display_text, sizeof(display_text), "%s", sensors[i].name);
			} else {
				snprintf(display_text, sizeof(display_text), "Sensor %d", i + 1);
			}
			
			// Get button child label and update it
			lv_obj_t* btn_label = lv_obj_get_child(btn_sensor[i], 0);
			if (btn_label) {
				lv_label_set_text(btn_label, display_text);
			}
			
			// Update MAC label at bottom
			lv_label_set_text(lbl_mac[i], sensors[i].mac);
			
			// Create temperature and humidity display
			char data_text[50];
			
			// Check if sensor has valid data
			if (sensors[i].last_data.datetime && strlen(sensors[i].last_data.datetime) > 0) {
				// Format: "T: 23.5°C H: 45%"
				snprintf(data_text, sizeof(data_text), "T: %.1f°C H: %d%%",
						sensors[i].last_data.temp,
						(int)sensors[i].last_data.humd);
			} else {
				// No data yet
				snprintf(data_text, sizeof(data_text), "No data");
			}
			
			// Create or update data label if it doesn't exist
			// Find or create second label in button for data display
			int child_count = lv_obj_get_child_cnt(btn_sensor[i]);
			lv_obj_t* data_label = NULL;
			
			if (child_count > 1) {
				data_label = lv_obj_get_child(btn_sensor[i], 1);
			} else {
				// Create new label for data
				data_label = lv_label_create(btn_sensor[i]);
				lv_obj_align(data_label, LV_ALIGN_CENTER, 0, 8);
			}
			
			if (data_label) {
				lv_label_set_text(data_label, data_text);
				lv_obj_set_style_text_font(data_label, &lv_font_montserrat_12, 0);
			}
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