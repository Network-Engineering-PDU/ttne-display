#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "lvgl/lvgl.h"

#include "scr_settings_nw.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"

/* Function Prototypes for Network Sub-pages */
static void menu_nw_cb(lv_event_t* e);
static void btn_nw_cb(lv_event_t* e);

/* Protocol sub-page creation functions */
static void scr_settings_eth_create(lv_obj_t* menu, lv_obj_t* btn);
static void scr_settings_snmp_create(lv_obj_t* menu, lv_obj_t* btn);
static void scr_settings_modbus_create(lv_obj_t* menu, lv_obj_t* btn);
static void scr_settings_ssh_create(lv_obj_t* menu, lv_obj_t* btn);
static void scr_settings_bluetooth_create(lv_obj_t* menu, lv_obj_t* btn);
static void scr_settings_ntp_create(lv_obj_t* menu, lv_obj_t* btn);

/**
 * @brief Creates the main Networks grid menu
 */
void scr_settings_nw_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* nw_cont = tt_obj_menu_page_create(menu, btn, menu_nw_cb, "Networks");

    lv_obj_t* nw_page = tt_obj_menu_page_create(menu, btn, NULL, "Networks");

    lv_obj_t* cont = tt_obj_cont_create(nw_page);

    /* Layout: center grid like Page 2 */
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(
        cont,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    /* Network protocol buttons */
    lv_obj_t* btn_ethernet = tt_obj_btn_mtx_create(
        cont, btn_nw_cb, "Ethernet", (void*)(uintptr_t)0
    );

    lv_obj_t* btn_snmp = tt_obj_btn_mtx_create(
        cont, btn_nw_cb, "SNMP", (void*)(uintptr_t)1
    );

    lv_obj_t* btn_modbus = tt_obj_btn_mtx_create(
        cont, btn_nw_cb, "Modbus", (void*)(uintptr_t)2
    );

    lv_obj_t* btn_ssh = tt_obj_btn_mtx_create(
        cont, btn_nw_cb, "SSH", (void*)(uintptr_t)3
    );

    lv_obj_t* btn_bluetooth = tt_obj_btn_mtx_create(
        cont, btn_nw_cb, "Bluetooth", (void*)(uintptr_t)4
    );

    lv_obj_t* btn_ntp = tt_obj_btn_mtx_create(
        cont, btn_nw_cb, "NTP - SNTP", (void*)(uintptr_t)5
    );

    /* Navigation (will be implemented step by step) */
    scr_settings_eth_create(menu, btn_ethernet);
    scr_settings_snmp_create(menu, btn_snmp);
    scr_settings_modbus_create(menu, btn_modbus);
    scr_settings_ssh_create(menu, btn_ssh);
    scr_settings_bluetooth_create(menu, btn_bluetooth);
    scr_settings_ntp_create(menu, btn_ntp);
}

/**
 * @brief Menu callback for Networks page
 */
static void menu_nw_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_VALUE_CHANGED) {
        LV_LOG_USER("Networks menu opened");
    }
}

/**
 * @brief Callback for network menu buttons
 */
static void btn_nw_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int btn_id = (int)(uintptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        /* Logic to navigate to specific network sub-pages */
        switch(btn_id) {
            case 0: /* Open Ethernet Settings */
                LV_LOG_USER("Ethernet configuration selected");
                break;
            case 1: /* Open SNMP Settings */
                LV_LOG_USER("SNMP configuration selected");
                break;
            case 2: /* Open Modbus Settings */
                LV_LOG_USER("Modbus configuration selected");
                break;
            case 3: /* Open SSH Settings */
                LV_LOG_USER("SSH configuration selected");
                break;
            case 4: /* Open Bluetooth Settings */
                LV_LOG_USER("Bluetooth configuration selected");
                break;
            case 5: /* Open NTP Settings */
                LV_LOG_USER("NTP/SNTP configuration selected");
                break;
            default: break;
        }
    }
}

/* Network Protocol Sub-page Functions ******************************************/

/**
 * @brief Creates Ethernet configuration sub-page
 */
static void scr_settings_eth_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* eth_page = tt_obj_menu_page_create(menu, btn, NULL, "Ethernet");
    lv_obj_t* cont = tt_obj_cont_create(eth_page);
    
    tt_obj_label_create(cont, "Ethernet Settings");
    tt_obj_label_create(cont, "Configure Ethernet interface");
}

/**
 * @brief Creates SNMP configuration sub-page
 */
static void scr_settings_snmp_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* snmp_page = tt_obj_menu_page_create(menu, btn, NULL, "SNMP");
    lv_obj_t* cont = tt_obj_cont_create(snmp_page);
    
    tt_obj_label_create(cont, "SNMP Settings");
    tt_obj_label_create(cont, "Configure SNMP service");
}

/**
 * @brief Creates Modbus configuration sub-page
 */
static void scr_settings_modbus_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* modbus_page = tt_obj_menu_page_create(menu, btn, NULL, "Modbus");
    lv_obj_t* cont = tt_obj_cont_create(modbus_page);
    
    tt_obj_label_create(cont, "Modbus Settings");
    tt_obj_label_create(cont, "Configure Modbus service");
}

/**
 * @brief Creates SSH configuration sub-page
 */
static void scr_settings_ssh_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* ssh_page = tt_obj_menu_page_create(menu, btn, NULL, "SSH");
    lv_obj_t* cont = tt_obj_cont_create(ssh_page);
    
    tt_obj_label_create(cont, "SSH Settings");
    tt_obj_label_create(cont, "Configure SSH service");
}

/**
 * @brief Creates Bluetooth configuration sub-page
 */
static void scr_settings_bluetooth_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* bt_page = tt_obj_menu_page_create(menu, btn, NULL, "Bluetooth");
    lv_obj_t* cont = tt_obj_cont_create(bt_page);
    
    tt_obj_label_create(cont, "Bluetooth Settings");
    tt_obj_label_create(cont, "Configure Bluetooth interface");
}

/**
 * @brief Creates NTP/SNTP configuration sub-page
 */
static void scr_settings_ntp_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* ntp_page = tt_obj_menu_page_create(menu, btn, NULL, "NTP - SNTP");
    lv_obj_t* cont = tt_obj_cont_create(ntp_page);
    
    tt_obj_label_create(cont, "NTP/SNTP Settings");
    tt_obj_label_create(cont, "Configure time synchronization");
}
