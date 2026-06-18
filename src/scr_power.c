#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "scr_power.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define MAX_BRANCHES 2
#define MAX_PHASES 3
#define TIMER_REFRESH_RATE 2000 // ms

/* Global variables ***********************************************************/

static bool running = false;
static bool refresh_pending;

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
static lv_obj_t* power_branch_column_create(lv_obj_t* parent);
static void power_timer_cb(lv_timer_t* timer);
static void power_refresh_cb(int err, void* userdata);
static void apply_power_snapshot(void);
static void set_line_label(lv_obj_t* lbl, const char* param, float data1,
		float data2, float data3);

static const app_state_power_input_t* get_power_input(
		const app_state_power_t* power, int index)
{
	if (power == NULL || index < 0 || index >= power->input_count ||
			index >= APP_STATE_MAX_POWER_INPUTS) {
		return NULL;
	}
	return &power->inputs[index];
}

static float positive_energy(float energy)
{
	return energy < 0.0f ? -energy : energy;
}

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
			lv_obj_t* curr_page = lv_event_get_user_data(e);
			lv_obj_t* page = lv_menu_get_cur_main_page(obj);
			if (curr_page == page) {
				if (timer == NULL) {
					timer = lv_timer_create(power_timer_cb, TIMER_REFRESH_RATE, NULL);
				}
				power_timer_cb(timer);
				if (!running) {
					running = true;
			}
		}
		} else if (code == LV_EVENT_CLICKED) {
			if (running) {
				running = false;
				if (timer != NULL) {
					lv_timer_del(timer);
					timer = NULL;
				}
			}
		}
	}

static lv_obj_t* power_branch_column_create(lv_obj_t* parent)
{
	lv_obj_t* col = lv_obj_create(parent);
	lv_obj_set_size(col, LV_PCT(50), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_scrollbar_mode(col, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_style_bg_opa(col, LV_OPA_0, 0);
	lv_obj_set_style_border_width(col, 0, 0);
	lv_obj_set_style_pad_all(col, 0, 0);
	lv_obj_set_style_pad_row(col, 2, 0);

	return col;
}

static void set_line_label(lv_obj_t* lbl, const char* param, float data1,
		float data2, float data3)
{
	char str[100];
	if (n_phases == 1) {
		snprintf(str, sizeof(str), "%s: #%06X %.1f#", param,
				TT_COLOR_GREEN_NE, data1);
	} else if (n_phases == 2) {
		snprintf(str, sizeof(str), "%s: #%06X %.1f# / #%06X %.1f#", param,
				TT_COLOR_GREEN_NE, data1, TT_COLOR_GREEN_NE, data2);
	} else if (n_phases == 3) {
		snprintf(str, sizeof(str), "%s: #%06X %.1f# / #%06X %.1f# / #%06X %.1f#", param,
				TT_COLOR_GREEN_NE, data1, TT_COLOR_GREEN_NE, data2,
				TT_COLOR_GREEN_NE, data3);
	} else {
		snprintf(str, sizeof(str), "%s: #%06X ERROR#", param,
				TT_COLOR_GREEN_NE);
	}
	lv_label_set_text(lbl, str);
}

/* Callbacks ******************************************************************/

static void power_timer_cb(lv_timer_t* timer)
{
	(void)timer;
	if (refresh_pending) {
		return;
	}
	if (backend_power_refresh(power_refresh_cb, NULL) == 0) {
		refresh_pending = true;
	}
}

static void power_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	refresh_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("Power", "Could not refresh power data", 1);
		return;
	}
	apply_power_snapshot();
}

static void apply_power_snapshot(void)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	const app_state_power_t* power = &snapshot.power;
	const app_state_power_input_t* in_data = power->inputs;

	if (!power->valid) {
		return;
	}

	float tot_p = 0.0;
	float tot_e = 0.0;
	int input_count = power->input_count;
	if (input_count > APP_STATE_MAX_POWER_INPUTS) {
		input_count = APP_STATE_MAX_POWER_INPUTS;
	}
	for (int i = 0; i < input_count; i++) {
		tot_p += in_data[i].active_power;
		tot_e += (in_data[i].energy < 0.0f) ? -in_data[i].energy : in_data[i].energy;
	}

	char label[100];
	snprintf(label, sizeof(label), "Power: #%06X %.2f W",
			TT_COLOR_GREEN_NE, tot_p);
	lv_label_set_text(lbl_tot_p, label);
	snprintf(label, sizeof(label), "Energy: #%06X %.2f Wh",
			TT_COLOR_GREEN_NE, tot_e);
	lv_label_set_text(lbl_tot_e, label);

	char pwr_info_str[100];
	switch (power->branch) {
	case 0:
		snprintf(pwr_info_str, sizeof(pwr_info_str), "Branch: #%06X Main",
				TT_COLOR_GREEN_NE);
		n_branches = 1;
		lv_obj_clear_flag(power_line_cont[0], LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(power_line_cont[1], LV_OBJ_FLAG_HIDDEN);
		break;
	case 1:
		snprintf(pwr_info_str, sizeof(pwr_info_str),
				"Branch: #%06X Main and aux", TT_COLOR_GREEN_NE);
		n_branches = 2;
		lv_obj_clear_flag(power_line_cont[0], LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(power_line_cont[1], LV_OBJ_FLAG_HIDDEN);
		break;
	default:
		snprintf(pwr_info_str, sizeof(pwr_info_str), "Branch: #%06X ERROR",
				TT_COLOR_GREEN_NE);
		lv_obj_add_flag(power_line_cont[0], LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(power_line_cont[1], LV_OBJ_FLAG_HIDDEN);
		n_branches = 0;
		break;
	};
	lv_label_set_text(lbl_branch, pwr_info_str);
	switch (power->sys_type) {
	case 0:
		snprintf(pwr_info_str, sizeof(pwr_info_str),
				"Type: #%06X Single-phase", TT_COLOR_GREEN_NE);
		n_phases = 1;
		break;
	case 1:
		snprintf(pwr_info_str, sizeof(pwr_info_str), "Type: #%06X Bi-phase",
				TT_COLOR_GREEN_NE);
		n_phases = 2;
		break;
	case 2:
		snprintf(pwr_info_str, sizeof(pwr_info_str),
				"Type: #%06X Three-phase without N", TT_COLOR_GREEN_NE);
		n_phases = 3;
		break;
	case 3:
		snprintf(pwr_info_str, sizeof(pwr_info_str),
				"Type: #%06X Three-phase with N", TT_COLOR_GREEN_NE);
		n_phases = 3;
		break;
	default:
		snprintf(pwr_info_str, sizeof(pwr_info_str), "Type: #%06X ERROR",
				TT_COLOR_GREEN_NE);
		n_phases = 0;
		break;
	};
	lv_label_set_text(lbl_sys_type, pwr_info_str);

	if (n_phases == 0) {
		return;
	}

	for (int i = 0; i < n_branches; i++) {
		int base = n_phases * i;
		if (base + n_phases > input_count) {
			break;
		}
		const app_state_power_input_t* phase1 = get_power_input(power, base);
		const app_state_power_input_t* phase2 = get_power_input(power, base + 1);
		const app_state_power_input_t* phase3 = get_power_input(power, base + 2);
		if (phase1 == NULL) {
			continue;
		}

		set_line_label(lbl_line_v[i], "V (V)", phase1->voltage,
			phase2 != NULL ? phase2->voltage : 0.0f,
			phase3 != NULL ? phase3->voltage : 0.0f);
		set_line_label(lbl_line_c[i], "I (A)", phase1->current,
			phase2 != NULL ? phase2->current : 0.0f,
			phase3 != NULL ? phase3->current : 0.0f);
		set_line_label(lbl_line_p[i], "P (W)", phase1->active_power,
			phase2 != NULL ? phase2->active_power : 0.0f,
			phase3 != NULL ? phase3->active_power : 0.0f);
		set_line_label(lbl_line_q[i], "Q (VAr)", phase1->reactive_power,
			phase2 != NULL ? phase2->reactive_power : 0.0f,
			phase3 != NULL ? phase3->reactive_power : 0.0f);
		set_line_label(lbl_line_s[i], "S (VA)", phase1->apparent_power,
			phase2 != NULL ? phase2->apparent_power : 0.0f,
			phase3 != NULL ? phase3->apparent_power : 0.0f);
		set_line_label(lbl_line_pf[i], "PF", phase1->power_factor,
			phase2 != NULL ? phase2->power_factor : 0.0f,
			phase3 != NULL ? phase3->power_factor : 0.0f);
		set_line_label(lbl_line_ph[i], "PH (deg)", phase1->phase,
			phase2 != NULL ? phase2->phase : 0.0f,
			phase3 != NULL ? phase3->phase : 0.0f);
		set_line_label(lbl_line_f[i], "f (Hz)", phase1->frequency,
			phase2 != NULL ? phase2->frequency : 0.0f,
			phase3 != NULL ? phase3->frequency : 0.0f);
		set_line_label(lbl_line_e[i], "E (Wh)", positive_energy(phase1->energy),
			phase2 != NULL ? positive_energy(phase2->energy) : 0.0f,
			phase3 != NULL ? positive_energy(phase3->energy) : 0.0f);
	}
}

/* Function definitions *******************************************************/

/* Public functions ***********************************************************/

void scr_power_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* power_cont = tt_obj_menu_page_create(menu, btn, menu_cb, "Power");
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
	lv_obj_add_flag(power_line_cont[1], LV_OBJ_FLAG_HIDDEN);

	for (int i = 0; i < MAX_BRANCHES; i++) {
		lv_obj_t* branch_cols = lv_obj_create(power_line_cont[i]);
		lv_obj_set_size(branch_cols, LV_PCT(100), LV_SIZE_CONTENT);
		lv_obj_set_flex_flow(branch_cols, LV_FLEX_FLOW_ROW);
		lv_obj_set_scrollbar_mode(branch_cols, LV_SCROLLBAR_MODE_OFF);
		lv_obj_set_style_bg_opa(branch_cols, LV_OPA_0, 0);
		lv_obj_set_style_border_width(branch_cols, 0, 0);
		lv_obj_set_style_pad_all(branch_cols, 0, 0);
		lv_obj_set_style_pad_column(branch_cols, 4, 0);

		lv_obj_t* left_col = power_branch_column_create(branch_cols);
		lv_obj_t* right_col = power_branch_column_create(branch_cols);

		lbl_line_v[i] = tt_obj_label_color_create(left_col, "");
		lbl_line_c[i] = tt_obj_label_color_create(left_col, "");
		lbl_line_s[i] = tt_obj_label_color_create(left_col, "");
		lbl_line_ph[i] = tt_obj_label_color_create(left_col, "");
		lbl_line_e[i] = tt_obj_label_color_create(left_col, "");

		lbl_line_p[i] = tt_obj_label_color_create(right_col, "");
		lbl_line_q[i] = tt_obj_label_color_create(right_col, "");
		lbl_line_pf[i] = tt_obj_label_color_create(right_col, "");
		lbl_line_f[i] = tt_obj_label_color_create(right_col, "");
	}
}
