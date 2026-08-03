#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <cjson/cJSON.h>

#include "lvgl/lvgl.h"
#include "http_async.h"
#include "scr_keyboard.h"
#include "scr_settings_sys_users.h"
#include "tt_obj.h"
#include "tt_styles.h"

#define USER_ACCESS_URL "http://localhost:8001/user-access"
#define USER_ACCESS_MAX_USERS 16
#define USER_ACCESS_MAX_LEVELS 8
#define USER_ACCESS_NAME_LEN 25

typedef enum {
	ACCESS_READ_ONLY = 0,
	ACCESS_CONTROL,
	ACCESS_FULL_EDIT,
} access_capacity_t;

typedef struct {
	char name[USER_ACCESS_NAME_LEN];
	char level[USER_ACCESS_NAME_LEN];
} user_entry_t;

typedef struct {
	char name[USER_ACCESS_NAME_LEN];
	access_capacity_t capacity;
} level_entry_t;

typedef struct {
	user_entry_t users[USER_ACCESS_MAX_USERS];
	level_entry_t levels[USER_ACCESS_MAX_LEVELS];
	int user_count;
	int level_count;
} user_access_config_t;

static lv_obj_t* users_list;
static lv_obj_t* levels_list;
static lv_obj_t* txt_user_name;
static lv_obj_t* txt_level_name;
static lv_obj_t* dd_user_level;
static lv_obj_t* dd_level_capacity;
static user_access_config_t loaded_config;
static user_access_config_t draft_config;
static bool request_pending;

static const char* capacity_api(access_capacity_t capacity)
{
	static const char* values[] = {"readOnly", "control", "fullEdit"};
	return values[capacity >= ACCESS_READ_ONLY &&
			capacity <= ACCESS_FULL_EDIT ? capacity : ACCESS_READ_ONLY];
}

static const char* capacity_display(access_capacity_t capacity)
{
	static const char* values[] = {"Read only", "Control", "Full edit"};
	return values[capacity >= ACCESS_READ_ONLY &&
			capacity <= ACCESS_FULL_EDIT ? capacity : ACCESS_READ_ONLY];
}

static bool parse_capacity(const char* value, access_capacity_t* capacity)
{
	if (value == NULL || capacity == NULL) {
		return false;
	}
	if (strcmp(value, "readOnly") == 0) {
		*capacity = ACCESS_READ_ONLY;
	} else if (strcmp(value, "control") == 0) {
		*capacity = ACCESS_CONTROL;
	} else if (strcmp(value, "fullEdit") == 0) {
		*capacity = ACCESS_FULL_EDIT;
	} else {
		return false;
	}
	return true;
}

static bool valid_name(const char* value)
{
	if (value == NULL || value[0] == '\0' ||
			strlen(value) >= USER_ACCESS_NAME_LEN) {
		return false;
	}
	for (const char* ptr = value; *ptr != '\0'; ptr++) {
		if (!isalnum((unsigned char)*ptr) && *ptr != ' ' && *ptr != '_' &&
				*ptr != '-' && *ptr != '.') {
			return false;
		}
	}
	return true;
}

static int level_index(const user_access_config_t* config, const char* name)
{
	for (int i = 0; i < config->level_count; i++) {
		if (strcasecmp(config->levels[i].name, name) == 0) {
			return i;
		}
	}
	return -1;
}

static bool user_name_exists(const char* name)
{
	for (int i = 0; i < draft_config.user_count; i++) {
		if (strcasecmp(draft_config.users[i].name, name) == 0) {
			return true;
		}
	}
	return false;
}

static bool level_name_exists(const char* name)
{
	return level_index(&draft_config, name) >= 0;
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

static lv_obj_t* create_list_box(lv_obj_t* parent)
{
	lv_obj_t* list = lv_obj_create(parent);
	lv_obj_set_size(list, LV_PCT(100), 82);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_scroll_dir(list, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(list, 2, 0);
	lv_obj_set_style_border_color(list, lv_color_white(), 0);
	lv_obj_set_style_radius(list, 14, 0);
	lv_obj_set_style_pad_all(list, 7, 0);
	lv_obj_set_style_pad_row(list, 4, 0);
	return list;
}

static void add_list_line(lv_obj_t* list, const char* text)
{
	lv_obj_t* label = lv_label_create(list);
	lv_label_set_text(label, text);
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(label, LV_PCT(100));
}

static void rebuild_level_dropdown(void)
{
	char options[USER_ACCESS_MAX_LEVELS * USER_ACCESS_NAME_LEN];
	options[0] = '\0';
	for (int i = 0; i < draft_config.level_count; i++) {
		if (i > 0) {
			strncat(options, "\n", sizeof(options) - strlen(options) - 1);
		}
		strncat(options, draft_config.levels[i].name,
				sizeof(options) - strlen(options) - 1);
	}
	lv_dropdown_set_options(dd_user_level,
			options[0] != '\0' ? options : "No levels");
	lv_dropdown_set_selected(dd_user_level, 0);
}

static void rebuild_lists(void)
{
	lv_obj_clean(users_list);
	if (draft_config.user_count == 0) {
		add_list_line(users_list, "No users");
	}
	for (int i = 0; i < draft_config.user_count; i++) {
		char line[2 * USER_ACCESS_NAME_LEN + 8];
		snprintf(line, sizeof(line), "%s  -  %s",
				draft_config.users[i].name, draft_config.users[i].level);
		add_list_line(users_list, line);
	}

	lv_obj_clean(levels_list);
	for (int i = 0; i < draft_config.level_count; i++) {
		char line[2 * USER_ACCESS_NAME_LEN + 8];
		snprintf(line, sizeof(line), "%s  -  %s",
				draft_config.levels[i].name,
				capacity_display(draft_config.levels[i].capacity));
		add_list_line(levels_list, line);
	}
	rebuild_level_dropdown();
}

static bool parse_config(const char* json, user_access_config_t* config)
{
	if (json == NULL || config == NULL) {
		return false;
	}
	cJSON* root = cJSON_Parse(json);
	if (root == NULL) {
		return false;
	}
	cJSON* levels = cJSON_GetObjectItemCaseSensitive(root, "levels");
	cJSON* users = cJSON_GetObjectItemCaseSensitive(root, "users");
	if (!cJSON_IsArray(levels) || !cJSON_IsArray(users) ||
			cJSON_GetArraySize(levels) < 1 ||
			cJSON_GetArraySize(levels) > USER_ACCESS_MAX_LEVELS ||
			cJSON_GetArraySize(users) > USER_ACCESS_MAX_USERS) {
		cJSON_Delete(root);
		return false;
	}
	memset(config, 0, sizeof(*config));
	cJSON* item;
	cJSON_ArrayForEach(item, levels) {
		cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "name");
		cJSON* capacity = cJSON_GetObjectItemCaseSensitive(item, "capacity");
		level_entry_t* level = &config->levels[config->level_count];
		if (!cJSON_IsString(name) || !cJSON_IsString(capacity) ||
				!valid_name(name->valuestring) ||
				!parse_capacity(capacity->valuestring, &level->capacity)) {
			cJSON_Delete(root);
			return false;
		}
		snprintf(level->name, sizeof(level->name), "%s", name->valuestring);
		config->level_count++;
	}
	cJSON_ArrayForEach(item, users) {
		cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "name");
		cJSON* level = cJSON_GetObjectItemCaseSensitive(item, "level");
		user_entry_t* user = &config->users[config->user_count];
		if (!cJSON_IsString(name) || !cJSON_IsString(level) ||
				!valid_name(name->valuestring) ||
				level_index(config, level->valuestring) < 0) {
			cJSON_Delete(root);
			return false;
		}
		snprintf(user->name, sizeof(user->name), "%s", name->valuestring);
		snprintf(user->level, sizeof(user->level), "%s", level->valuestring);
		config->user_count++;
	}
	cJSON_Delete(root);
	return true;
}

static char* serialize_config(void)
{
	cJSON* root = cJSON_CreateObject();
	cJSON* users = cJSON_AddArrayToObject(root, "users");
	cJSON* levels = cJSON_AddArrayToObject(root, "levels");
	for (int i = 0; i < draft_config.user_count; i++) {
		cJSON* user = cJSON_CreateObject();
		cJSON_AddStringToObject(user, "name", draft_config.users[i].name);
		cJSON_AddStringToObject(user, "level", draft_config.users[i].level);
		cJSON_AddItemToArray(users, user);
	}
	for (int i = 0; i < draft_config.level_count; i++) {
		cJSON* level = cJSON_CreateObject();
		cJSON_AddStringToObject(level, "name", draft_config.levels[i].name);
		cJSON_AddStringToObject(level, "capacity",
				capacity_api(draft_config.levels[i].capacity));
		cJSON_AddItemToArray(levels, level);
	}
	char* json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json;
}

static void load_cb(int err, void* buffer, size_t len, void* userdata)
{
	(void)len;
	(void)userdata;
	request_pending = false;
	if (err != 0 || !parse_config(buffer, &loaded_config)) {
		free(buffer);
		tt_obj_info_box_create("Users", "Can not load users and levels", 1);
		return;
	}
	free(buffer);
	draft_config = loaded_config;
	rebuild_lists();
}

static void save_cb(int err, void* buffer, size_t len, void* userdata)
{
	(void)len;
	(void)userdata;
	request_pending = false;
	user_access_config_t saved;
	if (err != 0 || !parse_config(buffer, &saved)) {
		free(buffer);
		tt_obj_info_box_create("Users", "Can not save users and levels", 1);
		return;
	}
	free(buffer);
	loaded_config = saved;
	draft_config = saved;
	rebuild_lists();
	tt_obj_info_box_create("Users", "Users and levels saved", 0);
}

static void request_load(void)
{
	if (request_pending) {
		return;
	}
	request_pending = true;
	if (http_async_get(USER_ACCESS_URL, load_cb, NULL) < 0) {
		request_pending = false;
		tt_obj_info_box_create("Users", "Can not load users and levels", 1);
	}
}

static void request_save(void)
{
	if (request_pending) {
		return;
	}
	char* json = serialize_config();
	if (json == NULL) {
		tt_obj_info_box_create("Users", "Can not prepare settings", 1);
		return;
	}
	request_pending = true;
	if (http_async_put(USER_ACCESS_URL, json, save_cb, NULL) < 0) {
		request_pending = false;
		tt_obj_info_box_create("Users", "Can not save users and levels", 1);
	}
	free(json);
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
		request_load();
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

static void add_user_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || request_pending) {
		return;
	}
	const char* name = lv_textarea_get_text(txt_user_name);
	if (!valid_name(name)) {
		tt_obj_info_box_create("Users",
				"User name must contain 1-24 safe characters", 1);
		return;
	}
	if (draft_config.user_count >= USER_ACCESS_MAX_USERS) {
		tt_obj_info_box_create("Users", "Maximum 16 users", 1);
		return;
	}
	if (user_name_exists(name)) {
		tt_obj_info_box_create("Users", "User name already exists", 1);
		return;
	}
	int selected = (int)lv_dropdown_get_selected(dd_user_level);
	if (selected < 0 || selected >= draft_config.level_count) {
		tt_obj_info_box_create("Users", "Select a valid access level", 1);
		return;
	}
	user_entry_t* user = &draft_config.users[draft_config.user_count++];
	snprintf(user->name, sizeof(user->name), "%s", name);
	snprintf(user->level, sizeof(user->level), "%s",
			draft_config.levels[selected].name);
	lv_textarea_set_text(txt_user_name, "");
	rebuild_lists();
}

static void add_level_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || request_pending) {
		return;
	}
	const char* name = lv_textarea_get_text(txt_level_name);
	if (!valid_name(name)) {
		tt_obj_info_box_create("Users",
				"Level name must contain 1-24 safe characters", 1);
		return;
	}
	if (draft_config.level_count >= USER_ACCESS_MAX_LEVELS) {
		tt_obj_info_box_create("Users", "Maximum 8 access levels", 1);
		return;
	}
	if (level_name_exists(name)) {
		tt_obj_info_box_create("Users", "Access level already exists", 1);
		return;
	}
	level_entry_t* level = &draft_config.levels[draft_config.level_count++];
	snprintf(level->name, sizeof(level->name), "%s", name);
	level->capacity = (access_capacity_t)lv_dropdown_get_selected(
			dd_level_capacity);
	lv_textarea_set_text(txt_level_name, "");
	rebuild_lists();
}

static void ok_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		request_save();
	}
}

static void cancel_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || request_pending) {
		return;
	}
	draft_config = loaded_config;
	lv_textarea_set_text(txt_user_name, "");
	lv_textarea_set_text(txt_level_name, "");
	rebuild_lists();
}

static lv_obj_t* create_section_label(lv_obj_t* parent, const char* text)
{
	lv_obj_t* label = lv_label_create(parent);
	lv_label_set_text(label, text);
	lv_obj_set_width(label, LV_PCT(100));
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
	return label;
}

void scr_settings_sys_users_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* page = tt_obj_menu_page_create(menu, btn, menu_cb, "Users");
	lv_obj_t* main = tt_obj_cont_create(page);
	lv_obj_set_width(main, LV_PCT(100));
	lv_obj_set_height(main, 0);
	lv_obj_set_flex_grow(main, 1);
	lv_obj_add_flag(main, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scrollbar_mode(main, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(main, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(main, 4, 0);
	lv_obj_set_style_pad_row(main, 3, 0);

	create_section_label(main, "Users");
	users_list = create_list_box(main);

	lv_obj_t* row = create_row(main, 38);
	tt_obj_btn_perc_create(row, add_user_cb, "Add user", 31);
	txt_user_name = tt_obj_txt_create(row, "User name", text_cb);
	lv_obj_set_size(txt_user_name, LV_PCT(32), 34);
	lv_textarea_set_max_length(txt_user_name, USER_ACCESS_NAME_LEN - 1);
	lv_textarea_set_accepted_chars(txt_user_name,
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.- ");
	dd_user_level = tt_obj_dropdown_create(row, "Full edit", NULL);
	lv_obj_set_size(dd_user_level, LV_PCT(33), 34);

	row = create_row(main, 48);
	tt_obj_btn_perc_create(row, ok_cb, "OK", 47);
	tt_obj_btn_perc_create(row, cancel_cb, "Cancel", 47);

	create_section_label(main, "Level");
	levels_list = create_list_box(main);

	row = create_row(main, 38);
	tt_obj_btn_perc_create(row, add_level_cb, "Add level", 31);
	txt_level_name = tt_obj_txt_create(row, "Level name", text_cb);
	lv_obj_set_size(txt_level_name, LV_PCT(32), 34);
	lv_textarea_set_max_length(txt_level_name, USER_ACCESS_NAME_LEN - 1);
	lv_textarea_set_accepted_chars(txt_level_name,
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.- ");
	dd_level_capacity = tt_obj_dropdown_create(row,
			"Read only\nControl\nFull edit", NULL);
	lv_obj_set_size(dd_level_capacity, LV_PCT(33), 34);

	row = create_row(main, 48);
	tt_obj_btn_perc_create(row, ok_cb, "OK", 47);
	tt_obj_btn_perc_create(row, cancel_cb, "Cancel", 47);

	memset(&loaded_config, 0, sizeof(loaded_config));
	memset(&draft_config, 0, sizeof(draft_config));
	request_load();
}
