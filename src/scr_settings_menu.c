#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"

#include "scr_settings_menu.h"
#include "scr_settings_vis.h"
#include "scr_settings_nw.h"
#include "scr_settings_sys.h"
#include "scr_settings_upd.h"
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

#define TIMER_ROT 5000 // ms
#define TIMER_CHECK_UPDATE 1000 // ms

#ifdef SIMULATOR_ENABLED
#define MOUNT_DIR "/home/guille"
#else
#define MOUNT_DIR "/run/mount"
#endif

/* Global variables ***********************************************************/
static lv_obj_t* menu;

/* Static Helpers *************************************************************/

/**
 * Creates a horizontal container suitable for grouping buttons
 */
static lv_obj_t* create_button_row(lv_obj_t* parent) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_opa(row, 0, 0);
    lv_obj_set_style_pad_all(row, 5, 0);

    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    // Align everything to the center horizontally
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Add gap between buttons
    lv_obj_set_style_pad_gap(row, 15, 0);

    return row;
}

/**
 * Creates a standard large, rounded-square HUB button
 */
static lv_obj_t* create_hub_btn(lv_obj_t* parent, const char* text, const char* icon_path, lv_event_cb_t cb) {
    // Re-using tt_obj_btn_mtx_create which handles icon+text matrix style
    lv_obj_t* btn = tt_obj_btn_mtx_create(parent, cb, text, icon_path);
    
    // Size to match mockup (approx 30% width)
    lv_obj_set_size(btn, LV_PCT(30), 120); 
    
    // Smooth rounded corners as seen in the mockup
    lv_obj_set_style_radius(btn, 15, 0);
    
    // Standard text alignment
    lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, 0);
    return btn;
}

/* Function definitions *******************************************************/

void scr_settings_menu_create(lv_obj_t* l_menu, lv_obj_t* btn) {
    menu = l_menu;

    // 1. Create the detailed settings page hub (Back button/header handled by menu system)
    lv_obj_t* settings_page = tt_obj_menu_page_create(menu, btn, NULL, "Settings");
    
    // Enable scrolling if necessary, though this specific layout doesn't require it
    lv_obj_set_scrollbar_mode(settings_page, LV_SCROLLBAR_MODE_AUTO);

    // Main central container
    lv_obj_t* cont = tt_obj_cont_create(settings_page);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(cont, 15, 0);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* --- HUB Buttons Layout to match image_3.png --- */

    // Create Row 1: Fits the top 3 buttons (Visualization, Networks, System setup)
    lv_obj_t* row1 = create_button_row(cont);
    
    lv_obj_t* btn_vis = create_hub_btn(row1, "Visualisation", ASSET("menu.png"), NULL);
    lv_obj_t* btn_nw = create_hub_btn(row1, "Networks", ASSET("menu.png"), NULL);
    lv_obj_t* btn_sys = create_hub_btn(row1, "System setup", ASSET("menu.png"), NULL);

    // Create Row 2: Fits the bottom centered button (PDU update)
    lv_obj_t* row2 = create_button_row(cont);
    
    lv_obj_t* btn_update = create_hub_btn(row2, "PDU update", ASSET("menu.png"), NULL);

    /* Navigation Registration (This links the buttons to the sub-pages) */
    scr_settings_vis_create(menu, btn_vis);
    scr_settings_nw_create(menu, btn_nw);
    scr_settings_sys_create(menu, btn_sys);
    scr_settings_update_create(menu, btn_update);
}