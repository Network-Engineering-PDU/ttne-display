#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "lvgl/lvgl.h"

#include "scr_settings_nw.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"

/* Function Prototypes for Network Sub-pages */
static void btn_nw_cb(lv_event_t* e);

/**
 * @brief Creates the main Networks grid menu
 */
void scr_settings_nw_create(lv_obj_t* menu, lv_obj_t* btn)
{
    /* Create the main page and bind it to the menu button */
    lv_obj_t* nw_page = tt_obj_menu_page_create(menu, btn, NULL, "Networks");

    /* Create a container with a 3-column flex grid */
    lv_obj_t* cont = tt_obj_cont_create(nw_page);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(cont, 10, 0);

    /* Button labels based on mockup */
    const char* nw_labels[] = {
        "Ethernet", "SNMP", "Modbus",
        "SSH", "Bluetooth", "NTP - SNTP"
    };

    /* Create the 6 rounded square buttons */
    for (int i = 0; i < 6; i++) {
        lv_obj_t* btn_nw = tt_obj_btn_mtx_create(cont, btn_nw_cb, nw_labels[i], (void*)(uintptr_t)i);
        
        /* Size them to fit 3 per row (approx 30% width) */
        lv_obj_set_size(btn_nw, LV_PCT(30), 120); 
        lv_obj_set_style_radius(btn_nw, 15, 0);
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