#include <stdio.h>
#include "lvgl/lvgl.h"
#include "scr_current.h"
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
static lv_obj_t* menu;


/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			LV_LOG_USER("Network menu cb");
		}
	}
}

/* Public functions ***********************************************************/

void scr_current_create(lv_obj_t* l_menu, lv_obj_t* btn)
{
	menu = l_menu;

	/* Create the settings page directly */
    lv_obj_t* nw_menu_page = tt_obj_menu_page_create(menu, btn, NULL, "Networks");

	/* Apply Flex layout directly to the page. 
       In LVGL, menu pages usually have a 'scrollable' part or are objects themselves 
       that can act as containers.
    */
    lv_obj_set_flex_flow(nw_menu_page, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(
        nw_menu_page,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

	// Create buttons for each network protocol (parent is the menu page)
	lv_obj_t* btn_10a = tt_obj_btn_mtx_create(nw_menu_page, NULL, "10\nA", NULL);
	lv_obj_t* btn_15a = tt_obj_btn_mtx_create(nw_menu_page, NULL, "15\nA", NULL);
	lv_obj_t* btn_16a = tt_obj_btn_mtx_create(nw_menu_page, NULL, "16\nA", NULL);
	lv_obj_t* btn_20a = tt_obj_btn_mtx_create(nw_menu_page, NULL, "20\nA", NULL);
	lv_obj_t* btn_30a = tt_obj_btn_mtx_create(nw_menu_page, NULL, "30\nA", NULL);
	lv_obj_t* btn_32a = tt_obj_btn_mtx_create(nw_menu_page, NULL, "32\nA", NULL);
}
