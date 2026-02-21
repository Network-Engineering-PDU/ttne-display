#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"

#include "scr_settings_vis.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"
#include "config.h"

/* Global variables ***********************************************************/

static lv_obj_t* dd_rotation;
static lv_obj_t* txt_screen_saver;

/* PDU location information variables */
static lv_obj_t* txt_company;
static lv_obj_t* txt_rack;
static lv_obj_t* txt_sys_ab;
static lv_obj_t* txt_ups_ab;
static lv_obj_t* txt_elec;
static lv_obj_t* txt_circuit;
static lv_obj_t* txt_service;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void rotate_cb(lv_event_t* e);
static void txt_inactivity_cb(lv_event_t* e);
static void txt_pdu_info_cb(lv_event_t* e);
static void update_pdu_info_display();
static void save_pdu_info_field(int field_id, const char* value);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(menu);
		if (curr_page == page) {
			// Update display settings when page is shown
			int rotation = config_get_rotation();
			lv_dropdown_set_selected(dd_rotation, rotation);
			
			char inactivity_time_str[10];
			sprintf(inactivity_time_str, "%d", config_get_inactivity_time());
			lv_textarea_set_text(txt_screen_saver, inactivity_time_str);
			
			// Update PDU info fields
			update_pdu_info_display();
		}
	}
}

static void rotate_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		// TODO: Handle rotation change with confirmation dialog
		(void)dd_rotation; // Avoid unused variable warning
	}
}

static void txt_inactivity_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_NUM);
		lv_scr_load(kb_scr);
	}
	if (code == LV_EVENT_READY) {
		int t = atoi(lv_textarea_get_text(txt_screen_saver));
		if (t < 1 || t > 300) {
			tt_obj_info_box_create("ERROR",
					"Screensaver time must be in the interval [1-300] (minutes)", 1);
			return;
		}
		config_set_inactivity_time(t);
	}
}

static void txt_pdu_info_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);
	int field_id = (int)(uintptr_t)lv_event_get_user_data(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_ABC);
		lv_scr_load(kb_scr);
	}
	if (code == LV_EVENT_READY) {
		const char* value = lv_textarea_get_text(obj);
		save_pdu_info_field(field_id, value);
	}
}

static void update_pdu_info_display()
{
	// TODO: Load PDU info from config/models and update text fields
	// Example:
	// const char* company = config_get_pdu_company_name();
	// lv_textarea_set_text(txt_company, company);
	// ... repeat for other fields
}

static void save_pdu_info_field(int field_id, const char* value)
{
	// TODO: Save PDU info field to config based on field_id
	// Example:
	// if (field_id == 0) config_set_pdu_company_name(value);
	// if (field_id == 1) config_set_pdu_rack(value);
	// ... repeat for other fields
	(void)field_id;
	(void)value;
}

/* Function definitions *******************************************************/

void scr_settings_vis_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* vis_page = tt_obj_menu_page_create(menu, btn, menu_cb, "Visualization");

	lv_obj_t* cont = tt_obj_cont_create(vis_page);

	/* Two-column layout: labels on left, inputs on right */
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_width(cont, LV_PCT(100));

	/* Screen rotation setting */
	lv_obj_t* rot_cont = tt_obj_cont_create(cont);
	lv_obj_set_flex_flow(rot_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_width(rot_cont, LV_PCT(100));

	tt_obj_label_create(rot_cont, "Screen rotation");
	char* rotation_options = "0 deg\n90 deg\n180 deg\n270 deg";
	dd_rotation = tt_obj_dropdown_create(rot_cont, rotation_options, rotate_cb);
	int rotation = config_get_rotation();
	lv_dropdown_set_selected(dd_rotation, rotation);

	/* Screen saver timeout setting */
	lv_obj_t* saver_cont = tt_obj_cont_create(cont);
	lv_obj_set_flex_flow(saver_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_width(saver_cont, LV_PCT(100));

	tt_obj_label_create(saver_cont, "Screen saver (s)");
	txt_screen_saver = tt_obj_txt_create(saver_cont, "Time in seconds",
			txt_inactivity_cb);
	char inactivity_time_str[10];
	sprintf(inactivity_time_str, "%d", config_get_inactivity_time());
	lv_textarea_set_text(txt_screen_saver, inactivity_time_str);

	/* PDU Location Information Section */
	lv_obj_t* pdu_info_label = tt_obj_label_create(cont, "PDU location information");
	lv_obj_set_style_text_font(pdu_info_label, &lv_font_montserrat_14, 0);

	/* Company name */
	lv_obj_t* company_cont = tt_obj_cont_create(cont);
	lv_obj_set_flex_flow(company_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_width(company_cont, LV_PCT(100));
	tt_obj_label_create(company_cont, "Company name");
	txt_company = tt_obj_txt_create(company_cont, "", txt_pdu_info_cb);
	lv_obj_add_event_cb(txt_company, txt_pdu_info_cb, LV_EVENT_ALL, (void*)(uintptr_t)0);

	/* Rack */
	lv_obj_t* rack_cont = tt_obj_cont_create(cont);
	lv_obj_set_flex_flow(rack_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_width(rack_cont, LV_PCT(100));
	tt_obj_label_create(rack_cont, "Rack");
	txt_rack = tt_obj_txt_create(rack_cont, "", txt_pdu_info_cb);
	lv_obj_add_event_cb(txt_rack, txt_pdu_info_cb, LV_EVENT_ALL, (void*)(uintptr_t)1);

	/* System A - B */
	lv_obj_t* sys_ab_cont = tt_obj_cont_create(cont);
	lv_obj_set_flex_flow(sys_ab_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_width(sys_ab_cont, LV_PCT(100));
	tt_obj_label_create(sys_ab_cont, "System A - B");
	txt_sys_ab = tt_obj_txt_create(sys_ab_cont, "", txt_pdu_info_cb);
	lv_obj_add_event_cb(txt_sys_ab, txt_pdu_info_cb, LV_EVENT_ALL, (void*)(uintptr_t)2);

	/* UPS A - B */
	lv_obj_t* ups_ab_cont = tt_obj_cont_create(cont);
	lv_obj_set_flex_flow(ups_ab_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_width(ups_ab_cont, LV_PCT(100));
	tt_obj_label_create(ups_ab_cont, "UPS A - B");
	txt_ups_ab = tt_obj_txt_create(ups_ab_cont, "", txt_pdu_info_cb);
	lv_obj_add_event_cb(txt_ups_ab, txt_pdu_info_cb, LV_EVENT_ALL, (void*)(uintptr_t)3);

	/* Electrical board */
	lv_obj_t* elec_cont = tt_obj_cont_create(cont);
	lv_obj_set_flex_flow(elec_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_width(elec_cont, LV_PCT(100));
	tt_obj_label_create(elec_cont, "Electrical board");
	txt_elec = tt_obj_txt_create(elec_cont, "", txt_pdu_info_cb);
	lv_obj_add_event_cb(txt_elec, txt_pdu_info_cb, LV_EVENT_ALL, (void*)(uintptr_t)4);

	/* Circuit Breaker */
	lv_obj_t* circuit_cont = tt_obj_cont_create(cont);
	lv_obj_set_flex_flow(circuit_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_width(circuit_cont, LV_PCT(100));
	tt_obj_label_create(circuit_cont, "Circuit Breaker");
	txt_circuit = tt_obj_txt_create(circuit_cont, "", txt_pdu_info_cb);
	lv_obj_add_event_cb(txt_circuit, txt_pdu_info_cb, LV_EVENT_ALL, (void*)(uintptr_t)5);

	/* Service */
	lv_obj_t* service_cont = tt_obj_cont_create(cont);
	lv_obj_set_flex_flow(service_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_width(service_cont, LV_PCT(100));
	tt_obj_label_create(service_cont, "Service");
	txt_service = tt_obj_txt_create(service_cont, "", txt_pdu_info_cb);
	lv_obj_add_event_cb(txt_service, txt_pdu_info_cb, LV_EVENT_ALL, (void*)(uintptr_t)6);
}