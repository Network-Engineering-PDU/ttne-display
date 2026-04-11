#include "lvgl/lvgl.h"
#include "scr_settings_nw_eth.h"
#include "tt_obj.h"

/* Ethernet mode handling */
static lv_obj_t* eth_mode_combo;
static lv_obj_t* eth_config_combo;
static lv_obj_t* eth_ip_txt;
static lv_obj_t* eth_mask_txt;
static lv_obj_t* eth_gw_txt;
static lv_obj_t* eth_ssid_txt;
static lv_obj_t* eth_pass_txt;
static lv_obj_t* eth_ssid_lbl;
static lv_obj_t* eth_pass_lbl;
static lv_obj_t* eth_config_cont;

static void eth_mode_changed(lv_event_t* e)
{
    uint16_t selected = lv_dropdown_get_selected(eth_mode_combo);
    
    /* Update UI based on selected mode */
    switch(selected) {
        case 0: /* Single LAN */
            lv_obj_add_flag(eth_ssid_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(eth_ssid_txt, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(eth_pass_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(eth_pass_txt, LV_OBJ_FLAG_HIDDEN);
            break;
        case 1: /* WIFI */
            lv_obj_clear_flag(eth_ssid_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(eth_ssid_txt, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(eth_pass_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(eth_pass_txt, LV_OBJ_FLAG_HIDDEN);
            break;
        case 2: /* Dual LAN */
            lv_obj_add_flag(eth_ssid_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(eth_ssid_txt, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(eth_pass_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(eth_pass_txt, LV_OBJ_FLAG_HIDDEN);
            break;
        case 3: /* LAN & WIFI */
            lv_obj_clear_flag(eth_ssid_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(eth_ssid_txt, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(eth_pass_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(eth_pass_txt, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            break;
    }
}

void scr_settings_nw_eth_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* eth_page = tt_obj_menu_page_create(menu, btn, NULL, "Ethernet");
    lv_obj_t* cont = tt_obj_cont_create(eth_page);

    /* Ethernet mode selection */
    tt_obj_label_create(cont, "Ethernet mode");
    eth_mode_combo = tt_obj_dropdown_create(cont, "Single LAN\nWIFI\nDual LAN\nLAN & WIFI", eth_mode_changed);
    lv_dropdown_set_selected(eth_mode_combo, 0);

    /* Configuration type */
    tt_obj_label_create(cont, "Configure");
    eth_config_combo = tt_obj_dropdown_create(cont, "Manual\nDHCP", NULL);
    lv_dropdown_set_selected(eth_config_combo, 0);

    /* IP Address */
    tt_obj_label_create(cont, "I.P. address");
    eth_ip_txt = tt_obj_txt_create(cont, "I.P. address", NULL);

    /* Subnet Mask */
    tt_obj_label_create(cont, "Mask");
    eth_mask_txt = tt_obj_txt_create(cont, "Mask", NULL);

    /* Gateway */
    tt_obj_label_create(cont, "Gateway");
    eth_gw_txt = tt_obj_txt_create(cont, "Gateway", NULL);

    /* WIFI SSID (hidden by default for Single LAN mode) */
    eth_ssid_lbl = tt_obj_label_create(cont, "SSID");
    eth_ssid_txt = tt_obj_txt_create(cont, "SSID", NULL);
    lv_obj_add_flag(eth_ssid_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(eth_ssid_txt, LV_OBJ_FLAG_HIDDEN);

    /* WIFI Password (hidden by default for Single LAN mode) */
    eth_pass_lbl = tt_obj_label_create(cont, "Password");
    eth_pass_txt = tt_obj_txt_create(cont, "Password", NULL);
    lv_obj_add_flag(eth_pass_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(eth_pass_txt, LV_OBJ_FLAG_HIDDEN);

    /* OK / Cancel buttons */
    lv_obj_t* btn_row = lv_obj_create(cont);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    tt_obj_btn_std_create(btn_row, NULL, "OK");
    tt_obj_btn_std_create(btn_row, NULL, "Cancel");
}
