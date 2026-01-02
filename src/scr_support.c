
#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_support.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"
#include "screen.h"

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

lv_obj_t* scr_support_create(lv_obj_t* menu)
{
	lv_obj_t* support_cont = tt_obj_menu_page_create(menu, NULL,
			menu_cb, "Settings / Support");

	tt_obj_label_create(support_cont, "Contact with us:");
	tt_obj_label_create(support_cont, "jl.talavera@net-eng.com");
	lv_obj_t* qr = lv_qrcode_create(support_cont);
	const char* data = "mailto:jl.talavera@net-eng.com";
	if (screen_is_landscape()) {
		lv_qrcode_set_size(qr, 100);
		lv_obj_set_style_border_width(qr, 10, 0);
	} else {
		lv_qrcode_set_size(qr, 150);
		lv_obj_set_style_border_width(qr, 20, 0);
	}
	lv_qrcode_update(qr, data, strlen(data));
	lv_obj_center(qr);
	lv_obj_set_style_border_color(qr, lv_color_white(), 0);

	return lv_obj_get_parent(support_cont);
}