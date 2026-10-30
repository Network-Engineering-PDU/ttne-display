#include <stdio.h>

#include "i18n.h"
#include "lvgl/lvgl.h"

#include "scr_power_data.h"
#include "tt_colors.h"
#include "tt_obj.h"
#include "utils.h"

/* Global variables ***********************************************************/
/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);

/* Callbacks ******************************************************************/

/* "Voltage: 230.00 V", the value colored */
static void format_reading(char* dst, size_t size, const char* name, float value,
		const char* unit)
{
	char number[24];
	char colored[96];
	snprintf(number, sizeof(number), "%.2f", value);
	i18n_format_value(colored, sizeof(colored), TT_COLOR_GREEN_NE, number, unit);
	snprintf(dst, size, "%s: %s", name, colored);
}

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
			tr("Power / Input x"));

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
			tr("Input enabled"));
	lv_obj_set_height(btn_toggle, 30);

	lv_obj_t* data_cont = tt_obj_cont_create(power_data_cont);

	char pwr_data_str[160];
	format_reading(pwr_data_str, sizeof(pwr_data_str), tr("Voltage"), pwr_data.voltage, tr("V"));
	tt_obj_label_color_create(data_cont, pwr_data_str);
	format_reading(pwr_data_str, sizeof(pwr_data_str), tr("Current"), pwr_data.current, tr("A"));
	tt_obj_label_color_create(data_cont, pwr_data_str);
	format_reading(pwr_data_str, sizeof(pwr_data_str), tr("Active power"), pwr_data.active_power, tr("W"));
	tt_obj_label_color_create(data_cont, pwr_data_str);
	format_reading(pwr_data_str, sizeof(pwr_data_str), tr("Reactive power"), pwr_data.reactive_power, tr("VAr"));
	tt_obj_label_color_create(data_cont, pwr_data_str);
	format_reading(pwr_data_str, sizeof(pwr_data_str), tr("Apparent power"), pwr_data.apparent_power, tr("VA"));
	tt_obj_label_color_create(data_cont, pwr_data_str);
	format_reading(pwr_data_str, sizeof(pwr_data_str), tr("Power factor"), pwr_data.power_factor, NULL);
	tt_obj_label_color_create(data_cont, pwr_data_str);
	format_reading(pwr_data_str, sizeof(pwr_data_str), tr("Frequency"), pwr_data.frequency, tr("Hz"));
	tt_obj_label_color_create(data_cont, pwr_data_str);
	format_reading(pwr_data_str, sizeof(pwr_data_str), tr("Phase"), pwr_data.phase_vi, tr("deg"));
	tt_obj_label_color_create(data_cont, pwr_data_str);
	format_reading(pwr_data_str, sizeof(pwr_data_str), tr("Energy"), pwr_data.energy, tr("Wh"));
	tt_obj_label_color_create(data_cont, pwr_data_str);
}