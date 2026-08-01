#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "scr_settings_nw_snmp.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define SNMP_ROW_HEIGHT 36
#define SNMP_COMMUNITY_MAX 64

static lv_obj_t* menu_handle;
static lv_obj_t* cbx_enabled;
static lv_obj_t* cbx_set_enabled;
static lv_obj_t* cbx_traps_enabled;
static lv_obj_t* dd_version;
static lv_obj_t* txt_community;
static lv_obj_t* txt_managers[4];
static bool refresh_pending;
static bool save_pending;

static void refresh_cb(int err, void* userdata);
static void save_cb(int err, void* userdata);

static lv_obj_t* create_row(lv_obj_t* parent, int height)
{
	lv_obj_t* row = lv_obj_create(parent);
	lv_obj_set_size(row, LV_PCT(100), height);
	lv_obj_add_style(row, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(row, 0, 0);
	lv_obj_set_style_pad_column(row, 3, 0);
	return row;
}

static lv_obj_t* create_field_group(lv_obj_t* parent, const char* label_text,
		int width, int label_width)
{
	lv_obj_t* group = lv_obj_create(parent);
	lv_obj_set_size(group, LV_PCT(width), LV_PCT(100));
	lv_obj_add_style(group, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(group, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(group, LV_FLEX_ALIGN_SPACE_BETWEEN,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(group, 0, 0);
	lv_obj_set_style_pad_column(group, 2, 0);
	lv_obj_t* label = lv_label_create(group);
	lv_label_set_text(label, label_text);
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
	lv_obj_set_width(label, LV_PCT(label_width));
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
	return group;
}

static lv_obj_t* create_checkbox(lv_obj_t* parent)
{
	lv_obj_t* checkbox = tt_obj_checkbox_create(parent, "", NULL);
	lv_obj_set_size(checkbox, 32, 32);
	lv_obj_set_style_border_width(checkbox, 2, LV_PART_INDICATOR);
	return checkbox;
}

static void set_checked(lv_obj_t* checkbox, bool checked)
{
	if (checked) {
		lv_obj_add_state(checkbox, LV_STATE_CHECKED);
	} else {
		lv_obj_clear_state(checkbox, LV_STATE_CHECKED);
	}
}

static bool is_checked(lv_obj_t* checkbox)
{
	return (lv_obj_get_state(checkbox) & LV_STATE_CHECKED) != 0;
}

static void apply_snapshot(void)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	if (!snapshot.snmp.valid) {
		return;
	}
	set_checked(cbx_enabled, snapshot.snmp.enabled);
	set_checked(cbx_set_enabled, snapshot.snmp.set_enabled);
	set_checked(cbx_traps_enabled, snapshot.snmp.traps_enabled);
	lv_dropdown_set_selected(dd_version, 0);
	lv_textarea_set_text(txt_community, snapshot.snmp.community);
	for (int i = 0; i < 4; i++) {
		lv_textarea_set_text(txt_managers[i], snapshot.snmp.managers[i]);
	}
}

static void request_refresh(void)
{
	if (refresh_pending || save_pending) {
		return;
	}
	refresh_pending = true;
	if (backend_snmp_refresh(refresh_cb, NULL) != 0) {
		refresh_pending = false;
		tt_obj_info_box_create("SNMP", "Can not load SNMP settings", 1);
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

static void text_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		lv_obj_t* keyboard = scr_keyboard_create(
				lv_scr_act(), lv_event_get_target(e), KB_ABC);
		lv_scr_load(keyboard);
	}
}

static void refresh_button_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		request_refresh();
	}
}

static bool valid_community(const char* value)
{
	if (value == NULL || value[0] == '\0' ||
			strlen(value) > SNMP_COMMUNITY_MAX) {
		return false;
	}
	for (const char* ptr = value; *ptr != '\0'; ptr++) {
		if (!isalnum((unsigned char)*ptr) && *ptr != '_' &&
				*ptr != '.' && *ptr != '-') {
			return false;
		}
	}
	return true;
}

static bool valid_manager(const char* value)
{
	if (value == NULL || value[0] == '\0') {
		return true;
	}
	size_t length = strlen(value);
	if (length >= APP_STATE_NW_TEXT_LEN) {
		return false;
	}
	struct in_addr address;
	if (inet_pton(AF_INET, value, &address) == 1) {
		return true;
	}
	bool numeric_dotted = true;
	size_t label_length = 0;
	for (const char* ptr = value; *ptr != '\0'; ptr++) {
		if (*ptr == '.') {
			if (label_length == 0 || label_length > 63 ||
					!isalnum((unsigned char)*(ptr - 1))) {
				return false;
			}
			label_length = 0;
			continue;
		}
		if (label_length == 0 && !isalnum((unsigned char)*ptr)) {
			return false;
		}
		if (!isalnum((unsigned char)*ptr) && *ptr != '-') {
			return false;
		}
		numeric_dotted = numeric_dotted && isdigit((unsigned char)*ptr);
		label_length++;
	}
	return !numeric_dotted && label_length > 0 && label_length <= 63 &&
			isalnum((unsigned char)value[length - 1]);
}

static void ok_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || save_pending) {
		return;
	}
	const char* community = lv_textarea_get_text(txt_community);
	if (!valid_community(community)) {
		tt_obj_info_box_create("SNMP",
				"Community must use 1-64 letters, numbers, '.', '-' or '_'", 1);
		return;
	}

	app_state_snmp_t snmp;
	memset(&snmp, 0, sizeof(snmp));
	snmp.enabled = is_checked(cbx_enabled);
	snmp.set_enabled = is_checked(cbx_set_enabled);
	snmp.traps_enabled = is_checked(cbx_traps_enabled);
	snprintf(snmp.community, sizeof(snmp.community), "%s", community);
	bool has_manager = false;
	for (int i = 0; i < 4; i++) {
		const char* manager = lv_textarea_get_text(txt_managers[i]);
		if (!valid_manager(manager)) {
			tt_obj_info_box_create("SNMP",
					"Trap target must be a valid IPv4 address or DNS name", 1);
			return;
		}
		snprintf(snmp.managers[i], sizeof(snmp.managers[i]), "%s", manager);
		has_manager = has_manager || manager[0] != '\0';
	}
	if (snmp.traps_enabled && !has_manager) {
		tt_obj_info_box_create("SNMP",
				"Enter at least one trap IP or DNS name", 1);
		return;
	}

	save_pending = true;
	if (backend_snmp_save(&snmp, save_cb, NULL) != 0) {
		save_pending = false;
		tt_obj_info_box_create("SNMP", "Can not apply SNMP settings", 1);
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
		tt_obj_info_box_create("SNMP", "Can not load SNMP settings", 1);
		return;
	}
	apply_snapshot();
}

static void save_cb(int err, void* userdata)
{
	(void)userdata;
	save_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("SNMP", "Can not apply SNMP settings", 1);
		return;
	}
	apply_snapshot();
	tt_obj_info_box_create("SNMP", "SNMP settings applied", 0);
}

void scr_settings_nw_snmp_create(lv_obj_t* menu, lv_obj_t* btn)
{
	menu_handle = menu;
	lv_obj_t* page = tt_obj_menu_page_create(menu, btn, menu_cb, "SNMP");
	lv_obj_t* main = tt_obj_cont_create(page);
	lv_obj_set_width(main, LV_PCT(100));
	lv_obj_set_height(main, 0);
	lv_obj_set_flex_grow(main, 1);
	lv_obj_clear_flag(main, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(main, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(main, 3, 0);
	lv_obj_set_style_pad_row(main, 1, 0);

	lv_obj_t* row = create_row(main, SNMP_ROW_HEIGHT);
	lv_obj_t* group = create_field_group(row, "SNMP enable", 45, 58);
	cbx_enabled = create_checkbox(group);
	group = create_field_group(row, "Version", 53, 38);
	dd_version = tt_obj_dropdown_create(group, "V1 / V2c", NULL);
	lv_obj_set_size(dd_version, LV_PCT(60), 32);

	row = create_row(main, SNMP_ROW_HEIGHT);
	group = create_field_group(row, "SET enable", 45, 58);
	cbx_set_enabled = create_checkbox(group);
	group = create_field_group(row, "Community", 53, 38);
	txt_community = tt_obj_txt_create(group, "Community", text_cb);
	lv_obj_set_size(txt_community, LV_PCT(60), 32);
	lv_textarea_set_max_length(txt_community, SNMP_COMMUNITY_MAX);
	lv_textarea_set_accepted_chars(txt_community,
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-");

	row = create_row(main, 34);
	group = create_field_group(row, "Traps available", 62, 68);
	cbx_traps_enabled = create_checkbox(group);
	tt_obj_btn_create(row, refresh_button_cb, LV_SYMBOL_REFRESH, NULL,
			34, 32, LV_ALIGN_CENTER);

	lv_obj_t* trap_label = lv_label_create(main);
	lv_label_set_text(trap_label, "IP / DNS Traps");
	lv_obj_set_width(trap_label, LV_PCT(100));
	lv_obj_set_style_text_align(trap_label, LV_TEXT_ALIGN_LEFT, 0);

	row = create_row(main, 36);
	for (int i = 0; i < 4; i++) {
		txt_managers[i] = tt_obj_txt_create(row, "IP / DNS", text_cb);
		lv_obj_set_size(txt_managers[i], LV_PCT(24), 32);
		lv_textarea_set_max_length(txt_managers[i],
				APP_STATE_NW_TEXT_LEN - 1);
		lv_textarea_set_accepted_chars(txt_managers[i],
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-");
	}

	lv_obj_t* spacer = lv_obj_create(main);
	lv_obj_set_size(spacer, LV_PCT(100), 0);
	lv_obj_set_flex_grow(spacer, 1);
	lv_obj_add_style(spacer, &invisible_cont_style, LV_STATE_DEFAULT);

	row = create_row(main, 52);
	tt_obj_btn_perc_create(row, ok_cb, "OK", 47);
	tt_obj_btn_perc_create(row, cancel_cb, "Cancel", 47);

	apply_snapshot();
}
