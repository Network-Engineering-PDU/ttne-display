#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"

#include "scr_settings_vis.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"
#include "config.h"
#include "screen.h"

#define TIMER_ROT 5000 // ms

extern void reset_program();

/* Global variables ***********************************************************/

static lv_obj_t* dd_rotation;
static lv_obj_t* txt_screen_saver;
static lv_timer_t* timer_rot;
static lv_obj_t* msg_box_rot;

/* PDU location information pointers */
static lv_obj_t* txt_fields[7]; 

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void rotate_cb(lv_event_t* e);
static void txt_inactivity_cb(lv_event_t* e);
static void txt_pdu_info_cb(lv_event_t* e);
static void update_pdu_info_display();
static void save_pdu_info_field(int field_id, const char* value);

static void revert_rot();
static void msg_box_rot_cb(lv_event_t* e);
static void timer_rot_cb(lv_timer_t* timer);
static lv_obj_t* create_setting_row(lv_obj_t* parent, const char* label_text);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* menu = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* curr_page = lv_event_get_user_data(e);
        lv_obj_t* page = lv_menu_get_cur_main_page(menu);
        if (curr_page == page) {
            // Update rotation
            int rotation = config_get_rotation();
            lv_dropdown_set_selected(dd_rotation, rotation);
            
            // Update Screen Saver
            char inactivity_time_str[10];
            sprintf(inactivity_time_str, "%d", config_get_inactivity_time());
            lv_textarea_set_text(txt_screen_saver, inactivity_time_str);
            
            // Update PDU info fields
            update_pdu_info_display();
        }
    }
}

static void rotate_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        uint16_t rotation = lv_dropdown_get_selected(dd_rotation);
        screen_set_rotation(rotation);
        
        timer_rot = lv_timer_create(timer_rot_cb, TIMER_ROT, NULL);
        
        char msg[300];
        sprintf(msg, "Are you sure you want to save screen rotation?\n"
                "Rotation: " TT_COLOR_GREEN_NE_STR " %d deg#\n"
                "(changes will be reverted in 5 seconds)",
                rotation * 90); // Assuming 0,1,2,3 map to 0,90,180,270
                
        msg_box_rot = tt_obj_msg_box_create("Screen rotation", msg, NULL, msg_box_rot_cb);
    }
}

static void txt_inactivity_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_NUM);
        lv_scr_load(kb_scr);
    }
    if (code == LV_EVENT_READY) {
        int t = atoi(lv_textarea_get_text(obj));
        if (t < 1 || t > 300) {
            tt_obj_info_box_create("ERROR", "Time must be [1-300]", 1);
            return;
        }
        config_set_inactivity_time(t);
    }
}

static void txt_pdu_info_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    int field_id = (int)(uintptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_ABC);
        lv_scr_load(kb_scr);
    }
    if (code == LV_EVENT_READY) {
        save_pdu_info_field(field_id, lv_textarea_get_text(obj));
    }
}

static void timer_rot_cb(lv_timer_t* timer)
{
    lv_msgbox_close(msg_box_rot);
    lv_timer_del(timer);
    revert_rot();
}

static void msg_box_rot_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_timer_del(timer_rot);
        if (lv_msgbox_get_active_btn(obj) == 0) { // YES
            config_set_rotation(lv_dropdown_get_selected(dd_rotation));
            reset_program();
        } else {
            revert_rot();
        }
        lv_msgbox_close(obj);
    }
}

static void revert_rot()
{
    int rotation = config_get_rotation();
    screen_set_rotation(rotation);
    lv_dropdown_set_selected(dd_rotation, rotation);
}

/* Helper Logic ***************************************************************/

static void update_pdu_info_display()
{
    // These calls assume getters exist in your config.h/controller.h
    lv_textarea_set_text(txt_fields[0], config_get_pdu_company());
    lv_textarea_set_text(txt_fields[1], config_get_pdu_rack());
    lv_textarea_set_text(txt_fields[2], config_get_pdu_system());
    lv_textarea_set_text(txt_fields[3], config_get_pdu_ups());
    lv_textarea_set_text(txt_fields[4], config_get_pdu_elec_board());
    lv_textarea_set_text(txt_fields[5], config_get_pdu_breaker());
    lv_textarea_set_text(txt_fields[6], config_get_pdu_service());
}

static void save_pdu_info_field(int field_id, const char* value)
{
    switch(field_id) {
        case 0: config_set_pdu_company(value); break;
        case 1: config_set_pdu_rack(value); break;
        case 2: config_set_pdu_system(value); break;
        case 3: config_set_pdu_ups(value); break;
        case 4: config_set_pdu_elec_board(value); break;
        case 5: config_set_pdu_breaker(value); break;
        case 6: config_set_pdu_service(value); break;
    }
}

/* Layout Helper **************************************************************/

static lv_obj_t* create_setting_row(lv_obj_t* parent, const char* label_text)
{
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    
    // Transparent background for the row container
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_opa(row, 0, 0);
    lv_obj_set_style_pad_all(row, 5, 0);

    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl = tt_obj_label_create(row, label_text);
    lv_obj_set_flex_grow(lbl, 1); // Push the input to the right

    return row;
}

/* Main Page Creation *********************************************************/

void scr_settings_vis_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* vis_page = tt_obj_menu_page_create(menu, btn, menu_cb, "Visualization");
    
    // Enable scrollbar as seen in mockup
    lv_obj_set_scrollbar_mode(vis_page, LV_SCROLLBAR_MODE_ON);

    lv_obj_t* cont = tt_obj_cont_create(vis_page);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(cont, 5, 0);

    // 1. Screen Rotation Row
    lv_obj_t* row_rot = create_setting_row(cont, "Screen rotation");
    dd_rotation = tt_obj_dropdown_create(row_rot, "0 deg\n90 deg\n180 deg\n270 deg", rotate_cb);
    lv_obj_set_width(dd_rotation, 90); //180

    // 2. Screen Saver Row
    lv_obj_t* row_saver = create_setting_row(cont, "Screen saver (s)");
    txt_screen_saver = tt_obj_txt_create(row_saver, "0", txt_inactivity_cb);
    lv_obj_set_width(txt_screen_saver, 90); //180

    // 3. Section Header
    lv_obj_t* pdu_header = tt_obj_label_create(cont, "PDU location information");
    lv_obj_set_style_text_font(pdu_header, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_top(pdu_header, 15, 0);
    lv_obj_set_style_pad_left(pdu_header, 10, 0);

    // 4. PDU Fields Loop
    const char* pdu_labels[] = {
        "Company name", "Rack", "System A - B", 
        "UPS A - B", "Electrical board", "Circuit Breaker", "Service"
    };

    for (int i = 0; i < 7; i++) {
        lv_obj_t* row = create_setting_row(cont, pdu_labels[i]);
        txt_fields[i] = tt_obj_txt_create(row, "Text", txt_pdu_info_cb);
        lv_obj_set_width(txt_fields[i], 120); //180
        
        // Attach field ID to the event
        lv_obj_add_event_cb(txt_fields[i], txt_pdu_info_cb, LV_EVENT_ALL, (void*)(uintptr_t)i);
    }

    update_pdu_info_display();
}