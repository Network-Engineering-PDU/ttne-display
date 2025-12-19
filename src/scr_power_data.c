#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_power_data.h"
#include "tt_colors.h"
#include "tt_obj.h"
#include "utils.h"

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
			LV_LOG_USER("Power data cb");
		}
	}
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void scr_power_data_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* power_data_cont = tt_obj_menu_page_create(menu, btn, menu_cb,
			"Power / Input x");

	power_data_t pwr_data;
	pwr_data.voltage = 223.61;
	pwr_data.current = 30.6;
	pwr_data.active_power = 319.02;
	pwr_data.reactive_power = 23.65;
	pwr_data.apparent_power = 320.42;
	pwr_data.power_factor = 0.6;
	pwr_data.frequency = 50.01;
	pwr_data.phase_vi = 60.9;
	pwr_data.energy = 25.5;

	lv_obj_t* btn_toggle = tt_obj_btn_toggle_create(power_data_cont, NULL,
			"Input enabled");
	lv_obj_set_height(btn_toggle, 30);

	lv_obj_t* data_cont = tt_obj_cont_create(power_data_cont);

	char pwr_data_str[100];
	sprintf(pwr_data_str, "%s: #%06X %.2f V" , "Voltage", TT_COLOR_GREEN_NE, pwr_data.voltage);
	tt_obj_label_color_create(data_cont, pwr_data_str);
	sprintf(pwr_data_str, "%s: #%06X %.2f A" , "Current", TT_COLOR_GREEN_NE, pwr_data.current);
	tt_obj_label_color_create(data_cont, pwr_data_str);
	sprintf(pwr_data_str, "%s: #%06X %.2f W" , "Active power", TT_COLOR_GREEN_NE, pwr_data.active_power);
	tt_obj_label_color_create(data_cont, pwr_data_str);
	sprintf(pwr_data_str, "%s: #%06X %.2f VAr" , "Reactive power", TT_COLOR_GREEN_NE, pwr_data.reactive_power);
	tt_obj_label_color_create(data_cont, pwr_data_str);
	sprintf(pwr_data_str, "%s: #%06X %.2f VA" , "Apparent power", TT_COLOR_GREEN_NE, pwr_data.apparent_power);
	tt_obj_label_color_create(data_cont, pwr_data_str);
	sprintf(pwr_data_str, "%s: #%06X %.2f" , "Power factor", TT_COLOR_GREEN_NE, pwr_data.power_factor);
	tt_obj_label_color_create(data_cont, pwr_data_str);
	sprintf(pwr_data_str, "%s: #%06X %.2f Hz" , "Frequency", TT_COLOR_GREEN_NE, pwr_data.frequency);
	tt_obj_label_color_create(data_cont, pwr_data_str);
	sprintf(pwr_data_str, "%s: #%06X %.2f deg" , "Phase", TT_COLOR_GREEN_NE, pwr_data.phase_vi);
	tt_obj_label_color_create(data_cont, pwr_data_str);
	sprintf(pwr_data_str, "%s: #%06X %.2f Wh" , "Energy", TT_COLOR_GREEN_NE, pwr_data.energy);
	tt_obj_label_color_create(data_cont, pwr_data_str);
}