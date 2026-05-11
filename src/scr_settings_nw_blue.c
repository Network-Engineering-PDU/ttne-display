#include <stdio.h>

#include "lvgl/lvgl.h"
#include "scr_settings_nw_blue.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "models.h"
#include "controller.h"

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
static lv_obj_t* btn_device[MAX_BT_DEVICES];
static lv_obj_t* lbl_device_name[MAX_BT_DEVICES];
static lv_obj_t* lbl_device_mac[MAX_BT_DEVICES];
static lv_obj_t* lbl_device_state[MAX_BT_DEVICES];
static lv_obj_t* btn_pair;
static lv_obj_t* btn_refuse;
static lv_obj_t* btn_connect;
static lv_timer_t* refresh_timer;
static int selected_device = -1;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void settings_toggle_cb(lv_event_t* e);
static void scan_cb(lv_event_t* e);
static void refresh_cb(lv_event_t* e);
static void device_cb(lv_event_t* e);
static void pair_cb(lv_event_t* e);
static void refuse_cb(lv_event_t* e);
static void connect_cb(lv_event_t* e);
static void timer_cb(lv_timer_t* timer);
static void update_bluetooth();
static void update_device_actions(const models_bt_status_t* bt_status);
static void ensure_timer(bool enable);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* target_page = lv_event_get_user_data(e);
		lv_obj_t* active_page = lv_menu_get_cur_main_page(menu);

		if (target_page == active_page) {
			selected_device = -1;
			update_bluetooth();
			const models_bt_status_t* bt_status = models_get_bt_status();
			ensure_timer(bt_status->discovering);
			LV_LOG_USER("Bluetooth Settings Opened");
		} else {
			ensure_timer(false);
		}
	} else if (code == LV_EVENT_DELETE) {
		ensure_timer(false);
	}
}

static void settings_toggle_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		bool powered = lv_obj_get_state(btn_powered) & LV_STATE_CHECKED;
		bool pairable = lv_obj_get_state(btn_pairable) & LV_STATE_CHECKED;
		bool discoverable = lv_obj_get_state(btn_discoverable) & LV_STATE_CHECKED;
		controller_put_bluetooth(powered, pairable, discoverable);
		update_bluetooth();
	}
}

static void scan_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		const models_bt_status_t* bt_status = models_get_bt_status();
		controller_post_bluetooth_scan(!bt_status->discovering);
		update_bluetooth();
		bt_status = models_get_bt_status();
		ensure_timer(bt_status->discovering);
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
		selected_device = (long)lv_event_get_user_data(e);
		update_bluetooth();
	}
}

static void pair_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED && selected_device >= 0) {
		const models_bt_status_t* bt_status = models_get_bt_status();
		const models_bt_device_t* device = &bt_status->devices[selected_device];
		controller_post_bluetooth_device_action(device->mac, "pair");
		controller_post_bluetooth_device_action(device->mac, "trust");
		update_bluetooth();
	}
}

static void refuse_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED && selected_device >= 0) {
		const models_bt_status_t* bt_status = models_get_bt_status();
		const models_bt_device_t* device = &bt_status->devices[selected_device];
		controller_post_bluetooth_device_action(device->mac, "cancel-pairing");
		controller_post_bluetooth_device_action(device->mac, "remove");
		selected_device = -1;
		update_bluetooth();
	}
}

static void connect_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED && selected_device >= 0) {
		const models_bt_status_t* bt_status = models_get_bt_status();
		const models_bt_device_t* device = &bt_status->devices[selected_device];
		controller_post_bluetooth_device_action(device->mac,
				device->connected ? "disconnect" : "connect");
		update_bluetooth();
	}
}

static void timer_cb(lv_timer_t* timer)
{
	(void)timer;
	update_bluetooth();
	const models_bt_status_t* bt_status = models_get_bt_status();
	ensure_timer(bt_status->discovering);
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

static void update_device_actions(const models_bt_status_t* bt_status)
{
	bool has_selection = selected_device >= 0 &&
			selected_device < bt_status->device_count;

	if (!has_selection) {
		lv_obj_add_state(btn_pair, LV_STATE_DISABLED);
		lv_obj_add_state(btn_refuse, LV_STATE_DISABLED);
		lv_obj_add_state(btn_connect, LV_STATE_DISABLED);
		tt_obj_btn_set_text(btn_pair, "Pair");
		tt_obj_btn_set_text(btn_refuse, "Refuse");
		tt_obj_btn_set_text(btn_connect, "Connect");
		return;
	}

	const models_bt_device_t* device = &bt_status->devices[selected_device];
	lv_obj_clear_state(btn_pair, LV_STATE_DISABLED);
	lv_obj_clear_state(btn_refuse, LV_STATE_DISABLED);
	lv_obj_clear_state(btn_connect, LV_STATE_DISABLED);
	tt_obj_btn_set_text(btn_pair, device->paired ? "Trust" : "Pair");
	tt_obj_btn_set_text(btn_refuse, device->paired ? "Remove" : "Refuse");
	tt_obj_btn_set_text(btn_connect, device->connected ? "Disconnect" : "Connect");
}

static void update_bluetooth()
{
	controller_get_bluetooth();
	const models_bt_status_t* bt_status = models_get_bt_status();

	tt_obj_btn_toggle_set_state(btn_powered, bt_status->powered);
	tt_obj_btn_toggle_set_state(btn_pairable, bt_status->pairable);
	tt_obj_btn_toggle_set_state(btn_discoverable, bt_status->discoverable);
	tt_obj_btn_set_text(btn_scan, bt_status->discovering ? "Stop scan" : "Scan devices");

	char status[160];
	sprintf(status, "%s  %s  %s", bt_status->powered ? "ON" : "OFF",
			bt_status->discoverable ? "Discoverable" : "Hidden",
			bt_status->pairable ? "Pairable" : "Not pairable");
	lv_label_set_text(lbl_status, status);

	if (bt_status->device_count == 0) {
		lv_obj_clear_flag(lbl_no_devices, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(lbl_no_devices, LV_OBJ_FLAG_HIDDEN);
	}

	for (int i = 0; i < MAX_BT_DEVICES; i++) {
		if (i < bt_status->device_count) {
			const models_bt_device_t* device = &bt_status->devices[i];
			lv_obj_clear_flag(btn_device[i], LV_OBJ_FLAG_HIDDEN);
			lv_label_set_text(lbl_device_name[i], device->name);
			lv_label_set_text(lbl_device_mac[i], device->mac);
			lv_label_set_text(lbl_device_state[i],
					device->connected ? "Connected" :
					(device->paired ? "Paired" : "Available"));
			if (i == selected_device) {
				lv_obj_add_state(btn_device[i], LV_STATE_CHECKED);
			} else {
				lv_obj_clear_state(btn_device[i], LV_STATE_CHECKED);
			}
		} else {
			lv_obj_add_flag(btn_device[i], LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_state(btn_device[i], LV_STATE_CHECKED);
		}
	}

	update_device_actions(bt_status);
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

	for (int i = 0; i < MAX_BT_DEVICES; i++) {
		btn_device[i] = tt_obj_btn_create(page_handle, NULL, "", NULL,
				LV_PCT(100), 68, LV_ALIGN_CENTER);
		lv_obj_add_flag(btn_device[i], LV_OBJ_FLAG_CHECKABLE);
		lv_obj_add_style(btn_device[i], &btn_press_style, LV_STATE_CHECKED);
		lv_obj_add_event_cb(btn_device[i], device_cb, LV_EVENT_ALL,
				(void*)(long)i);
		lv_obj_set_scroll_dir(btn_device[i], LV_DIR_NONE);

		lbl_device_name[i] = lv_label_create(btn_device[i]);
		lv_label_set_long_mode(lbl_device_name[i], LV_LABEL_LONG_SCROLL_CIRCULAR);
		lv_obj_set_width(lbl_device_name[i], LV_PCT(55));
		lv_obj_align(lbl_device_name[i], LV_ALIGN_TOP_LEFT, 10, 6);

		lbl_device_mac[i] = lv_label_create(btn_device[i]);
		lv_obj_align(lbl_device_mac[i], LV_ALIGN_BOTTOM_LEFT, 10, -6);

		lbl_device_state[i] = lv_label_create(btn_device[i]);
		lv_obj_align(lbl_device_state[i], LV_ALIGN_RIGHT_MID, -10, 0);
		lv_obj_add_flag(btn_device[i], LV_OBJ_FLAG_HIDDEN);
	}

	btn_pair = tt_obj_btn_create(page_handle, pair_cb, "Pair", NULL,
			LV_PCT(31), 44, LV_ALIGN_CENTER);
	btn_refuse = tt_obj_btn_create(page_handle, refuse_cb, "Refuse", NULL,
			LV_PCT(31), 44, LV_ALIGN_CENTER);
	btn_connect = tt_obj_btn_create(page_handle, connect_cb, "Connect", NULL,
			LV_PCT(31), 44, LV_ALIGN_CENTER);
	lv_obj_add_state(btn_pair, LV_STATE_DISABLED);
	lv_obj_add_state(btn_refuse, LV_STATE_DISABLED);
	lv_obj_add_state(btn_connect, LV_STATE_DISABLED);
}
