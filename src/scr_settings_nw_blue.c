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

/* Global variables ***********************************************************/
static lv_obj_t* btn_bt_state;
static lv_obj_t* menu_handle;
static bool is_page_active = false;
static uint16_t selected_state = 0; // 0 = ON, 1 = OFF

/* Function prototypes ********************************************************/
static void menu_cb(lv_event_t* e);
static void btn_bt_state_cb(lv_event_t* e);
static void btn_bt_ok_cb(lv_event_t* e);
static void btn_bt_cancel_cb(lv_event_t* e);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* menu = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* target_page = lv_event_get_user_data(e);
        lv_obj_t* active_page = lv_menu_get_cur_main_page(menu);
        
        if (target_page == active_page) {
            // Refresh data from model when page opens
            controller_get_nw_services();
            const models_nw_services_t* nw_services = models_get_nw_services();
            
            // Set button state: 0 for ON, 1 for OFF
            selected_state = nw_services->bluetooth ? 0 : 1;
            lv_label_set_text(lv_obj_get_child(btn_bt_state, 0), nw_services->bluetooth ? "ON" : "OFF");

            if (!is_page_active) {
                is_page_active = true;
                LV_LOG_USER("Bluetooth Settings Opened");
            }
        }
    } else if (code == LV_EVENT_DELETE) {
        is_page_active = false;
    }
}

static void btn_bt_state_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // Toggle state: 0 = ON, 1 = OFF
        selected_state = selected_state == 0 ? 1 : 0;
        lv_label_set_text(lv_obj_get_child(btn_bt_state, 0), selected_state == 0 ? "ON" : "OFF");
    }
}

static void btn_bt_ok_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // 0 = ON, 1 = OFF
        
        if (selected_state == 0) {
            controller_post_start_bluetooth();
            LV_LOG_USER("Action: Bluetooth Enabled");
        } else {
            controller_post_stop_bluetooth();
            LV_LOG_USER("Action: Bluetooth Disabled");
        }

        // Update the local model state immediately for UI consistency
        models_nw_services_t* nw_services = (models_nw_services_t*)models_get_nw_services();
        if (nw_services) {
            nw_services->bluetooth = (selected_state == 0) ? true : false;
        }

        // Navigate back
        lv_obj_t* back_btn = lv_menu_get_main_header_back_btn(menu_handle);
        lv_event_send(back_btn, LV_EVENT_CLICKED, NULL);
    }
}

static void btn_bt_cancel_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // Just go back without calling any controller functions
        lv_obj_t* back_btn = lv_menu_get_main_header_back_btn(menu_handle);
        lv_event_send(back_btn, LV_EVENT_CLICKED, NULL);
    }
}

/* Creation Function **********************************************************/

void scr_settings_nw_blue_create(lv_obj_t* menu_param, lv_obj_t* btn)
{
    menu_handle = menu_param;

    // Create the menu page
    lv_obj_t* bt_cont = tt_obj_menu_page_create(menu_handle, btn, menu_cb, "Bluetooth");
    
    // 1. Label and Dropdown Container (Flex Row)
    lv_obj_t* state_cont = lv_obj_create(bt_cont);
    lv_obj_set_size(state_cont, LV_PCT(100), 60);
    lv_obj_set_flex_flow(state_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(state_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(state_cont, 20, 0);
    lv_obj_set_style_bg_opa(state_cont, 0, 0);
    lv_obj_set_style_border_opa(state_cont, 0, 0);

    // Label on the left
    lv_obj_t* label = tt_obj_label_create(state_cont, "Bluetooth state");
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Button on the right
    btn_bt_state = tt_obj_btn_create(state_cont, btn_bt_state_cb, "ON", 
                                      NULL, 80, 45, LV_ALIGN_CENTER);
    lv_obj_set_style_border_width(btn_bt_state, 2, 0);
    lv_obj_set_style_border_color(btn_bt_state, lv_color_white(), 0);

    // 2. Action Button Container (Flex Row)
    lv_obj_t* actions_cont = lv_obj_create(bt_cont);
    lv_obj_set_size(actions_cont, LV_PCT(100), 80);
    lv_obj_set_flex_flow(actions_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(actions_cont, 15, 0);
    lv_obj_set_style_bg_opa(actions_cont, 0, 0); // Transparent background
    lv_obj_set_style_border_opa(actions_cont, 0, 0);

    // 3. OK & Cancel Buttons
    tt_obj_btn_create(actions_cont, btn_bt_ok_cb, "OK", 
                      NULL, LV_PCT(40), 45, LV_ALIGN_CENTER);
                      
    tt_obj_btn_create(actions_cont, btn_bt_cancel_cb, "Cancel", 
                      NULL, LV_PCT(40), 45, LV_ALIGN_CENTER);
}