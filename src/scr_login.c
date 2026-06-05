#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "scr_login.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"

static lv_obj_t* txt_password;
static lv_obj_t* cbx_skip_password;
static bool skip_password = false;

static void menu_cb(lv_event_t* e);
static void txt_password_cb(lv_event_t* e);
static void cbx_skip_password_cb(lv_event_t* e);
static void btn_login_cb(lv_event_t* e);

static void menu_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* curr_page = lv_event_get_user_data(e);
        lv_obj_t* page = lv_menu_get_cur_main_page(obj);
        if (curr_page == page) {
            LV_LOG_USER("Login page active");
            lv_textarea_set_text(txt_password, "");
            if (skip_password) {
                lv_obj_add_state(cbx_skip_password, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(cbx_skip_password, LV_STATE_CHECKED);
            }
        }
    }
}

static void txt_password_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_ABC);
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

static void btn_login_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    const char* password = lv_textarea_get_text(txt_password);
    if (password == NULL || strlen(password) == 0) {
        tt_obj_info_box_create("Login error", "Password cannot be empty", 1);
        return;
    }

    if (skip_password) {
        tt_obj_info_box_create("Login", "Logged in. Password will not be requested again.", 0);
    } else {
        tt_obj_info_box_create("Login", "Logged in successfully.", 0);
    }

    lv_textarea_set_text(txt_password, "");
}

void scr_login_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* login_page = tt_obj_menu_page_create(menu, btn, menu_cb, "Login");
    lv_obj_t* cont = tt_obj_cont_create(login_page);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont, 20, 0);
    lv_obj_set_style_pad_row(cont, 15, 0);

    tt_obj_label_color_create(cont, "Enter Password");

    txt_password = tt_obj_txt_create(cont, "Password", txt_password_cb);
    lv_textarea_set_password_mode(txt_password, true);
    lv_textarea_set_text(txt_password, "");

    cbx_skip_password = tt_obj_checkbox_create(cont, "Don't request password again", cbx_skip_password_cb);
    lv_obj_set_width(cbx_skip_password, LV_PCT(100));

    lv_obj_t* row = tt_obj_cont_create(cont);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_width(row, LV_PCT(100));

    tt_obj_btn_mtx_create(row, btn_login_cb, "LOG IN", ASSET("menu.png"));
}
