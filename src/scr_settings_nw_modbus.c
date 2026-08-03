#include <stdbool.h>
#include <stdint.h>

#include "lvgl/lvgl.h"
#include "scr_settings_nw_modbus.h"
#include "tt_obj.h"
#include "tt_styles.h"

#define MODBUS_PANEL_HEIGHT 96
#define MODBUS_ROW_HEIGHT 29
#define MODBUS_CONTROL_HEIGHT 27

typedef struct {
	bool ethernet_1;
	bool ethernet_2;
	bool acc_1;
	bool acc_2;
	uint16_t tcp_baud_rate;
	uint16_t tcp_parity;
	uint16_t tcp_stop_bits;
	uint16_t rtu_baud_rate;
	uint16_t rtu_parity;
	uint16_t rtu_stop_bits;
} modbus_screen_settings_t;

static lv_obj_t* menu_handle;
static lv_obj_t* cbx_ethernet_1;
static lv_obj_t* cbx_ethernet_2;
static lv_obj_t* cbx_acc_1;
static lv_obj_t* cbx_acc_2;
static lv_obj_t* dd_tcp_baud_rate;
static lv_obj_t* dd_tcp_parity;
static lv_obj_t* dd_tcp_stop_bits;
static lv_obj_t* dd_rtu_baud_rate;
static lv_obj_t* dd_rtu_parity;
static lv_obj_t* dd_rtu_stop_bits;

/* Screen-local defaults until the Modbus API and hardware mapping are added. */
static modbus_screen_settings_t saved_settings = {
	.ethernet_1 = true,
	.ethernet_2 = true,
	.acc_1 = true,
	.acc_2 = false,
	.tcp_baud_rate = 4,
	.tcp_parity = 0,
	.tcp_stop_bits = 0,
	.rtu_baud_rate = 4,
	.rtu_parity = 0,
	.rtu_stop_bits = 0,
};

static lv_obj_t* create_row(lv_obj_t* parent)
{
	lv_obj_t* row = lv_obj_create(parent);
	lv_obj_set_size(row, LV_PCT(100), MODBUS_ROW_HEIGHT);
	lv_obj_add_style(row, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(row, 0, 0);
	lv_obj_set_style_pad_column(row, 3, 0);
	return row;
}

static lv_obj_t* create_panel(lv_obj_t* parent)
{
	lv_obj_t* panel = lv_obj_create(parent);
	lv_obj_set_size(panel, LV_PCT(100), MODBUS_PANEL_HEIGHT);
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(panel, 2, 0);
	lv_obj_set_style_border_color(panel, lv_color_white(), 0);
	lv_obj_set_style_radius(panel, 16, 0);
	lv_obj_set_style_pad_all(panel, 3, 0);
	lv_obj_set_style_pad_row(panel, 0, 0);
	return panel;
}

static lv_obj_t* create_label(lv_obj_t* parent, const char* text, int width)
{
	lv_obj_t* label = lv_label_create(parent);
	lv_label_set_text(label, text);
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
	lv_obj_set_width(label, LV_PCT(width));
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
	return label;
}

static lv_obj_t* create_checkbox(lv_obj_t* parent)
{
	lv_obj_t* checkbox = tt_obj_checkbox_create(parent, "", NULL);
	lv_obj_set_size(checkbox, 30, 30);
	lv_obj_set_style_border_width(checkbox, 2, LV_PART_INDICATOR);
	return checkbox;
}

static lv_obj_t* create_toggle_group(lv_obj_t* parent, const char* label,
		lv_obj_t** checkbox)
{
	lv_obj_t* group = lv_obj_create(parent);
	lv_obj_set_size(group, LV_PCT(38), LV_PCT(100));
	lv_obj_add_style(group, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(group, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(group, LV_FLEX_ALIGN_SPACE_BETWEEN,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(group, 0, 0);
	create_label(group, label, 68);
	*checkbox = create_checkbox(group);
	return group;
}

static lv_obj_t* create_dropdown_group(lv_obj_t* parent, const char* label,
		const char* options, lv_obj_t** dropdown)
{
	lv_obj_t* group = lv_obj_create(parent);
	lv_obj_set_size(group, LV_PCT(49), LV_PCT(100));
	lv_obj_add_style(group, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(group, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(group, LV_FLEX_ALIGN_SPACE_BETWEEN,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(group, 0, 0);
	lv_obj_set_style_pad_column(group, 2, 0);
	create_label(group, label, 48);
	*dropdown = tt_obj_dropdown_create(group, (char*)options, NULL);
	lv_obj_set_size(*dropdown, LV_PCT(51), MODBUS_CONTROL_HEIGHT);
	return group;
}

static void set_checked(lv_obj_t* checkbox, bool checked)
{
	if (checked) {
		lv_obj_add_state(checkbox, LV_STATE_CHECKED);
	} else {
		lv_obj_clear_state(checkbox, LV_STATE_CHECKED);
	}
}

static bool is_checked(lv_obj_t* checkbox)
{
	return (lv_obj_get_state(checkbox) & LV_STATE_CHECKED) != 0;
}

static void apply_saved_settings(void)
{
	set_checked(cbx_ethernet_1, saved_settings.ethernet_1);
	set_checked(cbx_ethernet_2, saved_settings.ethernet_2);
	set_checked(cbx_acc_1, saved_settings.acc_1);
	set_checked(cbx_acc_2, saved_settings.acc_2);
	lv_dropdown_set_selected(dd_tcp_baud_rate,
			saved_settings.tcp_baud_rate);
	lv_dropdown_set_selected(dd_tcp_parity, saved_settings.tcp_parity);
	lv_dropdown_set_selected(dd_tcp_stop_bits,
			saved_settings.tcp_stop_bits);
	lv_dropdown_set_selected(dd_rtu_baud_rate,
			saved_settings.rtu_baud_rate);
	lv_dropdown_set_selected(dd_rtu_parity, saved_settings.rtu_parity);
	lv_dropdown_set_selected(dd_rtu_stop_bits,
			saved_settings.rtu_stop_bits);
}

static void menu_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	lv_obj_t* target_page = lv_event_get_user_data(e);
	lv_obj_t* active_page = lv_menu_get_cur_main_page(
			lv_event_get_current_target(e));
	if (target_page == active_page) {
		apply_saved_settings();
	}
}

static void ok_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	saved_settings.ethernet_1 = is_checked(cbx_ethernet_1);
	saved_settings.ethernet_2 = is_checked(cbx_ethernet_2);
	saved_settings.acc_1 = is_checked(cbx_acc_1);
	saved_settings.acc_2 = is_checked(cbx_acc_2);
	saved_settings.tcp_baud_rate = lv_dropdown_get_selected(
			dd_tcp_baud_rate);
	saved_settings.tcp_parity = lv_dropdown_get_selected(dd_tcp_parity);
	saved_settings.tcp_stop_bits = lv_dropdown_get_selected(
			dd_tcp_stop_bits);
	saved_settings.rtu_baud_rate = lv_dropdown_get_selected(
			dd_rtu_baud_rate);
	saved_settings.rtu_parity = lv_dropdown_get_selected(dd_rtu_parity);
	saved_settings.rtu_stop_bits = lv_dropdown_get_selected(
			dd_rtu_stop_bits);
	lv_event_send(lv_menu_get_main_header_back_btn(menu_handle),
			LV_EVENT_CLICKED, NULL);
}

static void cancel_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	apply_saved_settings();
	lv_event_send(lv_menu_get_main_header_back_btn(menu_handle),
			LV_EVENT_CLICKED, NULL);
}

static void create_protocol_panel(lv_obj_t* main, const char* title,
		const char* port_1_label, const char* port_2_label,
		lv_obj_t** port_1, lv_obj_t** port_2, lv_obj_t** baud_rate,
		lv_obj_t** parity, lv_obj_t** stop_bits)
{
	lv_obj_t* panel = create_panel(main);
	lv_obj_t* row = create_row(panel);
	create_label(row, title, 20);
	create_toggle_group(row, port_1_label, port_1);
	create_toggle_group(row, port_2_label, port_2);

	row = create_row(panel);
	create_dropdown_group(row, "Baud rate",
			"9600\n19200\n38400\n57600\n115200", baud_rate);
	create_dropdown_group(row, "Parity", "None\nEven\nOdd", parity);

	row = create_row(panel);
	create_dropdown_group(row, "Stop bits", "1\n2", stop_bits);
}

void scr_settings_nw_modbus_create(lv_obj_t* menu, lv_obj_t* btn)
{
	menu_handle = menu;
	lv_obj_t* page = tt_obj_menu_page_create(menu, btn, menu_cb, "Modbus");
	lv_obj_t* main = tt_obj_cont_create(page);
	lv_obj_set_width(main, LV_PCT(100));
	lv_obj_set_height(main, 0);
	lv_obj_set_flex_grow(main, 1);
	lv_obj_add_flag(main, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scrollbar_mode(main, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(main, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(main, 3, 0);
	lv_obj_set_style_pad_row(main, 3, 0);

	create_protocol_panel(main, "TCP /", "Ethernet 1", "Ethernet 2",
			&cbx_ethernet_1, &cbx_ethernet_2, &dd_tcp_baud_rate,
			&dd_tcp_parity, &dd_tcp_stop_bits);
	create_protocol_panel(main, "RT 485", "ACC-1", "ACC-2",
			&cbx_acc_1, &cbx_acc_2, &dd_rtu_baud_rate,
			&dd_rtu_parity, &dd_rtu_stop_bits);

	lv_obj_t* spacer = lv_obj_create(main);
	lv_obj_set_size(spacer, LV_PCT(100), 0);
	lv_obj_set_flex_grow(spacer, 1);
	lv_obj_add_style(spacer, &invisible_cont_style, LV_STATE_DEFAULT);

	lv_obj_t* button_row = create_row(main);
	lv_obj_set_height(button_row, 48);
	tt_obj_btn_perc_create(button_row, ok_cb, "OK", 47);
	tt_obj_btn_perc_create(button_row, cancel_cb, "Cancel", 47);

	apply_saved_settings();
}
