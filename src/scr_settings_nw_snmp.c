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
#define SNMP_V3_USER_MAX 32
#define SNMP_V3_PASSWORD_MAX 64
#define SNMP_SUCCESS_MSG_MS 2000

static lv_obj_t* menu_handle;
static lv_obj_t* cbx_enabled;
static lv_obj_t* cbx_set_enabled;
static lv_obj_t* cbx_traps_enabled;
static lv_obj_t* dd_version;
static lv_obj_t* lbl_community;
static lv_obj_t* txt_community;
static lv_obj_t* txt_managers[4];
static lv_obj_t* row_v3_security;
static lv_obj_t* row_v3_auth;
static lv_obj_t* row_v3_privacy;
static lv_obj_t* dd_v3_security;
static lv_obj_t* dd_v3_auth;
static lv_obj_t* txt_v3_auth_password;
static lv_obj_t* dd_v3_privacy;
static lv_obj_t* txt_v3_privacy_password;
static char v1_community_draft[65];
static char v3_user_draft[33];
static int displayed_version;
static bool refresh_pending;
static bool save_pending;
static lv_obj_t* save_msgbox;

static void refresh_cb(int err, void* userdata);
static void save_cb(int err, void* userdata);

static void close_save_msgbox(void)
{
	if (save_msgbox != NULL && lv_obj_is_valid(save_msgbox)) {
		lv_msgbox_close(save_msgbox);
	}
	save_msgbox = NULL;
}

static void save_msgbox_timer_cb(lv_timer_t* timer)
{
	(void)timer;
	close_save_msgbox();
}

static void show_save_wait_msgbox(void)
{
	close_save_msgbox();
	save_msgbox = tt_obj_info_box_create("SNMP",
			"Applying SNMP settings...\nPlease wait a moment.", 0);
	lv_obj_t* close_btn = lv_msgbox_get_close_btn(save_msgbox);
	if (close_btn != NULL) {
		lv_obj_add_flag(close_btn, LV_OBJ_FLAG_HIDDEN);
	}
}

static void set_hidden(lv_obj_t* object, bool hidden)
{
	if (hidden) {
		lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
	}
}

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

static void update_security_rows(void)
{
	bool v3 = displayed_version == 2;
	int security = (int)lv_dropdown_get_selected(dd_v3_security);
	set_hidden(row_v3_security, !v3);
	set_hidden(row_v3_auth, !v3 || security == 0);
	set_hidden(row_v3_privacy, !v3 || security != 2);
}

static void version_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	const char* current = lv_textarea_get_text(txt_community);
	if (displayed_version == 2) {
		snprintf(v3_user_draft, sizeof(v3_user_draft), "%s", current);
	} else {
		snprintf(v1_community_draft, sizeof(v1_community_draft), "%s",
				current);
	}
	displayed_version = (int)lv_dropdown_get_selected(dd_version);
	lv_label_set_text(lbl_community,
			displayed_version == 2 ? "V3 user" : "Community");
	lv_textarea_set_text(txt_community,
			displayed_version == 2 ? v3_user_draft : v1_community_draft);
	lv_textarea_set_max_length(txt_community,
			displayed_version == 2 ? SNMP_V3_USER_MAX : SNMP_COMMUNITY_MAX);
	update_security_rows();
}

static void security_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		update_security_rows();
	}
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
	snprintf(v1_community_draft, sizeof(v1_community_draft), "%s",
			snapshot.snmp.community);
	snprintf(v3_user_draft, sizeof(v3_user_draft), "%s",
			snapshot.snmp.v3_user);
	displayed_version = snapshot.snmp.version >= 0 &&
			snapshot.snmp.version <= 2 ? snapshot.snmp.version : 1;
	lv_dropdown_set_selected(dd_version, (uint16_t)displayed_version);
	lv_label_set_text(lbl_community,
			displayed_version == 2 ? "V3 user" : "Community");
	lv_textarea_set_text(txt_community,
			displayed_version == 2 ? v3_user_draft : v1_community_draft);
	lv_textarea_set_max_length(txt_community,
			displayed_version == 2 ? SNMP_V3_USER_MAX : SNMP_COMMUNITY_MAX);
	lv_dropdown_set_selected(dd_v3_security,
			(uint16_t)snapshot.snmp.v3_security_level);
	lv_dropdown_set_selected(dd_v3_auth,
			(uint16_t)snapshot.snmp.v3_auth_algorithm);
	lv_dropdown_set_selected(dd_v3_privacy,
			(uint16_t)snapshot.snmp.v3_privacy_algorithm);
	lv_textarea_set_text(txt_v3_auth_password, "");
	lv_textarea_set_text(txt_v3_privacy_password, "");
	update_security_rows();
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

static bool valid_password(const char* value)
{
	if (value == NULL || value[0] == '\0') {
		return true;
	}
	size_t length = strlen(value);
	if (length < 8 || length > SNMP_V3_PASSWORD_MAX) {
		return false;
	}
	for (const char* ptr = value; *ptr != '\0'; ptr++) {
		if (!isalnum((unsigned char)*ptr) &&
				strchr("_.@#%+=:-", *ptr) == NULL) {
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
	int version = (int)lv_dropdown_get_selected(dd_version);
	const char* identity = lv_textarea_get_text(txt_community);
	if (!valid_community(identity) ||
			(version == 2 && strlen(identity) > SNMP_V3_USER_MAX)) {
		tt_obj_info_box_create("SNMP",
				version == 2 ?
				"V3 user must use 1-32 letters, numbers, '.', '-' or '_'" :
				"Community must use 1-64 letters, numbers, '.', '-' or '_'", 1);
		return;
	}

	app_state_snmp_t snmp;
	memset(&snmp, 0, sizeof(snmp));
	snmp.enabled = is_checked(cbx_enabled);
	snmp.version = version;
	snmp.set_enabled = is_checked(cbx_set_enabled);
	snmp.traps_enabled = is_checked(cbx_traps_enabled);
	if (version == 2) {
		snprintf(v3_user_draft, sizeof(v3_user_draft), "%s", identity);
	} else {
		snprintf(v1_community_draft, sizeof(v1_community_draft), "%s",
				identity);
	}
	snprintf(snmp.community, sizeof(snmp.community), "%s",
			v1_community_draft);
	snprintf(snmp.v3_user, sizeof(snmp.v3_user), "%s", v3_user_draft);
	snmp.v3_security_level = (int)lv_dropdown_get_selected(dd_v3_security);
	snmp.v3_auth_algorithm = (int)lv_dropdown_get_selected(dd_v3_auth);
	snmp.v3_privacy_algorithm = (int)lv_dropdown_get_selected(dd_v3_privacy);
	const char* auth_password = lv_textarea_get_text(txt_v3_auth_password);
	const char* privacy_password = lv_textarea_get_text(
			txt_v3_privacy_password);
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	bool new_v3_user = !snapshot.snmp.v3_configured ||
			strcmp(snmp.v3_user, snapshot.snmp.v3_user) != 0;
	bool new_auth_credentials = new_v3_user ||
			snapshot.snmp.v3_security_level == 0 ||
			snmp.v3_auth_algorithm != snapshot.snmp.v3_auth_algorithm;
	bool new_privacy_credentials = new_v3_user ||
			snapshot.snmp.v3_security_level != 2 ||
			snmp.v3_privacy_algorithm != snapshot.snmp.v3_privacy_algorithm;
	if (version == 2 && snmp.v3_security_level >= 1 &&
			((new_auth_credentials && auth_password[0] == '\0') ||
			!valid_password(auth_password))) {
		tt_obj_info_box_create("SNMP",
				"V3 authentication password must be at least 8 characters", 1);
		return;
	}
	if (version == 2 && snmp.v3_security_level == 2 &&
			((new_privacy_credentials && privacy_password[0] == '\0') ||
			!valid_password(privacy_password))) {
		tt_obj_info_box_create("SNMP",
				"V3 privacy password must be at least 8 characters", 1);
		return;
	}
	snprintf(snmp.v3_auth_password, sizeof(snmp.v3_auth_password), "%s",
			auth_password);
	snprintf(snmp.v3_privacy_password,
			sizeof(snmp.v3_privacy_password), "%s", privacy_password);
	snmp.v3_configured = snapshot.snmp.v3_configured;
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
	show_save_wait_msgbox();
	if (backend_snmp_save(&snmp, save_cb, NULL) != 0) {
		save_pending = false;
		close_save_msgbox();
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
		close_save_msgbox();
		tt_obj_info_box_create("SNMP", "Can not apply SNMP settings", 1);
		return;
	}
	apply_snapshot();
	if (save_msgbox != NULL && lv_obj_is_valid(save_msgbox)) {
		lv_label_set_text(lv_msgbox_get_text(save_msgbox),
				"SNMP settings applied");
		lv_timer_t* timer = lv_timer_create(save_msgbox_timer_cb,
				SNMP_SUCCESS_MSG_MS, NULL);
		lv_timer_set_repeat_count(timer, 1);
	}
}

void scr_settings_nw_snmp_create(lv_obj_t* menu, lv_obj_t* btn)
{
	menu_handle = menu;
	lv_obj_t* page = tt_obj_menu_page_create(menu, btn, menu_cb, "SNMP");
	lv_obj_t* main = tt_obj_cont_create(page);
	lv_obj_set_width(main, LV_PCT(100));
	lv_obj_set_height(main, 0);
	lv_obj_set_flex_grow(main, 1);
	lv_obj_add_flag(main, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scrollbar_mode(main, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(main, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(main, 3, 0);
	lv_obj_set_style_pad_row(main, 1, 0);

	lv_obj_t* row = create_row(main, SNMP_ROW_HEIGHT);
	lv_obj_t* group = create_field_group(row, "SNMP enable", 45, 58);
	cbx_enabled = create_checkbox(group);
	group = create_field_group(row, "Version", 53, 38);
	dd_version = tt_obj_dropdown_create(group, "V1\nV2c\nV3", version_cb);
	lv_obj_set_size(dd_version, LV_PCT(60), 32);

	row = create_row(main, SNMP_ROW_HEIGHT);
	group = create_field_group(row, "SET enable", 45, 58);
	cbx_set_enabled = create_checkbox(group);
	group = create_field_group(row, "Community", 53, 38);
	lbl_community = lv_obj_get_child(group, 0);
	txt_community = tt_obj_txt_create(group, "Community", text_cb);
	lv_obj_set_size(txt_community, LV_PCT(60), 32);
	lv_textarea_set_max_length(txt_community, SNMP_COMMUNITY_MAX);
	lv_textarea_set_accepted_chars(txt_community,
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-");

	row_v3_security = create_row(main, SNMP_ROW_HEIGHT);
	group = create_field_group(row_v3_security, "Security level", 100, 38);
	dd_v3_security = tt_obj_dropdown_create(group,
			"noAuthNoPriv\nauthNoPriv\nauthPriv", security_cb);
	lv_obj_set_size(dd_v3_security, LV_PCT(60), 32);
	lv_dropdown_set_selected(dd_v3_security, 2);

	row_v3_auth = create_row(main, SNMP_ROW_HEIGHT);
	group = create_field_group(row_v3_auth, "Auth", 43, 38);
	dd_v3_auth = tt_obj_dropdown_create(group, "MD5\nSHA", NULL);
	lv_obj_set_size(dd_v3_auth, LV_PCT(60), 32);
	lv_dropdown_set_selected(dd_v3_auth, 1);
	group = create_field_group(row_v3_auth, "Password", 55, 38);
	txt_v3_auth_password = tt_obj_txt_create(group, "New password", text_cb);
	lv_obj_set_size(txt_v3_auth_password, LV_PCT(60), 32);
	lv_textarea_set_password_mode(txt_v3_auth_password, true);
	lv_textarea_set_max_length(txt_v3_auth_password, SNMP_V3_PASSWORD_MAX);
	lv_textarea_set_accepted_chars(txt_v3_auth_password,
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.@#%+=:-");

	row_v3_privacy = create_row(main, SNMP_ROW_HEIGHT);
	group = create_field_group(row_v3_privacy, "Privacy", 43, 38);
	dd_v3_privacy = tt_obj_dropdown_create(group, "DES\nAES", NULL);
	lv_obj_set_size(dd_v3_privacy, LV_PCT(60), 32);
	lv_dropdown_set_selected(dd_v3_privacy, 1);
	group = create_field_group(row_v3_privacy, "Password", 55, 38);
	txt_v3_privacy_password = tt_obj_txt_create(
			group, "New password", text_cb);
	lv_obj_set_size(txt_v3_privacy_password, LV_PCT(60), 32);
	lv_textarea_set_password_mode(txt_v3_privacy_password, true);
	lv_textarea_set_max_length(txt_v3_privacy_password,
			SNMP_V3_PASSWORD_MAX);
	lv_textarea_set_accepted_chars(txt_v3_privacy_password,
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.@#%+=:-");

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
