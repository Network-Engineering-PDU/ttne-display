/**
 * @file scr_settings_bluetooth.c
 * @brief Bluetooth settings screen with ON/OFF switch and navigation.
 */

#include <stdio.h>
#include "lvgl/lvgl.h"
#include "scr_settings_nw_blue.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"
#include "config.h"
#include "screen.h"
#include "ttne_display.h"
#include "runbg.h"

/* --- Framework Constants --- */
#define BTN_WIDTH  80
#define BTN_HEIGHT 40

/* --- Global/Static Variables --- */
static lv_obj_t* menu;

/* --- Event Callbacks --- */

/**
 * @brief Handles the OK button click (Saves settings and closes)
 */
static void ok_event_cb(lv_event_t * e) {
    lv_obj_t * sw = (lv_obj_t *)lv_event_get_user_data(e);
    bool is_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    
    printf("Bluetooth state saved: %s\n", is_enabled ? "ON" : "OFF");
    
    // Logic to return to previous menu
    // lv_menu_set_sidebar_page(menu, NULL); 
}

/**
 * @brief Handles the Cancel button click (Discards changes)
 */
static void cancel_event_cb(lv_event_t * e) {
    printf("Bluetooth setup cancelled.\n");
    // Logic to return to previous menu
}

/* --- Main Screen Creation Function --- */

void scr_settings_nw_blue_create(lv_obj_t* l_menu, lv_obj_t* btn)
{
    menu = l_menu;

    // 1. Create the Bluetooth Page
    // tt_obj_menu_page_create likely handles the "< Bluetooth" header internally
    lv_obj_t* bt_page = tt_obj_menu_page_create(menu, btn, NULL, "Bluetooth");
    
    // Set page background to black
    lv_obj_set_style_bg_color(bt_page, lv_color_black(), 0);

    // 2. Middle Row Container (Label and Switch)
    lv_obj_t* row_cont = lv_obj_create(bt_page);
    lv_obj_set_size(row_cont, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_center(row_cont);
    
    // Remove default container styling to keep it "floating" and black
    lv_obj_set_style_bg_opa(row_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(row_cont, 0, 0);

    // Layout: Horizontal row, centered vertically
    lv_obj_set_flex_flow(row_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 3. The "Bluetooth enable" Label
    lv_obj_t* lbl_bt = tt_obj_label_create(row_cont, "Bluetooth enable");
    lv_obj_set_style_text_color(lbl_bt, lv_color_white(), 0);
    lv_obj_set_flex_grow(lbl_bt, 1); // Pushes the switch to the right edge

    // 4. The ON/OFF Switch
    lv_obj_t* sw_bt = lv_switch_create(row_cont);
    // Optional: Set default state (e.g., ON)
    // lv_obj_add_state(sw_bt, LV_STATE_CHECKED);

    // 5. Bottom Navigation Buttons Container (OK, Cancel)
    lv_obj_t* footer = lv_obj_create(bt_page);
    lv_obj_set_size(footer, LV_PCT(100), 60);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -10); 
    
    // Make footer invisible
    lv_obj_set_style_bg_opa(footer, LV_OPA_0, 0);
    lv_obj_set_style_border_width(footer, 0, 0);

    // Layout: Align buttons to the bottom-right
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(footer, 10, 0); // Gap between OK and Cancel

    // 6. Create OK and Cancel Buttons using your framework
    lv_obj_t* btn_ok = tt_obj_btn_create(footer, "OK", BTN_WIDTH, BTN_HEIGHT);
    lv_obj_t* btn_cancel = tt_obj_btn_create(footer, "Cancel", BTN_WIDTH, BTN_HEIGHT);

    // 7. Assign Events
    // Passing sw_bt as user_data to OK so we can read the switch state on click
    lv_obj_add_event_cb(btn_ok, ok_event_cb, LV_EVENT_CLICKED, sw_bt);
    lv_obj_add_event_cb(btn_cancel, cancel_event_cb, LV_EVENT_CLICKED, NULL);
}