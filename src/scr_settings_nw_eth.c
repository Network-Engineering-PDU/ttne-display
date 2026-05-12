#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "scr_settings_nw_eth.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"

/* Mode Definitions matching your table */
typedef enum {
    MODE_WIFI = 0,
    MODE_SINGLE_LAN,
    MODE_DUAL_LAN,
    MODE_LAN_WIFI
} nw_mode_t;

/* Global UI Pointers */
static lv_obj_t *dd_mode;
static lv_obj_t *dd_config;
static lv_obj_t *cont_left, *cont_right;

/* Shared Fields */
static lv_obj_t *txt_ip, *txt_mask, *txt_gw, *txt_dns;
static lv_obj_t *txt_wifi_ssid, *txt_wifi_pass;
static lv_obj_t *txt_lan2_ip, *txt_lan2_mask;

/* Labels for visibility toggling */
static lv_obj_t *lbl_ip, *lbl_mask, *lbl_gw, *lbl_dns, *lbl_ssid, *lbl_pass, *lbl_lan2_ip, *lbl_lan2_mask;

/* --- Logic Functions --- */

static void update_visibility() {
    nw_mode_t mode = (nw_mode_t)lv_dropdown_get_selected(dd_mode);
    bool is_dhcp = (lv_dropdown_get_selected(dd_config) == 0);

    /* Reset everything to hidden */
    lv_obj_add_flag(cont_right, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(txt_ip, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(txt_mask, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(lbl_mask, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(txt_gw, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(lbl_gw, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(txt_wifi_ssid, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(lbl_ssid, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(txt_wifi_pass, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(lbl_pass, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(txt_lan2_ip, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(lbl_lan2_ip, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(txt_lan2_mask, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(lbl_lan2_mask, LV_OBJ_FLAG_HIDDEN);

    switch(mode) {
        case MODE_WIFI:
            lv_obj_clear_flag(lbl_ssid, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(txt_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(lbl_pass, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(txt_wifi_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(lbl_gw, LV_OBJ_FLAG_HIDDEN);   lv_obj_clear_flag(txt_gw, LV_OBJ_FLAG_HIDDEN);
            break;

        case MODE_SINGLE_LAN:
            if(!is_dhcp) {
                lv_obj_clear_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);   lv_obj_clear_flag(txt_ip, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lbl_mask, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(txt_mask, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lbl_gw, LV_OBJ_FLAG_HIDDEN);   lv_obj_clear_flag(txt_gw, LV_OBJ_FLAG_HIDDEN);
            }
            break;

        case MODE_DUAL_LAN:
            lv_obj_clear_flag(cont_right, LV_OBJ_FLAG_HIDDEN);
            if(!is_dhcp) {
                /* LAN 1 (Left) */
                lv_obj_clear_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);   lv_obj_clear_flag(txt_ip, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lbl_mask, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(txt_mask, LV_OBJ_FLAG_HIDDEN);
                /* LAN 2 (Right) */
                lv_obj_clear_flag(lbl_lan2_ip, LV_OBJ_FLAG_HIDDEN);   lv_obj_clear_flag(txt_lan2_ip, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lbl_lan2_mask, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(txt_lan2_mask, LV_OBJ_FLAG_HIDDEN);
                /* Shared Gateway */
                lv_obj_clear_flag(lbl_gw, LV_OBJ_FLAG_HIDDEN);   lv_obj_clear_flag(txt_gw, LV_OBJ_FLAG_HIDDEN);
            }
            break;

        case MODE_LAN_WIFI:
            lv_obj_clear_flag(cont_right, LV_OBJ_FLAG_HIDDEN);
            /* LAN (Left) */
            if(!is_dhcp) {
                lv_obj_clear_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);   lv_obj_clear_flag(txt_ip, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lbl_mask, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(txt_mask, LV_OBJ_FLAG_HIDDEN);
            }
            /* WIFI (Right) */
            lv_obj_clear_flag(lbl_ssid, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(txt_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(lbl_pass, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(txt_wifi_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(lbl_gw, LV_OBJ_FLAG_HIDDEN);   lv_obj_clear_flag(txt_gw, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}

static void update_cb(lv_event_t* e) { update_visibility(); }

/* --- Screen Creation --- */

void scr_settings_nw_eth_create(lv_obj_t* menu, lv_obj_t* btn) {
    /* Create Page with Correct Header Title */
    lv_obj_t* nw_cont = tt_obj_menu_page_create(menu, btn, NULL, "< Ethernet");
    lv_obj_t* main_cont = tt_obj_cont_create(nw_cont);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);

    /* 1. Top Section: Mode and Config Dropdowns */
    tt_obj_label_create(main_cont, "Ethernet mode");
    dd_mode = tt_obj_dropdown_create(main_cont, "WIFI\nSingle LAN\nDual LAN\nLAN & WIFI", update_cb);

    tt_obj_label_create(main_cont, "Configure");
    dd_config = tt_obj_dropdown_create(main_cont, "DHCP\nManual", update_cb);

    /* 2. Middle Section: Dual Column Layout for Fields */
    lv_obj_t* row_cont = lv_obj_create(main_cont);
    lv_obj_set_size(row_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_cont, 0, 0);
    lv_obj_set_style_border_opa(row_cont, 0, 0);
    lv_obj_set_flex_flow(row_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* Create Left Column */
    cont_left = lv_obj_create(row_cont);
    lv_obj_set_flex_grow(cont_left, 1);
    lv_obj_set_flex_flow(cont_left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(cont_left, 0, 0);

    /* Create Right Column */
    cont_right = lv_obj_create(row_cont);
    lv_obj_set_flex_grow(cont_right, 1);
    lv_obj_set_flex_flow(cont_right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(cont_right, 0, 0);

    /* Left Column Fields */
    lbl_ssid = tt_obj_label_create(cont_left, "SSID");
    txt_wifi_ssid = tt_obj_txt_create(cont_left, "SSID", NULL);
    
    lbl_pass = tt_obj_label_create(cont_left, "Password");
    txt_wifi_pass = tt_obj_txt_create(cont_left, "Password", NULL);
    lv_textarea_set_password_mode(txt_wifi_pass, true);

    lbl_ip = tt_obj_label_create(cont_left, "I.P. address");
    txt_ip = tt_obj_txt_create(cont_left, "0.0.0.0", NULL);

    lbl_mask = tt_obj_label_create(cont_left, "Mask");
    txt_mask = tt_obj_txt_create(cont_left, "255.255.255.0", NULL);

    /* Right Column Fields (used for Dual LAN / LAN+WiFi) */
    lbl_lan2_ip = tt_obj_label_create(cont_right, "LAN2 I.P.");
    txt_lan2_ip = tt_obj_txt_create(cont_right, "0.0.0.0", NULL);

    lbl_lan2_mask = tt_obj_label_create(cont_right, "LAN2 Mask");
    txt_lan2_mask = tt_obj_txt_create(cont_right, "255.255.255.0", NULL);

    /* Bottom Shared Fields (Back to Main Container) */
    lbl_gw = tt_obj_label_create(main_cont, "Gateway");
    txt_gw = tt_obj_txt_create(main_cont, "0.0.0.0", NULL);

    /* 3. Action Buttons Section */
    lv_obj_t* btn_row = lv_obj_create(main_cont);
    lv_obj_set_size(btn_row, LV_PCT(100), 60);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(btn_row, 0, 0);
    lv_obj_set_style_border_opa(btn_row, 0, 0);

    tt_obj_btn_std_create(btn_row, NULL, "OK");
    tt_obj_btn_std_create(btn_row, NULL, "Cancel");

    /* Initial state */
    update_visibility();
}