#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"

#include "scr_settings_menu.h"
#include "scr_settings_vis.h"
#include "scr_settings_sys.h"
#include "scr_settings_upd.h"
#include "scr_settings_nw_menu.h"
#include "scr_current.h"
#include "scr_login.h"
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

static lv_obj_t* sys_info_scr;

static lv_timer_t* timer_check;

static lv_obj_t* init_spinner;

static lv_obj_t* lbl_system;
static lv_obj_t* lbl_ip;

static lv_obj_t* l_menu;
char str[100];

/* Function definitions *******************************************************/


/* Callbacks ******************************************************************/

static void sys_info_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* scr = lv_event_get_user_data(e);

	if (lv_scr_act() != sys_info_scr) {
		return;
	}
	if (code == LV_EVENT_CLICKED) {
		lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
		lv_timer_pause(timer_check);
		lv_scr_load(scr);
	}
}


void scr_settings_sys_info_create(lv_obj_t* menu, lv_obj_t* btn) {

    l_menu = menu;
    /* Create the settings page directly */
    lv_obj_t* settings_sys_info_page = tt_obj_menu_page_create(l_menu, btn, NULL, "System Info");
    lv_obj_set_flex_flow(settings_sys_info_page, LV_FLEX_FLOW_COLUMN);
    
    /* Create and display the model field and value labels */
    lv_obj_t* model_title_label = lv_label_create(settings_sys_info_page);
    lv_label_set_text(model_title_label, "Model:");
    lv_obj_set_style_text_font(model_title_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(model_title_label, LV_TEXT_ALIGN_LEFT, 0);

    lv_obj_t* model_value_label = lv_label_create(settings_sys_info_page);
    lv_label_set_text(model_value_label, "PowerIT Easy");
    lv_obj_set_style_text_font(model_value_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(model_value_label, LV_TEXT_ALIGN_LEFT, 0);
    
}
