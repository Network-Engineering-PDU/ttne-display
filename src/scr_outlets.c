#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"

#include "scr_outlets.h"
#include "scr_outlet_data.h"
#include "scr_power_data.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "screen.h"
#include "models.h"
#include "backend/backend.h"

#define MAX_OUTLETS 48

/* Global variables ***********************************************************/

static lv_obj_t* menu;

static lv_obj_t* loader_scr;

static lv_obj_t* btn_out[MAX_OUTLETS];

static lv_obj_t* out_data_page;
static lv_obj_t* lbl_no_out;
static lv_obj_t* btn_en_all;
static lv_obj_t* btn_dis_all;

typedef struct tt_user_data_t {
	lv_obj_t* active_screen;
} tt_user_data_t;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void btn_out_cb(lv_event_t* e);
static void msg_box_all_cb(lv_event_t* e);
static void btn_all_cb(lv_event_t* e);

static void outlets_refresh_cb(int err, void* userdata);
static void outlets_set_all_cb(int err, void* userdata);
static void update_outlet_buttons(void);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			if (backend_outlets_refresh(outlets_refresh_cb, NULL) != 0) {
				tt_obj_info_box_create("Outlets", "Outlet refresh unavailable", 1);
			}
			update_outlet_buttons();
		}
	}
}

static void btn_out_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	int out_id = (long)lv_event_get_user_data(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		scr_outlet_data_set_out(out_id);
		lv_menu_set_page(menu, out_data_page);	
	}
}

static void msg_box_all_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);
	bool action = (bool)lv_event_get_user_data(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
			if (lv_msgbox_get_active_btn(obj) == 0) { // YES
				tt_user_data_t* tt_user_data = malloc(sizeof(tt_user_data_t));
				if (tt_user_data == NULL) {
					lv_msgbox_close(obj);
					return;
				}
				tt_user_data->active_screen = lv_scr_act();
				loader_scr = tt_obj_loader_create(action ?
						"Enabling all switches" : "Disabling all switches", NULL);
				lv_scr_load(loader_scr);
				if (backend_outlets_set_all(action, outlets_set_all_cb,
						tt_user_data) != 0) {
					lv_scr_load(tt_user_data->active_screen);
					lv_obj_del(loader_scr);
					loader_scr = NULL;
					free(tt_user_data);
					tt_obj_info_box_create("Outlets", "Outlet update unavailable", 1);
				}
		}
		lv_msgbox_close(obj);
	}
}

static void btn_all_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	bool action = (bool)lv_event_get_user_data(e);

	if (code == LV_EVENT_CLICKED) {
		char msg[100];
		sprintf(msg, "Are you sure you want to " TT_COLOR_GREEN_NE_STR
				" %s all outlets#?", action ? "enable" : "disable");
		lv_obj_t* msg_box = tt_obj_msg_box_create("Outlets", msg, NULL, NULL);
		lv_obj_add_event_cb(msg_box, msg_box_all_cb, LV_EVENT_ALL,
				(void*)(action));
	}
}

static void outlets_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	if (err != 0) {
		tt_obj_info_box_create("Outlets", "Could not refresh outlets", 1);
		return;
	}
	update_outlet_buttons();
}

static void outlets_set_all_cb(int err, void* userdata)
{
	tt_user_data_t* tt_user_data = userdata;
	lv_obj_t* scr = tt_user_data != NULL ? tt_user_data->active_screen : NULL;

	if (scr != NULL) {
		lv_scr_load(scr);
	}
	if (loader_scr != NULL) {
		lv_obj_del(loader_scr);
		loader_scr = NULL;
	}
	if (tt_user_data != NULL) {
		free(tt_user_data);
	}

	if (err != 0) {
		tt_obj_info_box_create("Outlets", "Could not update outlets", 1);
		return;
	}

	update_outlet_buttons();
}

static void update_outlet_buttons(void)
{
	int len;
	const models_out_sw_t* out_sw = models_get_out_sw(&len);
	if (len > MAX_OUTLETS) {
		len = MAX_OUTLETS;
	}

	if (len == 0 || out_sw == NULL) {
		lv_obj_add_flag(btn_en_all, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(btn_dis_all, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_no_out, LV_OBJ_FLAG_HIDDEN);
		for (int i = 0; i < MAX_OUTLETS; i++) {
			lv_obj_add_flag(btn_out[i], LV_OBJ_FLAG_HIDDEN);
		}
		return;
	}

	lv_obj_clear_flag(btn_en_all, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(btn_dis_all, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(lbl_no_out, LV_OBJ_FLAG_HIDDEN);

	for (int i = 0; i < MAX_OUTLETS; i++) {
		if (i < len) {
			tt_obj_btn_toggle_set_state(btn_out[i], out_sw[i].status);
			lv_obj_clear_flag(btn_out[i], LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_add_flag(btn_out[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void scr_outlets_create(lv_obj_t* l_menu, lv_obj_t* btn)
{
	menu = l_menu;

	lv_obj_t* outlets_cont = tt_obj_menu_page_create(menu, btn,
			menu_cb, "Outlets");

	out_data_page = scr_outlet_data_create(menu);

	lv_obj_t* outlets_status_cont = tt_obj_cont_create(outlets_cont);
	lv_obj_set_flex_flow(outlets_status_cont, LV_FLEX_FLOW_ROW_WRAP);

	lbl_no_out = tt_obj_label_color_create(outlets_status_cont,
			"No outlets found");
	lv_obj_add_flag(lbl_no_out, LV_OBJ_FLAG_HIDDEN);
	btn_en_all = tt_obj_btn_perc_create(outlets_status_cont, NULL,
			"ENABLE ALL", 48);
	lv_obj_add_event_cb(btn_en_all, btn_all_cb, LV_EVENT_ALL, (void*)true);
	btn_dis_all = tt_obj_btn_perc_create(outlets_status_cont, NULL,
			"DISABLE ALL", 48);
	lv_obj_add_event_cb(btn_dis_all, btn_all_cb, LV_EVENT_ALL, (void*)false);

	for (int i = 0; i < MAX_OUTLETS; i++) {
		char str[10];
		sprintf(str, "OUT %d", i + 1);
		if (screen_is_landscape()) {
			btn_out[i] = tt_obj_btn_toggle_perc_create(outlets_status_cont,
					NULL, str, 23);
		} else {
			btn_out[i] = tt_obj_btn_toggle_perc_create(outlets_status_cont,
					NULL, str, 30);
		}
		lv_obj_add_event_cb(btn_out[i], btn_out_cb, LV_EVENT_ALL,
				(void*)(long)(i + 1));
		lv_obj_set_scroll_dir(btn_out[i], LV_DIR_NONE);
	}
}
