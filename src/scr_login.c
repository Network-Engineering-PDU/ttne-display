#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "scr_login.h"
#include "scr_keyboard.h"
#include "scr_splash.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "tt_styles.h"
#include "app/app_state.h"
#include "backend/backend.h"
#include "screen.h"

#define LOGIN_PASSWORD "469130"
#define LOGIN_MSG_MS 2000

static lv_obj_t* login_scr;
static lv_obj_t* menu_scr;
static lv_obj_t* txt_password;
static lv_obj_t* cbx_skip_password;
static bool skip_password = false;
static bool login_ok_this_boot = false;

static void txt_password_cb(lv_event_t* e);
static void cbx_skip_password_cb(lv_event_t* e);
static void btn_login_cb(lv_event_t* e);
static lv_obj_t* login_msg_create(char* title, char* msg, int severity);
static void login_msg_close_timer_cb(lv_timer_t* timer);
static void login_success_msg_timer_cb(lv_timer_t* timer);

static void txt_password_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_PASS);
		lv_scr_load(kb_scr);
	}
}

static void cbx_skip_password_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* obj = lv_event_get_target(e);
		skip_password = (lv_obj_get_state(obj) & LV_STATE_CHECKED) != 0;
	}
}

static lv_obj_t* login_msg_create(char* title, char* msg, int severity)
{
	lv_obj_t* msgbox = tt_obj_info_box_create(title, msg, severity);
	lv_timer_t* timer = lv_timer_create(login_msg_close_timer_cb,
			LOGIN_MSG_MS, msgbox);
	lv_timer_set_repeat_count(timer, 1);

	return msgbox;
}

static void login_msg_close_timer_cb(lv_timer_t* timer)
{
	lv_obj_t* msgbox = lv_timer_get_user_data(timer);

	if (msgbox != NULL && lv_obj_is_valid(msgbox)) {
		lv_msgbox_close(msgbox);
	}
}

static void login_success_msg_timer_cb(lv_timer_t* timer)
{
	lv_obj_t* msgbox = lv_timer_get_user_data(timer);

	if (msgbox != NULL && lv_obj_is_valid(msgbox)) {
		lv_msgbox_close(msgbox);
	}
	login_ok_this_boot = true;
	scr_splash_set_next_scr(menu_scr);
	lv_textarea_set_text(txt_password, "");
	lv_scr_load(menu_scr);
}

static void btn_login_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}

	const char* password = lv_textarea_get_text(txt_password);
	if (password == NULL || strlen(password) == 0) {
		login_msg_create("Login error", "Password cannot be empty", 1);
		return;
	}

	if (strcmp(password, LOGIN_PASSWORD) != 0) {
		lv_textarea_set_text(txt_password, "");
		login_msg_create("Login error", "Invalid password", 1);
		return;
	}

	backend_login_set_skip(skip_password, NULL, NULL);

	lv_obj_t* msgbox;
	if (skip_password) {
		msgbox = tt_obj_info_box_create("Login",
				"Login success. Password will not be requested again.", 0);
	} else {
		msgbox = tt_obj_info_box_create("Login", "Login success.", 0);
	}
	lv_timer_t* timer = lv_timer_create(login_success_msg_timer_cb,
			LOGIN_MSG_MS, msgbox);
	lv_timer_set_repeat_count(timer, 1);
}

lv_obj_t* scr_login_create(lv_obj_t* main_menu_scr)
{
	menu_scr = main_menu_scr;

	login_scr = lv_obj_create(NULL);
	lv_obj_set_size(login_scr, lv_disp_get_hor_res(NULL),
			lv_disp_get_ver_res(NULL));
	lv_obj_set_style_radius(login_scr, 0, 0);
	lv_obj_set_style_bg_color(login_scr, lv_color_hex(TT_COLOR_BG1), 0);
	lv_obj_set_scrollbar_mode(login_scr, LV_SCROLLBAR_MODE_OFF);

	lv_obj_t* cont = tt_obj_cont_create(login_scr);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(cont, 20, 0);
	lv_obj_set_style_pad_row(cont, 15, 0);

	if (screen_is_landscape()) {
		lv_obj_set_width(cont, LV_PCT(90));
		lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
	} else {
		lv_obj_set_size(cont, lv_disp_get_hor_res(NULL),
				lv_disp_get_ver_res(NULL));
		lv_obj_add_style(cont, &invisible_cont_style, LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(cont, lv_color_hex(TT_COLOR_BG1), 0);
		lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
		lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
	}

	tt_obj_label_color_create(cont, "Enter Password");

	txt_password = tt_obj_txt_create(cont, "Password", txt_password_cb);
	lv_textarea_set_password_mode(txt_password, false);
	lv_textarea_set_accepted_chars(txt_password, "0123456789");
	lv_textarea_set_max_length(txt_password, 6);
	lv_textarea_set_text(txt_password, "");

	cbx_skip_password = tt_obj_checkbox_create(cont,
			"Don't request password again", cbx_skip_password_cb);
	lv_obj_set_width(cbx_skip_password, LV_PCT(100));

	skip_password = false;
	lv_obj_clear_state(cbx_skip_password, LV_STATE_CHECKED);

	tt_obj_btn_create(cont, btn_login_cb, "LOG IN", NULL, LV_PCT(100), 50,
			LV_ALIGN_CENTER);

	return login_scr;
}

bool scr_login_is_required()
{
	app_state_snapshot_t snapshot;

	app_state_get_snapshot(&snapshot);
	return !login_ok_this_boot && !snapshot.login_config.skip_login;
}
