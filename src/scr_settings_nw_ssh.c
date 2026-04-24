#include "lvgl/lvgl.h"
#include "scr_settings_nw_ssh.h"
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

/**
 * @brief Creates the SSH configuration sub-screen.
 * * @param l_menu The parent menu object that handles page navigation.
 * @param btn    The button on the main 'Networks' settings page that opens this.
 */
void scr_settings_nw_ssh_create(lv_obj_t* l_menu, lv_obj_t* btn)
{
    // 1. Create the Main SSH Page (Assuming this sets up the top "< SSH" header)
    lv_obj_t* ssh_page = tt_obj_menu_page_create(l_menu, btn, NULL, "SSH");

    // 2. The Main Row Container (SSH enable | Switch)
    lv_obj_t* row_cont = lv_obj_create(ssh_page);
    lv_obj_set_size(row_cont, LV_PCT(90), LV_SIZE_CONTENT); // Span 90% width
    lv_obj_center(row_cont);                                // Center vertically/horizontally
    
    // Configure row alignment: Middle left to Middle right
    lv_obj_set_flex_flow(row_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* --- ADD A BLACK BACKGROUND / REMOVE BORDER --- */
    // Usually required to match the background
    lv_obj_set_style_bg_color(row_cont, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(row_cont, 0, LV_PART_MAIN);

    // 3. The "SSH enable" Label
    lv_obj_t* lbl_ssh = tt_obj_label_create(row_cont, "SSH enable");
    lv_obj_set_style_text_font(lbl_ssh, &lv_font_montserrat_16, 0); // Example font
    
    // Add space after label so the switch is pushed to the far right
    lv_obj_set_style_margin_right(lbl_ssh, LV_SIZE_CONTENT, 0); 
    lv_obj_set_flex_grow(lbl_ssh, 1); // Pushes the next object (the switch) right

    // 4. The ON/OFF Switch (Replaces 'X')
    lv_obj_t* sw_ssh = lv_switch_create(row_cont);
    // Optional: add a specific event callback for the switch
    // lv_obj_add_event_cb(sw_ssh, ssh_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // (Optional) You can customize colors here for ON/OFF state if needed.

    // 5. The Bottom Buttons Container (OK, Cancel)
    lv_obj_t* bot_btns_cont = lv_obj_create(ssh_page);
    lv_obj_set_size(bot_btns_cont, LV_PCT(100), 50); // Set height, full width
    lv_obj_align(bot_btns_cont, LV_ALIGN_BOTTOM_MID, 0, -10); // Align bottom center, 10px padding

    // Configure grid: Flow Right, Align Bottom Right
    lv_obj_set_flex_flow(bot_btns_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bot_btns_cont, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);

    /* --- REMOVE CONTAINER STYLING --- */
    lv_obj_set_style_bg_opa(bot_btns_cont, LV_OPA_0, LV_PART_MAIN); // Make invisible
    lv_obj_set_style_border_width(bot_btns_cont, 0, LV_PART_MAIN);

    // 6. Add the Buttons
    // tt_obj_btn_create(parent, callback, label, img_path, width, height, lbl_align)
    lv_obj_t* btn_ok = tt_obj_btn_create(bot_btns_cont, NULL, "OK", NULL, 80, 40, LV_ALIGN_CENTER);
    lv_obj_t* btn_cancel = tt_obj_btn_create(bot_btns_cont, NULL, "Cancel", NULL, 80, 40, LV_ALIGN_CENTER);

    /* Add Event Callbacks (e.g., Close screen, Save settings) */
    // lv_obj_add_event_cb(btn_ok, ssh_ok_event_cb, LV_EVENT_CLICKED, NULL);
    // lv_obj_add_event_cb(btn_cancel, ssh_cancel_event_cb, LV_EVENT_CLICKED, NULL);
}
