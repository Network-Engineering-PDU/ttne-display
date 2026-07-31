#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "scr_settings_nw_ntp_sntp.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define NTP_OFFSET_MIN (-12)
#define NTP_OFFSET_MAX 12
#define NTP_LABEL_WIDTH 38
#define NTP_CONTROL_WIDTH 58

static const char* OFFSET_OPTIONS =
		"UTC-12\nUTC-11\nUTC-10\nUTC-09\nUTC-08\nUTC-07\nUTC-06\n"
		"UTC-05\nUTC-04\nUTC-03\nUTC-02\nUTC-01\nUTC+00\nUTC+01\n"
		"UTC+02\nUTC+03\nUTC+04\nUTC+05\nUTC+06\nUTC+07\nUTC+08\n"
		"UTC+09\nUTC+10\nUTC+11\nUTC+12";

static lv_obj_t* menu_handle;
static lv_obj_t* page_handle;
static lv_obj_t* cbx_enabled;
static lv_obj_t* dd_offset;
static lv_obj_t* txt_server;
static bool refresh_pending;
static bool save_pending;

static void refresh_cb(int err, void* userdata);
static void save_cb(int err, void* userdata);

static lv_obj_t* create_row(lv_obj_t* parent, const char* label_text,
		int label_width)
{
	lv_obj_t* row = lv_obj_create(parent);
	lv_obj_set_size(row, LV_PCT(100), 38);
	lv_obj_add_style(row, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* label = lv_label_create(row);
	lv_label_set_text(label, label_text);
	lv_obj_set_width(label, LV_PCT(label_width));
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
	return row;
}

static void apply_snapshot(void)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	if (!snapshot.ntp.valid) {
		return;
	}
	if (snapshot.ntp.enabled) {
		lv_obj_add_state(cbx_enabled, LV_STATE_CHECKED);
	} else {
		lv_obj_clear_state(cbx_enabled, LV_STATE_CHECKED);
	}
	int offset = snapshot.ntp.time_offset;
	if (offset < NTP_OFFSET_MIN || offset > NTP_OFFSET_MAX) {
		offset = 0;
	}
	lv_dropdown_set_selected(dd_offset, (uint16_t)(offset - NTP_OFFSET_MIN));
	lv_textarea_set_text(txt_server, snapshot.ntp.server);
}

static void request_refresh(void)
{
	if (refresh_pending) {
		return;
	}
	refresh_pending = true;
	if (backend_ntp_refresh(refresh_cb, NULL) != 0) {
		refresh_pending = false;
	}
}

static void menu_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	lv_obj_t* target_page = lv_event_get_user_data(e);
	lv_obj_t* active_page = lv_menu_get_cur_main_page(
			lv_event_get_current_target(e));
	if (target_page == active_page) {
		request_refresh();
	}
}

static void server_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		lv_obj_t* keyboard = scr_keyboard_create(
				lv_scr_act(), lv_event_get_target(e), KB_ABC);
		lv_scr_load(keyboard);
	}
}

static void ok_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || save_pending) {
		return;
	}
	const char* server = lv_textarea_get_text(txt_server);
	if (server == NULL || server[0] == '\0') {
		tt_obj_info_box_create("NTP / SNTP",
				"Server address can not be empty", 1);
		return;
	}
	bool enabled = (lv_obj_get_state(cbx_enabled) & LV_STATE_CHECKED) != 0;
	int offset = (int)lv_dropdown_get_selected(dd_offset) + NTP_OFFSET_MIN;
	save_pending = true;
	if (backend_ntp_save(enabled, offset, server, save_cb, NULL) != 0) {
		save_pending = false;
		tt_obj_info_box_create("NTP / SNTP",
				"Can not apply NTP settings", 1);
	}
}

static void cancel_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || save_pending) {
		return;
	}
	apply_snapshot();
	lv_event_send(lv_menu_get_main_header_back_btn(menu_handle),
			LV_EVENT_CLICKED, NULL);
}

static void refresh_cb(int err, void* userdata)
{
	(void)userdata;
	refresh_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("NTP / SNTP",
				"Can not load NTP settings", 1);
		return;
	}
	apply_snapshot();
}

static void save_cb(int err, void* userdata)
{
	(void)userdata;
	save_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("NTP / SNTP",
				"Can not apply NTP settings", 1);
		return;
	}
	apply_snapshot();
	tt_obj_info_box_create("NTP / SNTP",
			"Settings saved. Time synchronization may take a few moments.", 0);
}

void scr_settings_nw_ntp_sntp_create(lv_obj_t* menu, lv_obj_t* btn)
{
	menu_handle = menu;
	page_handle = tt_obj_menu_page_create(menu, btn, menu_cb, "NTP - SNTP");
	lv_obj_t* main = tt_obj_cont_create(page_handle);
	lv_obj_set_width(main, LV_PCT(100));
	lv_obj_set_height(main, 0);
	lv_obj_set_flex_grow(main, 1);
	lv_obj_clear_flag(main, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(main, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(main, 4, 0);
	lv_obj_set_style_pad_row(main, 2, 0);

	lv_obj_t* enabled_row = create_row(main, "NTP / SNTP enable", 70);
	cbx_enabled = tt_obj_checkbox_create(enabled_row, "", NULL);
	lv_obj_set_size(cbx_enabled, 34, 34);
	lv_obj_set_style_border_width(cbx_enabled, 2, LV_PART_INDICATOR);

	lv_obj_t* offset_row = create_row(main, "Time offset", NTP_LABEL_WIDTH);
	lv_obj_t* offset_label = lv_obj_get_child(offset_row, 0);
	lv_label_set_long_mode(offset_label, LV_LABEL_LONG_SCROLL);
	lv_obj_set_height(offset_label, LV_SIZE_CONTENT);
	dd_offset = tt_obj_dropdown_create(offset_row, (char*)OFFSET_OPTIONS, NULL);
	lv_obj_set_size(dd_offset, LV_PCT(NTP_CONTROL_WIDTH), 34);
	lv_dropdown_set_selected(dd_offset, 12);

	lv_obj_t* server_row = create_row(main, "Server", NTP_LABEL_WIDTH);
	txt_server = tt_obj_txt_create(server_row, "IP / DNS", server_cb);
	lv_obj_set_size(txt_server, LV_PCT(NTP_CONTROL_WIDTH), 34);
	lv_textarea_set_max_length(txt_server, APP_STATE_NW_TEXT_LEN - 1);
	lv_textarea_set_accepted_chars(txt_server,
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-:");

	lv_obj_t* spacer = lv_obj_create(main);
	lv_obj_set_size(spacer, LV_PCT(100), 0);
	lv_obj_set_flex_grow(spacer, 1);
	lv_obj_add_style(spacer, &invisible_cont_style, LV_STATE_DEFAULT);

	lv_obj_t* button_row = lv_obj_create(main);
	lv_obj_set_size(button_row, LV_PCT(100), 52);
	lv_obj_add_style(button_row, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	tt_obj_btn_perc_create(button_row, ok_cb, "OK", 47);
	tt_obj_btn_perc_create(button_row, cancel_cb, "Cancel", 47);

	apply_snapshot();
}
