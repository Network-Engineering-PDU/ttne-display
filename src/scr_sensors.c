#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "scr_sensors.h"
#include "scr_sensors_data.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define TIMER_SCAN 2000 // ms
#define MIN_SCAN_POLLS 3
#define MAX_SCAN_RETRIES 30 // 30 * 2s = 60 seconds scan timeout

#ifdef UI_POLL_DEBUG_LOGS
#define UI_POLL_LOG(...) LV_LOG_USER(__VA_ARGS__)
#else
#define UI_POLL_LOG(...) ((void)0)
#endif


/* Global variables ***********************************************************/

static lv_obj_t* menu;

static lv_obj_t* loader_scr;
static lv_obj_t* selection_scr;

static lv_timer_t* timer;

static lv_obj_t* btn_sensor[APP_STATE_MAX_SENSORS];
static lv_obj_t* lbl_mac[APP_STATE_MAX_SENSORS];
static lv_obj_t* btn_found_sensor[APP_STATE_MAX_SENSORS];

static lv_obj_t* sensors_data_page;
static lv_obj_t* lbl_no_sensors;

static int scan_retries;
static int discovered_count;
static lv_obj_t* current_main_scr;
static bool sensors_refresh_pending;
static bool scan_start_pending;
static bool discovered_refresh_pending;
static char selected_discovered_mac[APP_STATE_SENSOR_TEXT_LEN];

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void cancel_loader_cb(lv_event_t* e);
static void add_sensor_cb(lv_event_t* e);
static void btn_sensor_cb(lv_event_t* e);
static void btn_found_sensor_cb(lv_event_t* e);
static void btn_add_all_cb(lv_event_t* e);
static void timer_scan_cb(lv_timer_t* timer);
static void sensors_refresh_cb(int err, void* userdata);
static void scan_start_cb(int err, void* userdata);
static void discovered_refresh_cb(int err, void* userdata);
static void confirm_sensor_cb(int err, void* userdata);

static void update_sensors(void);
static void apply_sensors_snapshot(void);
static void finish_scan_and_show(void);
static void show_found_sensors_selection(void);
static bool open_sensor_by_mac_from_snapshot(const char* mac);

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
		if (selection_scr != NULL) {
			backend_ble_scan_stop(NULL, NULL);
			lv_scr_load(current_main_scr);
			lv_obj_del(selection_scr);
			selection_scr = NULL;
			return;
		}
		finish_scan_and_show();
	}
}


static void add_sensor_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		app_state_snapshot_t snapshot;
		app_state_get_snapshot(&snapshot);
		if (snapshot.sensor_count == APP_STATE_MAX_SENSORS) {
			tt_obj_info_box_create("ERROR", "Max number of sensor reached", 1);
			return;
		}
		discovered_count = 0;
		scan_retries = 0;
		if (scan_start_pending) {
			return;
		}
		if (backend_ble_scan_start(scan_start_cb, lv_scr_act()) == 0) {
			scan_start_pending = true;
		}
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

static void finish_selection_and_return(void)
{
	backend_ble_scan_stop(NULL, NULL);
	if (selection_scr != NULL) {
		lv_scr_load(current_main_scr);
		lv_obj_del(selection_scr);
		selection_scr = NULL;
	} else if (current_main_scr != NULL) {
		lv_scr_load(current_main_scr);
	}
	update_sensors();
}

static bool open_sensor_by_mac_from_snapshot(const char* mac)
{
	if (mac == NULL || mac[0] == '\0') {
		return false;
	}

	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	for (int i = 0; i < snapshot.sensor_count; i++) {
		if (strcmp(snapshot.sensors[i].mac, mac) == 0) {
			scr_sensors_data_set_sensor(i + 1);
			lv_menu_set_page(menu, sensors_data_page);
			return true;
		}
	}
	return false;
}

static void btn_found_sensor_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	int sensor_idx = (long)lv_event_get_user_data(e);

	if (code == LV_EVENT_CLICKED) {
		app_state_snapshot_t snapshot;
		app_state_get_snapshot(&snapshot);
		if (sensor_idx >= 0 && sensor_idx < snapshot.discovered_sensor_count) {
			snprintf(selected_discovered_mac, sizeof(selected_discovered_mac), "%s",
					snapshot.discovered_sensors[sensor_idx].mac);
			backend_ble_confirm_mac(selected_discovered_mac, confirm_sensor_cb,
					(void*)1);
			return;
		}
		finish_selection_and_return();
	}
}

static void btn_add_all_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		selected_discovered_mac[0] = '\0';
		backend_ble_confirm_all(confirm_sensor_cb, NULL);
	}
}

static void finish_scan_and_show(void)
{
	lv_obj_t* scr = NULL;

	if (timer != NULL) {
		scr = (lv_obj_t*)timer->user_data;
		lv_timer_del(timer);
		timer = NULL;
	}
	backend_ble_scan_stop(NULL, NULL);
	if (loader_scr != NULL && lv_scr_act() == loader_scr) {
		lv_scr_load(scr);
		lv_obj_del(loader_scr);
		loader_scr = NULL;
	}

	if (discovered_count > 0) {
		show_found_sensors_selection();
	} else {
		tt_obj_info_box_create("New sensor", "No BLE sensor found", 1);
	}
}

static void timer_scan_cb(lv_timer_t* timer)
{
	(void)timer;
	if (discovered_refresh_pending) {
		return;
	}
	if (backend_ble_discovered_refresh(discovered_refresh_cb, NULL) == 0) {
		discovered_refresh_pending = true;
	}
}

static void discovered_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	discovered_refresh_pending = false;
	if (err != 0) {
		return;
	}

	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	discovered_count = snapshot.discovered_sensor_count;
	scan_retries++;

	UI_POLL_LOG("Searching BLE sensors. Retries: %d (found=%d)",
			scan_retries, discovered_count);

	if (discovered_count > 0 && scan_retries >= MIN_SCAN_POLLS) {
		finish_scan_and_show();
		return;
	}

	if (scan_retries >= MAX_SCAN_RETRIES) {
		finish_scan_and_show();
	}
}

static void show_found_sensors_selection(void)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	int len = snapshot.discovered_sensor_count;

	selection_scr = lv_obj_create(NULL);
	lv_obj_set_size(selection_scr, LV_PCT(100), LV_PCT(100));
	lv_obj_set_flex_flow(selection_scr, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_scroll_dir(selection_scr, LV_DIR_VER);

	lv_obj_t* title = lv_label_create(selection_scr);
	lv_label_set_text(title, "Nearest sensors (strongest signal):");
	lv_obj_set_width(title, LV_PCT(100));
	lv_obj_set_height(title, 40);

	if (len > 0) {
		lv_obj_t* btn_all = tt_obj_btn_perc_create(selection_scr, btn_add_all_cb,
				"ADD ALL", 100);
		lv_obj_set_height(btn_all, 50);
	}

	for (int i = 0; i < len && i < APP_STATE_MAX_SENSORS; i++) {
		char btn_text[180];
		const app_state_discovered_sensor_t* device =
				&snapshot.discovered_sensors[i];
		const char* label = device->name[0] != '\0' ?
				device->name : device->kind;
		snprintf(btn_text, sizeof(btn_text), "#%d %s %s (%d dBm)",
				i + 1, label, device->mac, device->rssi);

		btn_found_sensor[i] = tt_obj_btn_perc_create(selection_scr, NULL,
				btn_text, 100);
		lv_obj_add_event_cb(btn_found_sensor[i], btn_found_sensor_cb,
				LV_EVENT_CLICKED, (void*)(long)i);
		lv_obj_set_height(btn_found_sensor[i], 50);
	}

	lv_obj_t* btn_back = tt_obj_btn_perc_create(selection_scr, NULL, "Cancel", 100);
	lv_obj_set_height(btn_back, 50);
	lv_obj_add_event_cb(btn_back, cancel_loader_cb, LV_EVENT_CLICKED, NULL);

	current_main_scr = lv_scr_act();
	lv_scr_load(selection_scr);
}

/* Function definitions *******************************************************/

static void update_sensors(void)
{
	if (sensors_refresh_pending) {
		return;
	}
	if (backend_sensors_refresh(sensors_refresh_cb, NULL) == 0) {
		sensors_refresh_pending = true;
	}
}

static void sensors_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	sensors_refresh_pending = false;
	if (err != 0) {
		return;
	}
	apply_sensors_snapshot();
}

static void apply_sensors_snapshot(void)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	int len = snapshot.sensor_count;

	if (len == 0) {
		lv_obj_clear_flag(lbl_no_sensors, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(lbl_no_sensors, LV_OBJ_FLAG_HIDDEN);
	}
	for (int i = 0; i < APP_STATE_MAX_SENSORS; i++) {
		if (i < len) {
			char label[APP_STATE_SENSOR_TEXT_LEN];
			const app_state_sensor_t* sensor = &snapshot.sensors[i];
			lv_obj_clear_flag(btn_sensor[i], LV_OBJ_FLAG_HIDDEN);
			if (sensor->name[0] != '\0') {
				snprintf(label, sizeof(label), "%s", sensor->name);
			} else {
				snprintf(label, sizeof(label), "SENSOR %d", i + 1);
			}
			lv_label_set_text(lv_obj_get_child(btn_sensor[i], 0), label);
			lv_label_set_text(lbl_mac[i], sensor->mac);
		} else {
			lv_obj_add_flag(btn_sensor[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static void scan_start_cb(int err, void* userdata)
{
	lv_obj_t* previous = userdata;
	scan_start_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("ERROR",
				"Could not start BLE scan.\nCheck Bluetooth service.", 1);
		return;
	}
	timer = lv_timer_create(timer_scan_cb, TIMER_SCAN, previous);
	loader_scr = tt_obj_loader_create("Searching nearest sensors...",
			cancel_loader_cb);
	lv_scr_load(loader_scr);
}

static void confirm_sensor_cb(int err, void* userdata)
{
	bool open_after_add = userdata != NULL;
	if (err != 0) {
		tt_obj_info_box_create("ERROR", "Could not register sensor", 1);
		finish_selection_and_return();
		return;
	}

	finish_selection_and_return();
	if (open_after_add && !open_sensor_by_mac_from_snapshot(selected_discovered_mac)) {
		tt_obj_info_box_create("Sensor added",
				"Sensor registered.\nOpen it to view live data.", 1);
	} else if (!open_after_add) {
		tt_obj_info_box_create("Sensors added",
				"Sensors registered.\nOpen one to view live data.", 1);
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

	for (int i = 0; i < APP_STATE_MAX_SENSORS; i++) {
		char str[10];
		snprintf(str, sizeof(str), "SENSOR %d", i + 1);
		btn_sensor[i] = tt_obj_btn_perc_create(sensors_cont, NULL, str, 48);
		lv_obj_add_event_cb(btn_sensor[i], btn_sensor_cb, LV_EVENT_ALL,
				(void*)(long)(i + 1));
		lv_obj_t* lbl_name = lv_obj_get_child(btn_sensor[i], 0);
		lv_obj_align(lbl_name, LV_ALIGN_TOP_MID, 0, 0);
		lbl_mac[i] = lv_label_create(btn_sensor[i]);
		lv_label_set_long_mode(lbl_mac[i], LV_LABEL_LONG_SCROLL_CIRCULAR);
		lv_obj_set_style_text_align(lbl_mac[i], LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_width(lbl_mac[i], LV_PCT(90));
		lv_obj_align(lbl_mac[i], LV_ALIGN_BOTTOM_MID, 0, 0);
		lv_obj_set_scrollbar_mode(btn_sensor[i], LV_SCROLLBAR_MODE_OFF);
	}

	update_sensors();
}
