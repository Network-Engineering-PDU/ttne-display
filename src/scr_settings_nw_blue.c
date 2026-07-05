#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "scr_settings_nw_blue.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define BT_REFRESH_PERIOD 4000

/* Global variables ***********************************************************/

static lv_obj_t* menu_handle;
static lv_obj_t* page_handle;
static lv_obj_t* btn_powered;
static lv_obj_t* btn_pairable;
static lv_obj_t* btn_discoverable;
static lv_obj_t* btn_scan;
static lv_obj_t* lbl_status;
static lv_obj_t* lbl_no_devices;
static lv_obj_t* btn_device[APP_STATE_MAX_BT_DEVICES];
static lv_obj_t* lbl_device_name[APP_STATE_MAX_BT_DEVICES];
static lv_obj_t* lbl_device_mac[APP_STATE_MAX_BT_DEVICES];
static lv_obj_t* lbl_device_state[APP_STATE_MAX_BT_DEVICES];
static lv_obj_t* pairing_msg_box;
static lv_timer_t* refresh_timer;
static bool page_active;
static bool refresh_pending;
static bool action_pending;
static char action_mac[APP_STATE_BT_TEXT_LEN];
static char action_name[APP_STATE_BT_TEXT_LEN];
static bool action_was_connected;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void settings_toggle_cb(lv_event_t* e);
static void scan_cb(lv_event_t* e);
static void refresh_cb(lv_event_t* e);
static void device_cb(lv_event_t* e);
static void pairing_msg_box_cb(lv_event_t* e);
static void timer_cb(lv_timer_t* timer);
static void update_bluetooth(void);
static void bluetooth_refresh_cb(int err, void* userdata);
static void bluetooth_action_cb(int err, void* userdata);
static void apply_bluetooth_snapshot(void);
static void ensure_timer(bool enable);
static const app_state_bt_device_t* find_device_by_mac(
		const app_state_bt_status_t* bt_status,
		const char* mac);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* target_page = lv_event_get_user_data(e);
		lv_obj_t* active_page = lv_menu_get_cur_main_page(menu);

		if (target_page == active_page) {
			page_active = true;
			update_bluetooth();
			ensure_timer(true);
			LV_LOG_USER("Bluetooth Settings Opened");
		} else {
			page_active = false;
			ensure_timer(false);
		}
	} else if (code == LV_EVENT_DELETE) {
		page_active = false;
		ensure_timer(false);
	}
}

static void settings_toggle_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		bool powered = lv_obj_get_state(btn_powered) & LV_STATE_CHECKED;
		bool pairable = lv_obj_get_state(btn_pairable) & LV_STATE_CHECKED;
		bool discoverable = lv_obj_get_state(btn_discoverable) & LV_STATE_CHECKED;
		backend_bluetooth_set(powered, pairable, discoverable,
				bluetooth_refresh_cb, NULL);
	}
}

static void scan_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		app_state_snapshot_t snapshot;
		app_state_get_snapshot(&snapshot);
		backend_bluetooth_scan(!snapshot.bt_status.discovering,
				bluetooth_refresh_cb, NULL);
	}
}

static void refresh_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		update_bluetooth();
	}
}

static void device_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		int device_idx = (long)lv_event_get_user_data(e);
		app_state_snapshot_t snapshot;
		app_state_get_snapshot(&snapshot);
		const app_state_bt_status_t* bt_status = &snapshot.bt_status;
		if (device_idx < 0 || device_idx >= bt_status->device_count) {
			return;
		}
		if (action_pending) {
			return;
		}
		const app_state_bt_device_t* device = &bt_status->devices[device_idx];
		action_was_connected = device->connected;
		snprintf(action_mac, sizeof(action_mac), "%s", device->mac);
		snprintf(action_name, sizeof(action_name), "%s", device->name);
		if (backend_bluetooth_device_action(action_mac,
				action_was_connected ? "disconnect" : "connect",
				bluetooth_action_cb, NULL) == 0) {
			action_pending = true;
		}
	}
}

static void pairing_msg_box_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* obj = lv_event_get_current_target(e);
		bool accept = lv_msgbox_get_active_btn(obj) == 0;
		backend_bluetooth_pairing_response(accept, bluetooth_refresh_cb, NULL);
		lv_msgbox_close(obj);
		pairing_msg_box = NULL;
	}
}

static void timer_cb(lv_timer_t* timer)
{
	(void)timer;
	update_bluetooth();
	ensure_timer(page_active);
}

/* Function definitions *******************************************************/

static void ensure_timer(bool enable)
{
	if (enable && refresh_timer == NULL) {
		refresh_timer = lv_timer_create(timer_cb, BT_REFRESH_PERIOD, NULL);
	} else if (!enable && refresh_timer != NULL) {
		lv_timer_del(refresh_timer);
		refresh_timer = NULL;
	}
}

static const app_state_bt_device_t* find_device_by_mac(
		const app_state_bt_status_t* bt_status,
		const char* mac)
{
	if (bt_status == NULL || mac == NULL) {
		return NULL;
	}
	for (int i = 0; i < bt_status->device_count; i++) {
		if (strcmp(bt_status->devices[i].mac, mac) == 0) {
			return &bt_status->devices[i];
		}
	}
	return NULL;
}

static void update_bluetooth()
{
	if (refresh_pending) {
		return;
	}
	if (backend_bluetooth_refresh(bluetooth_refresh_cb, NULL) == 0) {
		refresh_pending = true;
	}
}

static void bluetooth_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	refresh_pending = false;
	if (err != 0) {
		return;
	}
	apply_bluetooth_snapshot();
}

static void bluetooth_action_cb(int err, void* userdata)
{
	(void)userdata;
	action_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("Bluetooth", "Bluetooth action failed", 1);
		return;
	}

	apply_bluetooth_snapshot();
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	const app_state_bt_device_t* updated_device =
			find_device_by_mac(&snapshot.bt_status, action_mac);
	bool connected = updated_device != NULL && updated_device->connected;
	bool disconnected = updated_device != NULL && !updated_device->connected;
	char msg[120];
	if (action_was_connected) {
		snprintf(msg, sizeof(msg), "%s %s", action_name,
				disconnected ? "disconnected" : "disconnect failed");
	} else {
		snprintf(msg, sizeof(msg), "%s %s", action_name,
				connected ? "connected" : "connect failed");
	}
	tt_obj_info_box_create("Bluetooth", msg,
			connected || disconnected ? 0 : 1);
}

static void apply_bluetooth_snapshot(void)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	const app_state_bt_status_t* bt_status = &snapshot.bt_status;

	if (!bt_status->valid) {
		return;
	}

	if (bt_status->pairing_request && pairing_msg_box == NULL) {
		char msg[180];
		if (strlen(bt_status->pairing_passkey) > 0) {
			snprintf(msg, sizeof(msg), "%s wants to pair\nPasskey: %s",
					bt_status->pairing_name, bt_status->pairing_passkey);
		} else {
			snprintf(msg, sizeof(msg), "%s wants to pair",
					bt_status->pairing_name);
		}
		pairing_msg_box = tt_obj_msg_box_create("Bluetooth pairing", msg, NULL,
				pairing_msg_box_cb);
	} else if (!bt_status->pairing_request && pairing_msg_box != NULL) {
		lv_msgbox_close(pairing_msg_box);
		pairing_msg_box = NULL;
	}

	tt_obj_btn_toggle_set_state(btn_powered, bt_status->powered);
	tt_obj_btn_toggle_set_state(btn_pairable, bt_status->pairable);
	tt_obj_btn_toggle_set_state(btn_discoverable, bt_status->discoverable);
	tt_obj_btn_set_text(btn_scan, bt_status->discovering ? "Stop scan" : "Scan devices");

	char status[160];
	snprintf(status, sizeof(status), "%s  %s  %s",
			bt_status->powered ? "ON" : "OFF",
			bt_status->discoverable ? "Discoverable" : "Hidden",
			bt_status->pairable ? "Pairable" : "Not pairable");
	lv_label_set_text(lbl_status, status);

	if (bt_status->device_count == 0) {
		lv_obj_clear_flag(lbl_no_devices, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(lbl_no_devices, LV_OBJ_FLAG_HIDDEN);
	}

	for (int i = 0; i < APP_STATE_MAX_BT_DEVICES; i++) {
		if (i < bt_status->device_count) {
			const app_state_bt_device_t* device = &bt_status->devices[i];
			lv_obj_clear_flag(btn_device[i], LV_OBJ_FLAG_HIDDEN);
			lv_label_set_text(lbl_device_name[i], device->name);
			lv_label_set_text(lbl_device_mac[i], device->mac);
			lv_label_set_text(lbl_device_state[i],
					device->connected ? "Connected" :
					(device->paired ? "Paired" : "Available"));
			lv_obj_clear_state(btn_device[i], LV_STATE_CHECKED);
		} else {
			lv_obj_add_flag(btn_device[i], LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_state(btn_device[i], LV_STATE_CHECKED);
		}
	}
}

/* Public functions ***********************************************************/

void scr_settings_nw_blue_create(lv_obj_t* menu_param, lv_obj_t* btn)
{
	menu_handle = menu_param;

	page_handle = tt_obj_menu_page_create(menu_handle, btn, menu_cb, "Bluetooth");
	lv_obj_set_flex_flow(page_handle, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_style_pad_gap(page_handle, 8, 0);

	btn_powered = tt_obj_btn_toggle_create(page_handle, settings_toggle_cb, "Bluetooth");
	btn_pairable = tt_obj_btn_toggle_create(page_handle, settings_toggle_cb, "Pairable");
	btn_discoverable = tt_obj_btn_toggle_create(page_handle, settings_toggle_cb, "Discoverable");

	btn_scan = tt_obj_btn_create(page_handle, scan_cb, "Scan devices", NULL,
			LV_PCT(48), 44, LV_ALIGN_CENTER);
	tt_obj_btn_create(page_handle, refresh_cb, "Refresh", NULL,
			LV_PCT(48), 44, LV_ALIGN_CENTER);

	lbl_status = tt_obj_label_color_create(page_handle, "OFF  Hidden  Not pairable");
	lv_obj_set_width(lbl_status, LV_PCT(100));

	lbl_no_devices = tt_obj_label_color_create(page_handle, "No devices");
	lv_obj_set_width(lbl_no_devices, LV_PCT(100));

	for (int i = 0; i < APP_STATE_MAX_BT_DEVICES; i++) {
		btn_device[i] = tt_obj_btn_create(page_handle, NULL, "", NULL,
				LV_PCT(100), 68, LV_ALIGN_CENTER);
		lv_obj_add_flag(btn_device[i], LV_OBJ_FLAG_CHECKABLE);
		lv_obj_add_style(btn_device[i], &btn_press_style, LV_STATE_CHECKED);
		lv_obj_add_event_cb(btn_device[i], device_cb, LV_EVENT_ALL,
				(void*)(long)i);
		lv_obj_set_scroll_dir(btn_device[i], LV_DIR_NONE);

		lbl_device_name[i] = lv_label_create(btn_device[i]);
		lv_label_set_long_mode(lbl_device_name[i], LV_LABEL_LONG_SCROLL);
		lv_obj_set_width(lbl_device_name[i], LV_PCT(55));
		lv_obj_align(lbl_device_name[i], LV_ALIGN_TOP_LEFT, 10, 6);

		lbl_device_mac[i] = lv_label_create(btn_device[i]);
		lv_obj_align(lbl_device_mac[i], LV_ALIGN_BOTTOM_LEFT, 10, -6);

		lbl_device_state[i] = lv_label_create(btn_device[i]);
		lv_obj_align(lbl_device_state[i], LV_ALIGN_RIGHT_MID, -10, 0);
		lv_obj_add_flag(btn_device[i], LV_OBJ_FLAG_HIDDEN);
	}

}
