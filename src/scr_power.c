#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_power.h"
#include "scr_power_data.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"

#define MAX_BRANCHES 2
#define MAX_PHASES 3
#define N_INPUTS (MAX_BRANCHES * MAX_PHASES)
#define TIMER_REFRESH_RATE 2000 // ms

/* Global variables ***********************************************************/

static bool running = false;

static lv_obj_t* g_menu;

static lv_timer_t* timer;

static lv_obj_t* power_info_cont;
static lv_obj_t* power_line_cont[MAX_BRANCHES];

static lv_obj_t* lbl_branch;
static lv_obj_t* lbl_sys_type;

static lv_obj_t* lbl_tot_p;
static lv_obj_t* lbl_tot_e;

static lv_obj_t* lbl_line_v[MAX_BRANCHES];
static lv_obj_t* lbl_line_c[MAX_BRANCHES];
static lv_obj_t* lbl_line_p[MAX_BRANCHES];
static lv_obj_t* lbl_line_q[MAX_BRANCHES];
static lv_obj_t* lbl_line_s[MAX_BRANCHES];
static lv_obj_t* lbl_line_pf[MAX_BRANCHES];
static lv_obj_t* lbl_line_ph[MAX_BRANCHES];
static lv_obj_t* lbl_line_f[MAX_BRANCHES];
static lv_obj_t* lbl_line_e[MAX_BRANCHES];

static uint8_t n_branches = 0;
static uint8_t n_phases = 0;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void power_timer_cb();
static void set_line_label(lv_obj_t* lbl, const char* param, float data1,
		float data2, float data3);

static void get_in_sw();

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			timer = lv_timer_create(power_timer_cb, TIMER_REFRESH_RATE, NULL);
			power_timer_cb(timer);
			if (!running) {
				running = true;
			}
			get_in_sw();
		}
	} else if (code == LV_EVENT_CLICKED) {
		if (running) {
			running = false;
			lv_timer_del(timer);
		}
	}
}

static void set_line_label(lv_obj_t* lbl, const char* param, float data1,
		float data2, float data3)
{
	char str[100];
	if (n_phases == 1) {
		sprintf(str, "%s: #%06X %.1f#", param, TT_COLOR_GREEN_NE, data1);
	} else if (n_phases == 3) {
		sprintf(str, "%s: #%06X %.1f# / #%06X %.1f# / #%06X %.1f#", param,
				TT_COLOR_GREEN_NE, data1, TT_COLOR_GREEN_NE, data2,
				TT_COLOR_GREEN_NE, data3);
	} else {
		sprintf(str, "%s: #%06X ERROR#", param, TT_COLOR_GREEN_NE);
	}
	// SEGMENTATION FAULT
	lv_label_set_text(lbl, str);
}

/* Callbacks ******************************************************************/

static void power_timer_cb()
{
	models_in_data_t in_data[N_INPUTS];
	for (int i = 0; i < N_INPUTS; i++) {
		controller_get_in_data(i);
		const models_in_data_t* current_in_data;
		current_in_data = models_get_in_data();
		memcpy(&in_data[i], current_in_data, sizeof(models_in_data_t));
	}

	float tot_p = 0.0;
	float tot_e = 0.0;
	for (int i = 0; i < N_INPUTS; i++) {
		tot_p += in_data[i].active_power;
		tot_e += in_data[i].energy;
	}

	char label[100];
	sprintf(label, "Power: #%06X %.2f W" ,TT_COLOR_GREEN_NE, tot_p);
	lv_label_set_text(lbl_tot_p, label);
	sprintf(label, "Energy: #%06X %.2f Wh" ,TT_COLOR_GREEN_NE, tot_e);
	lv_label_set_text(lbl_tot_e, label);

	for (int i = 0; i < n_branches; i++) {
		set_line_label(lbl_line_v[i], "V (V)", in_data[0+(n_phases*i)].voltage,
			in_data[1+(n_phases*i)].voltage, in_data[2+(n_phases*i)].voltage);
		set_line_label(lbl_line_c[i], "I (A)", in_data[0+(n_phases*i)].current,
			in_data[1+(n_phases*i)].current, in_data[2+(n_phases*i)].current);
		set_line_label(lbl_line_p[i], "P (W)", in_data[0+(n_phases*i)].active_power,
			in_data[1+(n_phases*i)].active_power, in_data[2+(n_phases*i)].active_power);
		set_line_label(lbl_line_q[i], "Q (VAr)", in_data[0+(n_phases*i)].reactive_power,
			in_data[1+(n_phases*i)].reactive_power, in_data[2+(n_phases*i)].reactive_power);
		set_line_label(lbl_line_s[i], "S (VA)", in_data[0+(n_phases*i)].apparent_power,
			in_data[1+(n_phases*i)].apparent_power, in_data[2+(n_phases*i)].apparent_power);
		set_line_label(lbl_line_pf[i], "PF", in_data[0+(n_phases*i)].power_factor,
			in_data[1+(n_phases*i)].power_factor, in_data[2+(n_phases*i)].power_factor);
		set_line_label(lbl_line_ph[i], "Ph (deg)", in_data[0+(n_phases*i)].phase,
			in_data[1+(n_phases*i)].phase, in_data[2+(n_phases*i)].phase);
		set_line_label(lbl_line_f[i], "f (Hz)", in_data[0+(n_phases*i)].frequency,
			in_data[1+(n_phases*i)].frequency, in_data[2+(n_phases*i)].frequency);
		set_line_label(lbl_line_e[i], "E (Wh)", in_data[0+(n_phases*i)].energy,
			in_data[1+(n_phases*i)].energy, in_data[2+(n_phases*i)].energy);
	}
}

/* Function definitions *******************************************************/

static void get_in_sw()
{
	controller_get_in_sw();
	const models_in_sw_t* in_sw = models_get_in_sw();
	char pwr_info_str[100];

	switch (in_sw->branch) {
	case BRANCH_MAIN:
		sprintf(pwr_info_str, "Branch: #%06X Main", TT_COLOR_GREEN_NE);
		n_branches = 1;
		lv_obj_clear_flag(power_line_cont[0], LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(power_line_cont[1], LV_OBJ_FLAG_HIDDEN);
		break;
	case BRANCH_ALL:
		sprintf(pwr_info_str, "Branch: #%06X Main and aux", TT_COLOR_GREEN_NE);
		n_branches = 2;
		lv_obj_clear_flag(power_line_cont[0], LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(power_line_cont[1], LV_OBJ_FLAG_HIDDEN);
		break;
	default:
		sprintf(pwr_info_str, "Branch: #%06X ERROR", TT_COLOR_GREEN_NE);
		lv_obj_add_flag(power_line_cont[0], LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(power_line_cont[1], LV_OBJ_FLAG_HIDDEN);
		n_branches = 0;
		break;
	};
	lv_label_set_text(lbl_branch, pwr_info_str);
	switch (in_sw->sys_type) {
	case TYPE_SINGLE:
		sprintf(pwr_info_str, "Type: #%06X Single-phase", TT_COLOR_GREEN_NE);
		n_phases = 1;
		break;
	case TYPE_BI:
		sprintf(pwr_info_str, "Type: #%06X Bi-phase", TT_COLOR_GREEN_NE);
		n_phases = 2;
		break;
	case TYPE_TRI:
		sprintf(pwr_info_str, "Type: #%06X Three-phase without N", TT_COLOR_GREEN_NE);
		n_phases = 3;
		break;
	case TYPE_TRI_N:
		sprintf(pwr_info_str, "Type: #%06X Three-phase with N", TT_COLOR_GREEN_NE);
		n_phases = 3;
		break;
	default:
		sprintf(pwr_info_str, "Type: #%06X ERROR", TT_COLOR_GREEN_NE);
		n_phases = 0;
		break;
	};
	lv_label_set_text(lbl_sys_type, pwr_info_str);
	switch (in_sw->curr_type) {
	case CURR_MLX:
		LV_LOG_USER("Current: IMC-Hall sensor");
		break;
	case CURR_TRA:
		LV_LOG_USER("Current: Current transformer sensor");
		break;
	default:
		LV_LOG_USER("Current: ERROR");
		break;
	};
}

/* Public functions ***********************************************************/

void scr_power_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* power_cont = tt_obj_menu_page_create(menu, btn, menu_cb, "Power");
	g_menu = menu;
	power_info_cont = tt_obj_cont_create(power_cont);
	tt_obj_label_create(power_info_cont, "Power info");

	lbl_branch = tt_obj_label_color_create(power_info_cont, "");
	lbl_sys_type = tt_obj_label_color_create(power_info_cont, "");
	
	lv_obj_t* power_tot_cont = tt_obj_cont_create(power_cont);
	tt_obj_label_create(power_tot_cont, "TOTAL");
	lbl_tot_p = tt_obj_label_color_create(power_tot_cont, "");
	lbl_tot_e = tt_obj_label_color_create(power_tot_cont, "");

	power_line_cont[0] = tt_obj_cont_create(power_cont);
	power_line_cont[1] = tt_obj_cont_create(power_cont);
	tt_obj_label_create(power_line_cont[0], "MAIN BRANCH");
	tt_obj_label_create(power_line_cont[1], "AUX BRANCH");
	get_in_sw();

	for (int i = 0; i < MAX_BRANCHES; i++) {
		lbl_line_v[i] = tt_obj_label_color_create(power_line_cont[i], "");
		lbl_line_c[i] = tt_obj_label_color_create(power_line_cont[i], "");
		lbl_line_p[i] = tt_obj_label_color_create(power_line_cont[i], "");
		lbl_line_q[i] = tt_obj_label_color_create(power_line_cont[i], "");
		lbl_line_s[i] = tt_obj_label_color_create(power_line_cont[i], "");
		lbl_line_pf[i] = tt_obj_label_color_create(power_line_cont[i], "");
		lbl_line_ph[i] = tt_obj_label_color_create(power_line_cont[i], "");
		lbl_line_f[i] = tt_obj_label_color_create(power_line_cont[i], "");
		lbl_line_e[i] = tt_obj_label_color_create(power_line_cont[i], "");
	}
}
