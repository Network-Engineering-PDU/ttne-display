#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "scr_settings_nw_modbus.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define MODBUS_ROW_HEIGHT 36
#define MODBUS_CONTROL_HEIGHT 32
/* Modbus RTU unit IDs: 0 is broadcast, 248-255 are reserved. */
#define MODBUS_ADDR_MIN 1
#define MODBUS_ADDR_MAX 247

/* Defaults used when a daisy chain field holds no usable value. */
#define DEFAULT_SCAN_FIRST 2
#define DEFAULT_SCAN_LAST 16

/* TCP has no baud/parity/stop bits; only RTU (RS-485, the "485" ports) does.
 * The "ACC" ports are a separate door/handle accessory interface, not
 * Modbus, so they are not offered here. */
static const char* BAUD_RATE_OPTIONS = "9600\n19200\n38400\n57600\n115200";
static const int BAUD_RATES[] = { 9600, 19200, 38400, 57600, 115200 };
#define BAUD_RATE_COUNT (sizeof(BAUD_RATES) / sizeof(BAUD_RATES[0]))

static const char* PARITY_OPTIONS = "None\nEven\nOdd";
static const char PARITIES[] = { 'N', 'E', 'O' };
#define PARITY_COUNT (sizeof(PARITIES) / sizeof(PARITIES[0]))

/* Slave answers requests on the 485 bus; Master polls the daisy chain. */
static const char* ROLE_OPTIONS = "Slave\nMaster";
#define ROLE_MASTER_INDEX 1

static const char* TIMEOUT_OPTIONS = "100 ms\n200 ms\n500 ms\n1000 ms";
static const int TIMEOUTS_MS[] = { 100, 200, 500, 1000 };
#define TIMEOUT_COUNT (sizeof(TIMEOUTS_MS) / sizeof(TIMEOUTS_MS[0]))

static const char* POLL_OPTIONS = "1 s\n2 s\n5 s\n10 s\n30 s";
static const int POLL_INTERVALS_S[] = { 1, 2, 5, 10, 30 };
#define POLL_COUNT (sizeof(POLL_INTERVALS_S) / sizeof(POLL_INTERVALS_S[0]))

static const char* OFFLINE_OPTIONS = "1 missed\n2 missed\n3 missed\n5 missed\n10 missed";
static const int OFFLINE_AFTER[] = { 1, 2, 3, 5, 10 };
#define OFFLINE_COUNT (sizeof(OFFLINE_AFTER) / sizeof(OFFLINE_AFTER[0]))

/* Global variables ***********************************************************/

static lv_obj_t* menu_handle;
static lv_obj_t* daisy_page;
static lv_obj_t* last_page;

/* Modbus page */
static lv_obj_t* cbx_ethernet_1;
static lv_obj_t* cbx_ethernet_2;
static lv_obj_t* cbx_rtu_port1;
static lv_obj_t* cbx_rtu_port2;
static lv_obj_t* lbl_port_note;
static lv_obj_t* dd_role;
static lv_obj_t* lbl_addr;
static lv_obj_t* txt_addr;
static lv_obj_t* dd_baud_rate;
static lv_obj_t* dd_parity;
static lv_obj_t* dd_stop_bits;
static lv_obj_t* btn_daisy;

/* Daisy chain page */
static lv_obj_t* txt_scan_first;
static lv_obj_t* txt_scan_last;
static lv_obj_t* dd_timeout;
static lv_obj_t* dd_poll;
static lv_obj_t* dd_offline;

/* Whether this board can switch each 485 jack on its own (read-only). */
static bool port_control;
static bool refresh_pending;
static bool save_pending;

/* Function prototypes ********************************************************/

static void refresh_cb(int err, void* userdata);
static void save_cb(int err, void* userdata);
static void update_rtu_controls(void);

/* Function definitions *******************************************************/

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

static lv_obj_t* create_section(lv_obj_t* parent, const char* title)
{
	lv_obj_t* section = tt_obj_cont_create(parent);
	lv_obj_clear_flag(section, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(section, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(section, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(section, 5, 0);
	lv_obj_set_style_pad_row(section, 3, 0);
	lv_obj_t* label = lv_label_create(section);
	lv_label_set_text(label, title);
	lv_obj_set_width(label, LV_PCT(100));
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
	return section;
}

static lv_obj_t* create_row_label(lv_obj_t* parent, const char* text)
{
	lv_obj_t* label = lv_label_create(parent);
	lv_label_set_text(label, text);
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
	lv_obj_set_flex_grow(label, 1);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
	return label;
}

static lv_obj_t* create_checkbox(lv_obj_t* parent, lv_event_cb_t cb)
{
	lv_obj_t* checkbox = tt_obj_checkbox_create(parent, "", cb);
	lv_obj_set_size(checkbox, 30, 30);
	lv_obj_set_style_border_width(checkbox, 2, LV_PART_INDICATOR);
	return checkbox;
}

static void create_toggle_row(lv_obj_t* parent, const char* label,
		lv_obj_t** checkbox, lv_event_cb_t cb)
{
	lv_obj_t* row = create_row(parent);
	create_row_label(row, label);
	*checkbox = create_checkbox(row, cb);
}

static void create_dropdown_row(lv_obj_t* parent, const char* label,
		const char* options, lv_obj_t** dropdown, lv_event_cb_t cb)
{
	lv_obj_t* row = create_row(parent);
	create_row_label(row, label);
	*dropdown = tt_obj_dropdown_create(row, (char*)options, cb);
	lv_obj_set_size(*dropdown, LV_PCT(55), MODBUS_CONTROL_HEIGHT);
}

static void number_txt_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		lv_obj_t* kb = scr_keyboard_create(lv_scr_act(),
				lv_event_get_target(e), KB_NUM);
		lv_scr_load(kb);
	}
}

static lv_obj_t* create_number_row(lv_obj_t* parent, const char* label,
		const char* placeholder, lv_obj_t** label_out)
{
	lv_obj_t* row = create_row(parent);
	lv_obj_t* lbl = create_row_label(row, label);
	if (label_out != NULL) {
		*label_out = lbl;
	}
	lv_obj_t* txt = tt_obj_txt_create(row, (char*)placeholder, number_txt_cb);
	lv_obj_set_size(txt, LV_PCT(55), MODBUS_CONTROL_HEIGHT);
	lv_textarea_set_one_line(txt, true);
	lv_textarea_set_max_length(txt, 3);
	lv_textarea_set_accepted_chars(txt, "0123456789");
	return txt;
}

static int index_of(const int* values, size_t count, int value, int fallback)
{
	for (size_t i = 0; i < count; i++) {
		if (values[i] == value) {
			return (int)i;
		}
	}
	return fallback;
}

static int parity_to_index(char parity)
{
	for (size_t i = 0; i < PARITY_COUNT; i++) {
		if (PARITIES[i] == parity) {
			return (int)i;
		}
	}
	return 0; /* default: None */
}

static bool is_checked(lv_obj_t* obj)
{
	return (lv_obj_get_state(obj) & LV_STATE_CHECKED) != 0;
}

static void set_checked(lv_obj_t* obj, bool checked)
{
	if (checked) {
		lv_obj_add_state(obj, LV_STATE_CHECKED);
	} else {
		lv_obj_clear_state(obj, LV_STATE_CHECKED);
	}
}

/* Disabled controls do not take touches; they are also faded so it is clear
 * they do nothing right now. */
static void set_enabled(lv_obj_t* obj, bool enabled)
{
	if (enabled) {
		lv_obj_clear_state(obj, LV_STATE_DISABLED);
		lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);
	} else {
		lv_obj_add_state(obj, LV_STATE_DISABLED);
		lv_obj_set_style_opa(obj, LV_OPA_40, 0);
	}
}

static void set_number_text(lv_obj_t* txt, int value)
{
	char text[8];
	snprintf(text, sizeof(text), "%d", value);
	lv_textarea_set_text(txt, text);
}

static int get_number_text(lv_obj_t* txt)
{
	const char* text = lv_textarea_get_text(txt);
	return text != NULL ? atoi(text) : 0;
}

static bool role_is_master(void)
{
	return lv_dropdown_get_selected(dd_role) == ROLE_MASTER_INDEX;
}

static void apply_daisy_snapshot(const app_state_modbus_t* modbus)
{
	set_number_text(txt_scan_first, modbus->scan_first);
	set_number_text(txt_scan_last, modbus->scan_last);
	lv_dropdown_set_selected(dd_timeout, (uint16_t)index_of(TIMEOUTS_MS,
			TIMEOUT_COUNT, modbus->timeout_ms, 1));
	lv_dropdown_set_selected(dd_poll, (uint16_t)index_of(POLL_INTERVALS_S,
			POLL_COUNT, modbus->poll_interval_s, 2));
	lv_dropdown_set_selected(dd_offline, (uint16_t)index_of(OFFLINE_AFTER,
			OFFLINE_COUNT, modbus->offline_after, 2));
}

static void apply_snapshot(void)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	if (!snapshot.modbus.valid) {
		return;
	}
	const app_state_modbus_t* modbus = &snapshot.modbus;
	port_control = modbus->rtu_port_control;
	set_checked(cbx_ethernet_1, modbus->tcp_eth1);
	set_checked(cbx_ethernet_2, modbus->tcp_eth2);
	set_checked(cbx_rtu_port1, modbus->rtu_port1);
	set_checked(cbx_rtu_port2, modbus->rtu_port2);
	lv_dropdown_set_selected(dd_role, modbus->master ? ROLE_MASTER_INDEX : 0);
	set_number_text(txt_addr, modbus->addr);
	lv_dropdown_set_selected(dd_baud_rate, (uint16_t)index_of(BAUD_RATES,
			BAUD_RATE_COUNT, modbus->baud_rate, BAUD_RATE_COUNT - 1));
	lv_dropdown_set_selected(dd_parity,
			(uint16_t)parity_to_index(modbus->parity));
	lv_dropdown_set_selected(dd_stop_bits,
			(uint16_t)(modbus->stop_bits == 2 ? 1 : 0));
	apply_daisy_snapshot(modbus);
	update_rtu_controls();
}

/* Keeps the RTU section consistent with the current choices:
 *  - the address is "PDU number" for a master, "Slave address" otherwise;
 *  - the Daisy chain button exists only for a master;
 *  - with both 485 ports off, the RTU settings have no effect, so they are
 *    disabled;
 *  - the port checkboxes work only on boards with one transceiver per jack. */
static void update_rtu_controls(void)
{
	bool master = role_is_master();
	bool rtu_used = !port_control || is_checked(cbx_rtu_port1)
			|| is_checked(cbx_rtu_port2);

	lv_label_set_text(lbl_addr, master ? "PDU number" : "Slave address");
	if (master) {
		lv_obj_clear_flag(btn_daisy, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(btn_daisy, LV_OBJ_FLAG_HIDDEN);
	}

	set_enabled(cbx_rtu_port1, port_control);
	set_enabled(cbx_rtu_port2, port_control);
	if (port_control) {
		lv_obj_add_flag(lbl_port_note, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_clear_flag(lbl_port_note, LV_OBJ_FLAG_HIDDEN);
	}

	set_enabled(dd_role, rtu_used);
	set_enabled(txt_addr, rtu_used);
	set_enabled(dd_baud_rate, rtu_used);
	set_enabled(dd_parity, rtu_used);
	set_enabled(dd_stop_bits, rtu_used);
	set_enabled(btn_daisy, rtu_used);
}

static void request_refresh(void)
{
	if (refresh_pending) {
		return;
	}
	refresh_pending = true;
	if (backend_modbus_refresh(refresh_cb, NULL) != 0) {
		refresh_pending = false;
	}
}

static void menu_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	lv_obj_t* target_page = lv_event_get_user_data(e);
	lv_obj_t* active_page = lv_menu_get_cur_main_page(
			lv_event_get_current_target(e));
	/* Coming back from the Daisy chain page must keep unsaved edits here
	 * (for example a Role just changed to Master), so only reload the
	 * settings when the page is opened from the Networks menu. */
	if (target_page == active_page && last_page != daisy_page) {
		request_refresh();
	}
	last_page = active_page;
}

static void daisy_menu_cb(lv_event_t* e)
{
	/* The Modbus page loads the settings for both pages. */
	(void)e;
}

static void port_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	if (!is_checked(lv_event_get_target(e)) && !role_is_master()) {
		tt_obj_info_box_create("Modbus",
				"PDUs connected to this jack will lose contact with the chain", 0);
	}
	update_rtu_controls();
}

static void role_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	if (role_is_master()) {
		tt_obj_info_box_create("Modbus",
				"This PDU will stop answering Modbus RTU on its 485 jacks. "
				"Modbus TCP keeps working.", 0);
	}
	update_rtu_controls();
}

/* Reads both pages into one settings record, so a Save on either page sends
 * everything the user changed on both. */
static void collect_settings(app_state_modbus_t* modbus)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);

	modbus->addr = get_number_text(txt_addr);
	modbus->baud_rate = BAUD_RATES[lv_dropdown_get_selected(dd_baud_rate)];
	modbus->parity = PARITIES[lv_dropdown_get_selected(dd_parity)];
	modbus->stop_bits = lv_dropdown_get_selected(dd_stop_bits) == 1 ? 2 : 1;
	modbus->tcp_eth1 = is_checked(cbx_ethernet_1);
	modbus->tcp_eth2 = is_checked(cbx_ethernet_2);
	modbus->rtu_port1 = is_checked(cbx_rtu_port1);
	modbus->rtu_port2 = is_checked(cbx_rtu_port2);
	modbus->rtu_port_control = port_control;
	modbus->master = role_is_master();
	modbus->scan_first = get_number_text(txt_scan_first);
	modbus->scan_last = get_number_text(txt_scan_last);
	modbus->timeout_ms = TIMEOUTS_MS[lv_dropdown_get_selected(dd_timeout)];
	modbus->poll_interval_s = POLL_INTERVALS_S[lv_dropdown_get_selected(dd_poll)];
	modbus->offline_after = OFFLINE_AFTER[lv_dropdown_get_selected(dd_offline)];
	modbus->valid = true;

	/* A slave does not use the scan range, so an empty or invalid field
	 * there must not block saving; keep the stored range instead. */
	if (!modbus->master && (modbus->scan_first < MODBUS_ADDR_MIN
			|| modbus->scan_last > MODBUS_ADDR_MAX
			|| modbus->scan_first > modbus->scan_last)) {
		bool stored = snapshot.modbus.valid;
		modbus->scan_first = stored ? snapshot.modbus.scan_first : DEFAULT_SCAN_FIRST;
		modbus->scan_last = stored ? snapshot.modbus.scan_last : DEFAULT_SCAN_LAST;
	}
}

/* Returns the message to show when the settings can't be saved, or NULL. */
static const char* validate_settings(const app_state_modbus_t* modbus)
{
	static char msg[96];

	if (modbus->addr < MODBUS_ADDR_MIN || modbus->addr > MODBUS_ADDR_MAX) {
		return modbus->master
				? "PDU number must be between 1 and 247"
				: "Slave address must be between 1 and 247";
	}
	if (!modbus->master) {
		return NULL;
	}
	if (port_control && !modbus->rtu_port1 && !modbus->rtu_port2) {
		return "Tick at least one 485 port so the master can reach the chain";
	}
	if (modbus->scan_first < MODBUS_ADDR_MIN
			|| modbus->scan_last > MODBUS_ADDR_MAX
			|| modbus->scan_first > modbus->scan_last) {
		return "Scan IDs must be between 1 and 247, with the first ID "
				"not above the last";
	}
	if (modbus->addr >= modbus->scan_first && modbus->addr <= modbus->scan_last) {
		snprintf(msg, sizeof(msg), "Scan range includes this PDU's own "
				"number (%d). Change one of them.", modbus->addr);
		return msg;
	}
	return NULL;
}

static void save_settings(void)
{
	if (save_pending) {
		return;
	}
	app_state_modbus_t modbus;
	collect_settings(&modbus);
	const char* error = validate_settings(&modbus);
	if (error != NULL) {
		tt_obj_info_box_create("Modbus", (char*)error, 1);
		return;
	}
	save_pending = true;
	if (backend_modbus_save(&modbus, save_cb, NULL) != 0) {
		save_pending = false;
		tt_obj_info_box_create("Modbus", "Can not apply Modbus settings", 1);
	}
}

static void ok_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		save_settings();
	}
}

static void cancel_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || save_pending) {
		return;
	}
	apply_snapshot();
	lv_event_send(lv_menu_get_main_header_back_btn(menu_handle),
			LV_EVENT_CLICKED, NULL);
}

/* Cancel on the Daisy chain page only undoes that page, then goes back to
 * the Modbus page with its own edits untouched. */
static void daisy_cancel_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || save_pending) {
		return;
	}
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	if (snapshot.modbus.valid) {
		apply_daisy_snapshot(&snapshot.modbus);
	}
	lv_event_send(lv_menu_get_main_header_back_btn(menu_handle),
			LV_EVENT_CLICKED, NULL);
}

static void refresh_cb(int err, void* userdata)
{
	(void)userdata;
	refresh_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("Modbus", "Can not load Modbus settings", 1);
		return;
	}
	apply_snapshot();
}

static void save_cb(int err, void* userdata)
{
	(void)userdata;
	save_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("Modbus", "Can not apply Modbus settings", 1);
		return;
	}
	apply_snapshot();
	tt_obj_info_box_create_timed("Modbus",
			"Settings saved. The Modbus service restarts.", 0, 5000);
}

/* Page body: a scrolling area for the settings above a fixed Cancel/Save
 * row, the layout every settings page here uses. */
static lv_obj_t* create_page_body(lv_obj_t* page)
{
	lv_obj_set_height(page, LV_PCT(100));
	lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(page, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_row(page, 3, 0);

	lv_obj_t* main = lv_obj_create(page);
	lv_obj_set_width(main, LV_PCT(100));
	lv_obj_set_height(main, 0);
	lv_obj_set_flex_grow(main, 1);
	lv_obj_add_flag(main, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scrollbar_mode(main, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_scroll_dir(main, LV_DIR_VER);
	lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(main, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_add_style(main, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(main, 3, 0);
	lv_obj_set_style_pad_row(main, 5, 0);
	return main;
}

static void create_button_row(lv_obj_t* page, lv_event_cb_t cancel,
		lv_event_cb_t save)
{
	lv_obj_t* button_row = create_row(page);
	lv_obj_set_height(button_row, 52);
	tt_obj_btn_perc_create(button_row, cancel, "Cancel", 47);
	tt_obj_btn_perc_create(button_row, save, "Save", 47);
}

static void create_daisy_page(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* page = tt_obj_menu_page_create(menu, btn, daisy_menu_cb,
			"Daisy chain");
	daisy_page = lv_obj_get_parent(page);
	lv_obj_t* main = create_page_body(page);

	lv_obj_t* scan_section = create_section(main, "SCAN");
	txt_scan_first = create_number_row(scan_section, "First ID", "1-247", NULL);
	txt_scan_last = create_number_row(scan_section, "Last ID", "1-247", NULL);

	lv_obj_t* poll_section = create_section(main, "POLLING");
	create_dropdown_row(poll_section, "Reply timeout", TIMEOUT_OPTIONS,
			&dd_timeout, NULL);
	create_dropdown_row(poll_section, "Poll every", POLL_OPTIONS,
			&dd_poll, NULL);
	create_dropdown_row(poll_section, "Offline after", OFFLINE_OPTIONS,
			&dd_offline, NULL);

	create_button_row(page, daisy_cancel_cb, ok_cb);

	set_number_text(txt_scan_first, DEFAULT_SCAN_FIRST);
	set_number_text(txt_scan_last, DEFAULT_SCAN_LAST);
	lv_dropdown_set_selected(dd_timeout, 1); /* 200 ms */
	lv_dropdown_set_selected(dd_poll, 2);    /* 5 s */
	lv_dropdown_set_selected(dd_offline, 2); /* 3 missed */
}

/* Public functions ***********************************************************/

void scr_settings_nw_modbus_create(lv_obj_t* menu, lv_obj_t* btn)
{
	menu_handle = menu;
	lv_obj_t* page = tt_obj_menu_page_create(menu, btn, menu_cb, "Modbus");
	lv_obj_t* main = create_page_body(page);

	lv_obj_t* tcp_section = create_section(main, "MODBUS TCP");
	create_toggle_row(tcp_section, "Ethernet 1", &cbx_ethernet_1, NULL);
	create_toggle_row(tcp_section, "Ethernet 2", &cbx_ethernet_2, NULL);

	lv_obj_t* rtu_section = create_section(main, "MODBUS RTU (RS-485)");
	create_toggle_row(rtu_section, "485 port 1", &cbx_rtu_port1, port_cb);
	create_toggle_row(rtu_section, "485 port 2", &cbx_rtu_port2, port_cb);
	lbl_port_note = lv_label_create(rtu_section);
	lv_label_set_text(lbl_port_note,
			"Switching each 485 port needs a board revision");
	lv_label_set_long_mode(lbl_port_note, LV_LABEL_LONG_WRAP);
	lv_obj_set_width(lbl_port_note, LV_PCT(100));
	lv_obj_set_style_text_font(lbl_port_note, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(lbl_port_note,
			lv_palette_main(LV_PALETTE_GREY), 0);
	create_dropdown_row(rtu_section, "Role", ROLE_OPTIONS, &dd_role, role_cb);
	txt_addr = create_number_row(rtu_section, "Slave address", "1-247",
			&lbl_addr);
	create_dropdown_row(rtu_section, "Baud rate", BAUD_RATE_OPTIONS,
			&dd_baud_rate, NULL);
	create_dropdown_row(rtu_section, "Parity", PARITY_OPTIONS, &dd_parity, NULL);
	create_dropdown_row(rtu_section, "Stop bits", "1\n2", &dd_stop_bits, NULL);
	btn_daisy = tt_obj_btn_create(rtu_section, NULL,
			"Daisy chain settings  " LV_SYMBOL_RIGHT, NULL, LV_PCT(100),
			MODBUS_ROW_HEIGHT, LV_ALIGN_CENTER);

	create_button_row(page, cancel_cb, ok_cb);

	/* Until the settings arrive, show the port checkboxes as on (a board
	 * without per-port control always has both jacks live). */
	set_checked(cbx_rtu_port1, true);
	set_checked(cbx_rtu_port2, true);

	create_daisy_page(menu, btn_daisy);

	apply_snapshot();
	update_rtu_controls();
}
