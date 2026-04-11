#include "lvgl/lvgl.h"
#include "scr_settings_nw_snmp.h"
#include "tt_obj.h"

/* SNMP configuration variables */
static lv_obj_t* snmp_enable_cbx;
static lv_obj_t* snmp_version_combo;
static lv_obj_t* set_enable_cbx;
static lv_obj_t* community_txt;
static lv_obj_t* traps_enable_cbx;
static lv_obj_t* trap_ip_txts[4];

static void snmp_enable_changed(lv_event_t* e)
{
    bool enabled = lv_obj_get_state(snmp_enable_cbx) & LV_STATE_CHECKED;
    lv_obj_set_enabled(snmp_version_combo, enabled);
    lv_obj_set_enabled(set_enable_cbx, enabled);
    lv_obj_set_enabled(community_txt, enabled);
    lv_obj_set_enabled(traps_enable_cbx, enabled);
}

void scr_settings_nw_snmp_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* snmp_page = tt_obj_menu_page_create(menu, btn, NULL, "SNMP");
    lv_obj_t* cont = tt_obj_cont_create(snmp_page);

    /* SNMP Enable */
    tt_obj_label_create(cont, "SNMP enable");
    snmp_enable_cbx = tt_obj_checkbox_create(cont, "", snmp_enable_changed);

    /* SNMP Version */
    tt_obj_label_create(cont, "Version");
    snmp_version_combo = tt_obj_dropdown_create(cont, "SNMP v1\nSNMP v2c\nSNMP v3", NULL);
    lv_dropdown_set_selected(snmp_version_combo, 0);

    /* SET Enable */
    tt_obj_label_create(cont, "SET enable");
    set_enable_cbx = tt_obj_checkbox_create(cont, "", NULL);

    /* Community String */
    tt_obj_label_create(cont, "Community");
    community_txt = tt_obj_txt_create(cont, "Community", NULL);

    /* Traps Available */
    tt_obj_label_create(cont, "Traps available");
    traps_enable_cbx = tt_obj_checkbox_create(cont, "", NULL);

    /* Trap IP addresses */
    for (int i = 0; i < 4; i++) {
        trap_ip_txts[i] = tt_obj_txt_create(cont, "IP / DNS Traps", NULL);
    }

    /* OK / Cancel buttons */
    lv_obj_t* btn_row = lv_obj_create(cont);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    tt_obj_btn_std_create(btn_row, NULL, "OK");
    tt_obj_btn_std_create(btn_row, NULL, "Cancel");
}
