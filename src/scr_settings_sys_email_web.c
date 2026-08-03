#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "lvgl/lvgl.h"
#include "http_async.h"
#include "scr_keyboard.h"
#include "scr_settings_sys_email_web.h"
#include "tt_obj.h"
#include "tt_styles.h"

#define EMAIL_WEB_URL "http://localhost:8001/email-web"
#define SERVER_LEN 254
#define MAIL_ADDRESS_LEN 255
#define PASSWORD_LEN 129
#define RECIPIENT_COUNT 3
#define ROW_HEIGHT 38
#define CONTROL_HEIGHT 32

typedef enum {
	WEB_HTTP = 0,
	WEB_HTTPS,
} web_protocol_t;

typedef enum {
	SMTP_AUTH_NONE = 0,
	SMTP_AUTH_LOGIN,
} smtp_auth_t;

typedef struct {
	web_protocol_t web_protocol;
	uint16_t web_port;
	char smtp_server[SERVER_LEN];
	uint16_t smtp_port;
	smtp_auth_t smtp_auth;
	char from_address[MAIL_ADDRESS_LEN];
	bool password_configured;
	char recipients[RECIPIENT_COUNT][MAIL_ADDRESS_LEN];
} email_web_config_t;

static lv_obj_t* dd_web_protocol;
static lv_obj_t* txt_web_port;
static lv_obj_t* txt_smtp_server;
static lv_obj_t* txt_smtp_port;
static lv_obj_t* dd_smtp_auth;
static lv_obj_t* txt_from_address;
static lv_obj_t* txt_password;
static lv_obj_t* txt_recipients[RECIPIENT_COUNT];
static email_web_config_t loaded_config;
static bool request_pending;

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

static void open_keyboard(lv_event_t* e, keyboard_type_t type)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		lv_obj_t* keyboard = scr_keyboard_create(
				lv_scr_act(), lv_event_get_target(e), type);
		lv_scr_load(keyboard);
	}
}

static void text_cb(lv_event_t* e)
{
	open_keyboard(e, KB_ABC);
}

static void number_cb(lv_event_t* e)
{
	open_keyboard(e, KB_NUM);
}

static bool parse_port(const char* text, uint16_t* port)
{
	if (text == NULL || text[0] == '\0' || port == NULL) {
		return false;
	}
	errno = 0;
	char* end = NULL;
	long value = strtol(text, &end, 10);
	if (errno != 0 || end == text || *end != '\0' ||
			value < 1 || value > 65535) {
		return false;
	}
	*port = (uint16_t)value;
	return true;
}

static bool valid_server(const char* value)
{
	if (value == NULL || strlen(value) >= SERVER_LEN) {
		return false;
	}
	for (const char* ptr = value; *ptr != '\0'; ptr++) {
		if (!isalnum((unsigned char)*ptr) && *ptr != '.' && *ptr != '_' &&
				*ptr != ':' && *ptr != '-') {
			return false;
		}
	}
	return true;
}

static bool valid_mail_address(const char* value, bool allow_empty)
{
	if (value == NULL || strlen(value) >= MAIL_ADDRESS_LEN) {
		return false;
	}
	if (value[0] == '\0') {
		return allow_empty;
	}
	const char* at = strrchr(value, '@');
	if (at == NULL || at == value || strchr(at + 1, '.') == NULL ||
			at[1] == '.' || strchr(value, '@') != at) {
		return false;
	}
	for (const char* ptr = value; *ptr != '\0'; ptr++) {
		if (isspace((unsigned char)*ptr) || iscntrl((unsigned char)*ptr)) {
			return false;
		}
	}
	return true;
}

static bool json_string(cJSON* root, const char* key, char* destination,
		size_t size)
{
	cJSON* value = cJSON_GetObjectItemCaseSensitive(root, key);
	if (!cJSON_IsString(value) || value->valuestring == NULL ||
			strlen(value->valuestring) >= size) {
		return false;
	}
	snprintf(destination, size, "%s", value->valuestring);
	return true;
}

static bool parse_config(const char* json, email_web_config_t* config)
{
	if (json == NULL || config == NULL) {
		return false;
	}
	cJSON* root = cJSON_Parse(json);
	if (root == NULL) {
		return false;
	}
	cJSON* protocol = cJSON_GetObjectItemCaseSensitive(root, "web_protocol");
	cJSON* web_port = cJSON_GetObjectItemCaseSensitive(root, "web_port");
	cJSON* smtp_port = cJSON_GetObjectItemCaseSensitive(root, "smtp_port");
	cJSON* auth = cJSON_GetObjectItemCaseSensitive(root, "smtp_auth");
	cJSON* password = cJSON_GetObjectItemCaseSensitive(
			root, "password_configured");
	cJSON* recipients = cJSON_GetObjectItemCaseSensitive(root, "recipients");
	if (!cJSON_IsString(protocol) ||
			(strcmp(protocol->valuestring, "http") != 0 &&
			strcmp(protocol->valuestring, "https") != 0) ||
			!cJSON_IsNumber(web_port) || web_port->valueint < 1 ||
			web_port->valueint > 65535 || !cJSON_IsNumber(smtp_port) ||
			smtp_port->valueint < 1 || smtp_port->valueint > 65535 ||
			!cJSON_IsString(auth) ||
			(strcmp(auth->valuestring, "none") != 0 &&
			strcmp(auth->valuestring, "login") != 0) ||
			!cJSON_IsBool(password) || !cJSON_IsArray(recipients) ||
			cJSON_GetArraySize(recipients) > RECIPIENT_COUNT) {
		cJSON_Delete(root);
		return false;
	}
	memset(config, 0, sizeof(*config));
	if (!json_string(root, "smtp_server", config->smtp_server,
				sizeof(config->smtp_server)) ||
			!json_string(root, "from_address", config->from_address,
				sizeof(config->from_address)) ||
			!valid_server(config->smtp_server) ||
			!valid_mail_address(config->from_address, true)) {
		cJSON_Delete(root);
		return false;
	}
	config->web_protocol = strcmp(protocol->valuestring, "https") == 0 ?
			WEB_HTTPS : WEB_HTTP;
	config->web_port = (uint16_t)web_port->valueint;
	config->smtp_port = (uint16_t)smtp_port->valueint;
	config->smtp_auth = strcmp(auth->valuestring, "login") == 0 ?
			SMTP_AUTH_LOGIN : SMTP_AUTH_NONE;
	config->password_configured = cJSON_IsTrue(password);
	int index = 0;
	cJSON* item;
	cJSON_ArrayForEach(item, recipients) {
		if (!cJSON_IsString(item) || item->valuestring == NULL ||
				!valid_mail_address(item->valuestring, false)) {
			cJSON_Delete(root);
			return false;
		}
		snprintf(config->recipients[index],
				sizeof(config->recipients[index]), "%s", item->valuestring);
		index++;
	}
	cJSON_Delete(root);
	return true;
}

static void set_port_text(lv_obj_t* textarea, uint16_t port)
{
	char value[6];
	snprintf(value, sizeof(value), "%u", port);
	lv_textarea_set_text(textarea, value);
}

static void apply_config(void)
{
	lv_dropdown_set_selected(dd_web_protocol, loaded_config.web_protocol);
	set_port_text(txt_web_port, loaded_config.web_port);
	lv_textarea_set_text(txt_smtp_server, loaded_config.smtp_server);
	set_port_text(txt_smtp_port, loaded_config.smtp_port);
	lv_dropdown_set_selected(dd_smtp_auth, loaded_config.smtp_auth);
	lv_textarea_set_text(txt_from_address, loaded_config.from_address);
	lv_textarea_set_text(txt_password, "");
	lv_textarea_set_placeholder_text(txt_password,
			loaded_config.password_configured ? "Saved - enter new" : "Password");
	for (int i = 0; i < RECIPIENT_COUNT; i++) {
		lv_textarea_set_text(txt_recipients[i], loaded_config.recipients[i]);
	}
}

static char* serialize_config(void)
{
	cJSON* root = cJSON_CreateObject();
	if (root == NULL) {
		return NULL;
	}
	cJSON_AddStringToObject(root, "web_protocol",
			lv_dropdown_get_selected(dd_web_protocol) == WEB_HTTPS ?
			"https" : "http");
	uint16_t web_port;
	uint16_t smtp_port;
	if (!parse_port(lv_textarea_get_text(txt_web_port), &web_port) ||
			!parse_port(lv_textarea_get_text(txt_smtp_port), &smtp_port)) {
		cJSON_Delete(root);
		return NULL;
	}
	cJSON_AddNumberToObject(root, "web_port", web_port);
	cJSON_AddStringToObject(root, "smtp_server",
			lv_textarea_get_text(txt_smtp_server));
	cJSON_AddNumberToObject(root, "smtp_port", smtp_port);
	cJSON_AddStringToObject(root, "smtp_auth",
			lv_dropdown_get_selected(dd_smtp_auth) == SMTP_AUTH_LOGIN ?
			"login" : "none");
	cJSON_AddStringToObject(root, "from_address",
			lv_textarea_get_text(txt_from_address));
	const char* password = lv_textarea_get_text(txt_password);
	if (password[0] == '\0') {
		cJSON_AddNullToObject(root, "password");
	} else {
		cJSON_AddStringToObject(root, "password", password);
	}
	cJSON* recipients = cJSON_AddArrayToObject(root, "recipients");
	for (int i = 0; i < RECIPIENT_COUNT; i++) {
		const char* recipient = lv_textarea_get_text(txt_recipients[i]);
		if (recipient[0] != '\0') {
			cJSON_AddItemToArray(recipients, cJSON_CreateString(recipient));
		}
	}
	char* json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json;
}

static bool validate_form(void)
{
	const char* server = lv_textarea_get_text(txt_smtp_server);
	const char* from = lv_textarea_get_text(txt_from_address);
	const char* password = lv_textarea_get_text(txt_password);
	uint16_t ignored;
	if (!parse_port(lv_textarea_get_text(txt_web_port), &ignored) ||
			!parse_port(lv_textarea_get_text(txt_smtp_port), &ignored)) {
		tt_obj_info_box_create("Email/Web", "Ports must be between 1 and 65535", 1);
		return false;
	}
	if (!valid_server(server)) {
		tt_obj_info_box_create("Email/Web", "Invalid SMTP server", 1);
		return false;
	}
	bool has_recipient = false;
	for (int i = 0; i < RECIPIENT_COUNT; i++) {
		const char* recipient = lv_textarea_get_text(txt_recipients[i]);
		if (!valid_mail_address(recipient, true)) {
			tt_obj_info_box_create("Email/Web", "Invalid recipient address", 1);
			return false;
		}
		has_recipient = has_recipient || recipient[0] != '\0';
	}
	bool email_configured = server[0] != '\0' || from[0] != '\0' ||
			has_recipient || lv_dropdown_get_selected(dd_smtp_auth) != 0;
	if (!valid_mail_address(from, !email_configured)) {
		tt_obj_info_box_create("Email/Web", "Invalid From address", 1);
		return false;
	}
	if (email_configured && (server[0] == '\0' || from[0] == '\0' ||
			!has_recipient)) {
		tt_obj_info_box_create("Email/Web",
				"SMTP server, From and recipient are required", 1);
		return false;
	}
	if (lv_dropdown_get_selected(dd_smtp_auth) == SMTP_AUTH_LOGIN &&
			password[0] == '\0' && !loaded_config.password_configured) {
		tt_obj_info_box_create("Email/Web", "SMTP login password is required", 1);
		return false;
	}
	return true;
}

static void load_cb(int err, void* buffer, size_t len, void* userdata)
{
	(void)len;
	(void)userdata;
	request_pending = false;
	if (err != 0 || !parse_config(buffer, &loaded_config)) {
		free(buffer);
		tt_obj_info_box_create("Email/Web", "Can not load settings", 1);
		return;
	}
	free(buffer);
	apply_config();
}

static void save_cb(int err, void* buffer, size_t len, void* userdata)
{
	(void)len;
	(void)userdata;
	request_pending = false;
	if (err != 0 || !parse_config(buffer, &loaded_config)) {
		free(buffer);
		tt_obj_info_box_create("Email/Web", "Can not save settings", 1);
		return;
	}
	free(buffer);
	apply_config();
	tt_obj_info_box_create("Email/Web", "Settings saved", 0);
}

static void request_load(void)
{
	if (request_pending) {
		return;
	}
	request_pending = true;
	if (http_async_get(EMAIL_WEB_URL, load_cb, NULL) < 0) {
		request_pending = false;
		tt_obj_info_box_create("Email/Web", "Can not load settings", 1);
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
		request_load();
	}
}

static void ok_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || request_pending ||
			!validate_form()) {
		return;
	}
	char* json = serialize_config();
	if (json == NULL) {
		tt_obj_info_box_create("Email/Web", "Can not prepare settings", 1);
		return;
	}
	request_pending = true;
	if (http_async_put(EMAIL_WEB_URL, json, save_cb, NULL) < 0) {
		request_pending = false;
		tt_obj_info_box_create("Email/Web", "Can not save settings", 1);
	}
	free(json);
}

static void cancel_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED && !request_pending) {
		apply_config();
	}
}

void scr_settings_sys_email_web_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* page = tt_obj_menu_page_create(
			menu, btn, menu_cb, "Email / Web");
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
	lv_obj_set_style_pad_row(main, 2, 0);

	lv_obj_t* row = create_row(main, ROW_HEIGHT);
	lv_obj_t* group = create_field_group(row, "Web", 62, 34);
	dd_web_protocol = tt_obj_dropdown_create(group, "HTTP\nHTTPS", NULL);
	lv_obj_set_size(dd_web_protocol, LV_PCT(64), CONTROL_HEIGHT);
	group = create_field_group(row, "Port", 36, 35);
	txt_web_port = tt_obj_txt_create(group, "Port", number_cb);
	lv_obj_set_size(txt_web_port, LV_PCT(64), CONTROL_HEIGHT);
	lv_textarea_set_accepted_chars(txt_web_port, "0123456789");
	lv_textarea_set_max_length(txt_web_port, 5);

	row = create_row(main, ROW_HEIGHT);
	group = create_field_group(row, "email SMTP", 62, 38);
	txt_smtp_server = tt_obj_txt_create(group, "SMTP server", text_cb);
	lv_obj_set_size(txt_smtp_server, LV_PCT(60), CONTROL_HEIGHT);
	lv_textarea_set_max_length(txt_smtp_server, SERVER_LEN - 1);
	lv_textarea_set_accepted_chars(txt_smtp_server,
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.:-");
	group = create_field_group(row, "Port", 36, 35);
	txt_smtp_port = tt_obj_txt_create(group, "Port", number_cb);
	lv_obj_set_size(txt_smtp_port, LV_PCT(64), CONTROL_HEIGHT);
	lv_textarea_set_accepted_chars(txt_smtp_port, "0123456789");
	lv_textarea_set_max_length(txt_smtp_port, 5);

	row = create_row(main, ROW_HEIGHT);
	group = create_field_group(row, "Auth", 62, 38);
	dd_smtp_auth = tt_obj_dropdown_create(group, "None\nLogin", NULL);
	lv_obj_set_size(dd_smtp_auth, LV_PCT(60), CONTROL_HEIGHT);

	row = create_row(main, ROW_HEIGHT);
	group = create_field_group(row, "From", 62, 30);
	txt_from_address = tt_obj_txt_create(group, "From address", text_cb);
	lv_obj_set_size(txt_from_address, LV_PCT(68), CONTROL_HEIGHT);
	lv_textarea_set_max_length(txt_from_address, MAIL_ADDRESS_LEN - 1);
	group = create_field_group(row, "Password", 36, 43);
	txt_password = tt_obj_txt_create(group, "Password", text_cb);
	lv_obj_set_size(txt_password, LV_PCT(55), CONTROL_HEIGHT);
	lv_textarea_set_password_mode(txt_password, true);
	lv_textarea_set_max_length(txt_password, PASSWORD_LEN - 1);

	lv_obj_t* label = lv_label_create(main);
	lv_label_set_text(label, "Addresses to");
	lv_obj_set_width(label, LV_PCT(100));
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

	row = create_row(main, ROW_HEIGHT);
	for (int i = 0; i < RECIPIENT_COUNT; i++) {
		txt_recipients[i] = tt_obj_txt_create(row, "Recipient", text_cb);
		lv_obj_set_size(txt_recipients[i], LV_PCT(32), CONTROL_HEIGHT);
		lv_textarea_set_max_length(
				txt_recipients[i], MAIL_ADDRESS_LEN - 1);
	}

	row = create_row(main, 48);
	tt_obj_btn_perc_create(row, ok_cb, "OK", 47);
	tt_obj_btn_perc_create(row, cancel_cb, "Cancel", 47);

	memset(&loaded_config, 0, sizeof(loaded_config));
	request_load();
}
