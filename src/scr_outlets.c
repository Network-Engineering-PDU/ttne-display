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
#include "controller.h"

#define MAX_OUTLETS 48
#define TIMER_TOGGLE_ALL 500 // ms

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
	bool action;
	int line_id;
} tt_user_data_t;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void btn_out_cb(lv_event_t* e);
static void msg_box_all_cb(lv_event_t* e);
static void btn_all_cb(lv_event_t* e);

static void timer_toggle_all_cb(lv_timer_t* timer);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			controller_get_out_sw();
			int len;
			const models_out_sw_t* out_sw = models_get_out_sw(&len);
			if (len == 0) {
				lv_obj_add_flag(btn_en_all, LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(btn_dis_all, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(lbl_no_out, LV_OBJ_FLAG_HIDDEN);
			}
			for (int i = 0; i < MAX_OUTLETS; i++) {
				if (i < len) {
					LV_LOG_USER("OUTLET %d (line %d) status: %d", i,
							out_sw[i].line_id, out_sw[i].status);
					tt_obj_btn_toggle_set_state(btn_out[i], out_sw[i].status);
					lv_obj_clear_flag(btn_out[i], LV_OBJ_FLAG_HIDDEN);
				} else {
					lv_obj_add_flag(btn_out[i], LV_OBJ_FLAG_HIDDEN);
				}
			}
		}
	}
}

static void btn_out_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	int out_id = (int)lv_event_get_user_data(e);

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
			tt_user_data->active_screen = lv_scr_act();
			tt_user_data->action = action;
			tt_user_data->line_id = 0;
			lv_timer_create(timer_toggle_all_cb, TIMER_TOGGLE_ALL, tt_user_data);
			loader_scr = tt_obj_loader_create(action ?
					"Enabling all switches" : "Disabling all switches", NULL);
			lv_scr_load(loader_scr);
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

static void timer_toggle_all_cb(lv_timer_t* timer)
{
	tt_user_data_t* tt_user_data = timer->user_data;
	lv_obj_t* scr = tt_user_data->active_screen;
	bool action = tt_user_data->action;
	int line_id = tt_user_data->line_id;
	models_out_sw_t out_sw;
	int len;
	models_get_out_sw(&len);
	out_sw.line_id = line_id;
	out_sw.status = action;
	tt_obj_btn_toggle_set_state(btn_out[line_id], action);
	controller_put_out_sw(&out_sw, line_id);
	tt_user_data->line_id++;
	if (tt_user_data->line_id >= len) {
		lv_timer_del(timer);
		lv_scr_load(scr);
		lv_obj_del(loader_scr);
		free(tt_user_data);
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
