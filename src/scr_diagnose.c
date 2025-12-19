
#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_diagnose.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"

/* Global variables ***********************************************************/

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			
		}
	}
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

lv_obj_t* scr_diagnose_create(lv_obj_t* menu)
{
	lv_obj_t* diagnose_cont = tt_obj_menu_page_create(menu, NULL,
			menu_cb, "Settings / Diagnose");

	tt_obj_label_color_create(diagnose_cont, "Diagnose system");
	char msg[50];
	sprintf(msg, TT_COLOR_GREEN_NE_STR "     No error found#");
	tt_obj_label_color_create(diagnose_cont, msg);

	return lv_obj_get_parent(diagnose_cont);
}