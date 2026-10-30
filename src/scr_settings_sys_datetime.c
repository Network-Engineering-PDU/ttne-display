/* System Setup > Date & Time: the time shown on the top bar of every page,
 * its time zone and 12/24-hour clock, automatic time (NTP), and setting the
 * time by hand while automatic time is off. Each change applies at once. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "i18n.h"
#include "scr_settings_sys_datetime.h"
#include "tt_clock.h"
#include "tt_colors.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define ZONE_TABLE "/usr/share/zoneinfo/zone1970.tab"
#define MAX_ZONES 512
#define UTC_ZONE "UTC"
#define CITY_OPTIONS_SIZE 8192
#define LABEL_WIDTH 42 /* % */
#define CONTROL_WIDTH 58 /* % */
#define SWITCH_LABEL_WIDTH 0 /* the room the switch leaves */
#define CONTROL_HEIGHT 34
#define UPDATE_PERIOD_MS 1000
#define NTP_POLL_PERIOD_S 10
#define ROLLER_ROWS 3
#define ROLLER_FONT_SIZE 14
#define DIALOG_BUTTON_HEIGHT 34

/* Regions of the tz database, in the order of the Region dropdown. */
static const struct {
	const char* prefix;
	const char* name;
} REGIONS[] = {
	{ "Africa", N_("Africa") },
	{ "America", N_("America") },
	{ "Antarctica", N_("Antarctica") },
	{ "Asia", N_("Asia") },
	{ "Atlantic", N_("Atlantic Ocean") },
	{ "Australia", N_("Australia") },
	{ "Europe", N_("Europe") },
	{ "Indian", N_("Indian Ocean") },
	{ "Pacific", N_("Pacific Ocean") },
	{ UTC_ZONE, UTC_ZONE },
};
#define REGION_COUNT ((int)(sizeof(REGIONS) / sizeof(REGIONS[0])))

static lv_obj_t* menu_handle;
static lv_obj_t* page_handle;
static lv_obj_t* lbl_time;
static lv_obj_t* lbl_date;
static lv_obj_t* lbl_offset;
static lv_obj_t* lbl_status;
static lv_obj_t* sw_ntp;
static lv_obj_t* dd_region;
static lv_obj_t* dd_city;
static lv_obj_t* dd_format;
static lv_obj_t* btn_set;

/* Time zones (sorted), and the zones of the City dropdown's region. */
static char zones[MAX_ZONES][APP_STATE_TZ_LEN];
static int zone_count;
static int region_of_options[REGION_COUNT]; /* Region dropdown index -> REGIONS */
static int region_option_count;
static int city_zones[MAX_ZONES]; /* City dropdown index -> zones */
static int city_count;
static char city_options[CITY_OPTIONS_SIZE];

static bool busy;
static int seconds_to_poll;

/* Set time dialog */
static lv_obj_t* dialog;
static lv_obj_t* roller_year;
static lv_obj_t* roller_month;
static lv_obj_t* roller_day;
static lv_obj_t* roller_hour;
static lv_obj_t* roller_minute;
static lv_obj_t* roller_ampm;

static void apply_snapshot(void);

/* Time zones *****************************************************************/

static int compare_zones(const void* a, const void* b)
{
	return strcmp((const char*)a, (const char*)b);
}

static void load_zones(void)
{
	zone_count = 0;
	FILE* table = fopen(ZONE_TABLE, "r");
	char line[256];
	while (table != NULL && zone_count < MAX_ZONES - 1 &&
			fgets(line, sizeof(line), table) != NULL) {
		if (line[0] == '#') {
			continue;
		}
		/* country codes, coordinates, zone, comments */
		char* zone = strchr(line, '\t');
		zone = zone != NULL ? strchr(zone + 1, '\t') : NULL;
		if (zone == NULL) {
			continue;
		}
		zone++;
		zone[strcspn(zone, "\t\r\n")] = '\0';
		if (zone[0] != '\0' && strlen(zone) < APP_STATE_TZ_LEN) {
			snprintf(zones[zone_count++], APP_STATE_TZ_LEN, "%s", zone);
		}
	}
	if (table != NULL) {
		fclose(table);
	}
	snprintf(zones[zone_count++], APP_STATE_TZ_LEN, "%s", UTC_ZONE);
	qsort(zones, (size_t)zone_count, sizeof(zones[0]), compare_zones);
}

/* Index in REGIONS of a zone, or -1. */
static int zone_region(const char* zone)
{
	for (int i = 0; i < REGION_COUNT; i++) {
		size_t len = strlen(REGIONS[i].prefix);
		if (strncmp(zone, REGIONS[i].prefix, len) == 0 &&
				(zone[len] == '/' || zone[len] == '\0')) {
			return i;
		}
	}
	return -1;
}

static int find_zone(const char* zone)
{
	for (int i = 0; i < zone_count; i++) {
		if (strcmp(zones[i], zone) == 0) {
			return i;
		}
	}
	return -1;
}

/* "America/Argentina/Buenos_Aires" -> "Argentina / Buenos Aires" */
static void append_city_name(const char* zone)
{
	const char* city = strchr(zone, '/');
	city = city != NULL ? city + 1 : zone;
	size_t len = strlen(city_options);
	if (len > 0 && len < sizeof(city_options) - 1) {
		city_options[len++] = '\n';
	}
	for (; *city != '\0' && len < sizeof(city_options) - 4; city++) {
		if (*city == '_') {
			city_options[len++] = ' ';
		} else if (*city == '/') {
			memcpy(city_options + len, " / ", 3);
			len += 3;
		} else {
			city_options[len++] = *city;
		}
	}
	city_options[len] = '\0';
}

static void fill_cities(int region)
{
	city_count = 0;
	city_options[0] = '\0';
	for (int i = 0; i < zone_count; i++) {
		if (zone_region(zones[i]) == region) {
			city_zones[city_count++] = i;
			append_city_name(zones[i]);
		}
	}
	lv_dropdown_set_options(dd_city, city_options);
}

static void fill_regions(void)
{
	static char options[512];
	options[0] = '\0';
	region_option_count = 0;
	for (int region = 0; region < REGION_COUNT; region++) {
		bool used = false;
		for (int i = 0; i < zone_count && !used; i++) {
			used = zone_region(zones[i]) == region;
		}
		if (!used) {
			continue;
		}
		if (region_option_count > 0) {
			strncat(options, "\n", sizeof(options) - strlen(options) - 1);
		}
		const char* name = strcmp(REGIONS[region].name, UTC_ZONE) == 0 ?
				UTC_ZONE : tr(REGIONS[region].name);
		strncat(options, name, sizeof(options) - strlen(options) - 1);
		region_of_options[region_option_count++] = region;
	}
	lv_dropdown_set_options(dd_region, options);
}

/* Shows the zone in the Region and City dropdowns. */
static void select_zone(const char* zone)
{
	int index = find_zone(zone);
	if (index < 0) {
		index = find_zone(UTC_ZONE);
	}
	int region = zone_region(zones[index]);
	for (int option = 0; option < region_option_count; option++) {
		if (region_of_options[option] == region) {
			lv_dropdown_set_selected(dd_region, (uint16_t)option);
		}
	}
	fill_cities(region);
	lv_dropdown_set_text(dd_city, NULL);
	for (int city = 0; city < city_count; city++) {
		if (city_zones[city] == index) {
			lv_dropdown_set_selected(dd_city, (uint16_t)city);
		}
	}
}

/* Status ********************************************************************/

static void show_time(void)
{
	struct tm local;
	char text[48];
	bool set = tt_clock_now(&local);
	if (set) {
		tt_clock_format_time(text, sizeof(text), &local, true);
		lv_label_set_text(lbl_time, text);
		char date[32];
		char offset[16];
		tt_clock_format_date(date, sizeof(date), &local);
		tt_clock_format_offset(offset, sizeof(offset), &local);
		lv_label_set_text(lbl_date, date);
		lv_label_set_text(lbl_offset, offset);
	} else {
		lv_label_set_text(lbl_time, "--:--");
		lv_label_set_text(lbl_date, "");
		lv_label_set_text(lbl_offset, "");
	}

	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	bool ntp = lv_obj_has_state(sw_ntp, LV_STATE_CHECKED);
	const char* status;
	uint32_t color;
	if (!set) {
		status = ntp ? tr("Time not set: waiting for the time server") :
				tr("Time not set: set it by hand");
		color = TT_COLOR_ERROR;
	} else if (ntp && snapshot.ntp.synchronized) {
		status = tr("Synchronized with the time server");
		color = TT_COLOR_GREEN_NE;
	} else if (ntp) {
		status = tr("Waiting for the time server");
		color = TT_COLOR_WARNING;
	} else {
		/* No backup battery for the clock. */
		status = tr("Set by hand: set it again after a power cut");
		color = 0xBBBBBB;
	}
	if (strcmp(lv_label_get_text(lbl_status), status) != 0) {
		lv_label_set_text(lbl_status, status);
	}
	lv_obj_set_style_text_color(lbl_status, lv_color_hex(color), 0);
}

static bool page_is_shown(void)
{
	return lv_menu_get_cur_main_page(menu_handle) ==
			lv_obj_get_parent(page_handle);
}

static void update_cb(lv_timer_t* timer)
{
	(void)timer;
	if (!page_is_shown()) {
		return;
	}
	show_time();
	if (--seconds_to_poll <= 0) {
		seconds_to_poll = NTP_POLL_PERIOD_S;
		backend_ntp_refresh(NULL, NULL);
	}
}

/* Saving *********************************************************************/

static void error_box(const char* msg)
{
	tt_obj_info_box_create(tr("Date & Time"), msg, 1);
}

static void refresh_cb(int err, void* userdata)
{
	(void)err;
	(void)userdata;
	if (page_is_shown() && !busy) {
		apply_snapshot();
	}
}

static void datetime_save_cb(int err, void* userdata)
{
	(void)userdata;
	busy = false;
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	if (err != 0) {
		error_box(tr("Can not save the date and time settings"));
	} else {
		/* The top bar applies it too, within a second. */
		tt_clock_apply(&snapshot.datetime);
	}
	apply_snapshot();
}

static void save_datetime(const char* timezone, int hour_format)
{
	busy = true;
	if (backend_datetime_save(timezone, hour_format, datetime_save_cb,
			NULL) != 0) {
		datetime_save_cb(1, NULL);
	}
}

static void ntp_save_cb(int err, void* userdata)
{
	(void)userdata;
	busy = false;
	if (err != 0) {
		error_box(tr("Can not apply NTP settings"));
	}
	/* The date and time status says whether NTP is on. */
	backend_datetime_refresh(refresh_cb, NULL);
	apply_snapshot();
}

static void ntp_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	bool enabled = lv_obj_has_state(sw_ntp, LV_STATE_CHECKED);
	/* Keep the NTP server and offset: they are only known once loaded. */
	if (!snapshot.ntp.valid || busy) {
		error_box(tr("Can not load NTP settings"));
		apply_snapshot();
		backend_ntp_refresh(refresh_cb, NULL);
		return;
	}
	busy = true;
	if (backend_ntp_save(enabled, snapshot.ntp.time_offset,
			snapshot.ntp.server, ntp_save_cb, NULL) != 0) {
		ntp_save_cb(1, NULL);
	}
}

static void region_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	int option = (int)lv_dropdown_get_selected(dd_region);
	fill_cities(region_of_options[option]);
	if (city_count == 1) {
		/* UTC */
		app_state_snapshot_t snapshot;
		app_state_get_snapshot(&snapshot);
		save_datetime(zones[city_zones[0]], snapshot.datetime.hour_format);
		return;
	}
	/* Nothing is saved until a city is chosen. */
	lv_dropdown_set_text(dd_city, tr("Choose a city"));
	lv_dropdown_open(dd_city);
}

static void city_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	int city = (int)lv_dropdown_get_selected(dd_city);
	if (city < 0 || city >= city_count) {
		return;
	}
	lv_dropdown_set_text(dd_city, NULL);
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	save_datetime(zones[city_zones[city]], snapshot.datetime.hour_format);
}

static void format_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	int hour_format = lv_dropdown_get_selected(dd_format) == 0 ? 24 : 12;
	save_datetime(snapshot.datetime.timezone, hour_format);
}

/* Set time dialog ***********************************************************/

static void close_dialog(void)
{
	if (dialog != NULL) {
		lv_obj_del(dialog);
		dialog = NULL;
	}
}

static bool leap_year(int year)
{
	return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static int days_in_month(int year, int month)
{
	static const int DAYS[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	return month == 2 && leap_year(year) ? 29 : DAYS[month - 1];
}

/* Options "first\n...\nlast", zero-padded to width digits. */
static void number_options(char* dst, size_t size, int first, int last,
		int width)
{
	dst[0] = '\0';
	for (int i = first; i <= last; i++) {
		size_t len = strlen(dst);
		snprintf(dst + len, size - len, "%s%0*d", i > first ? "\n" : "",
				width, i);
	}
}

static int selected_year(void)
{
	return TT_CLOCK_MIN_YEAR + (int)lv_roller_get_selected(roller_year);
}

static int selected_month(void)
{
	return 1 + (int)lv_roller_get_selected(roller_month);
}

static void fill_days(void)
{
	char options[128];
	int days = days_in_month(selected_year(), selected_month());
	int day = (int)lv_roller_get_selected(roller_day);
	number_options(options, sizeof(options), 1, days, 2);
	lv_roller_set_options(roller_day, options, LV_ROLLER_MODE_NORMAL);
	lv_roller_set_selected(roller_day, (uint16_t)LV_MIN(day, days - 1),
			LV_ANIM_OFF);
}

static void month_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		fill_days();
	}
}

static void clock_set_cb(int err, void* userdata)
{
	(void)userdata;
	busy = false;
	if (err != 0) {
		error_box(tr("Can not set the time"));
		return;
	}
	close_dialog();
	tt_obj_info_box_create_timed(tr("Date & Time"), tr("The time is set."), 0,
			3000);
	apply_snapshot();
}

static void dialog_set_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || busy) {
		return;
	}
	int hour = (int)lv_roller_get_selected(roller_hour);
	if (roller_ampm != NULL) {
		/* Hours 12, 1...11 */
		hour = hour % 12 + (lv_roller_get_selected(roller_ampm) == 1 ? 12 : 0);
	}
	char local_time[32];
	snprintf(local_time, sizeof(local_time), "%04d-%02d-%02d %02d:%02d",
			selected_year(), selected_month(),
			1 + (int)lv_roller_get_selected(roller_day), hour,
			(int)lv_roller_get_selected(roller_minute));
	busy = true;
	if (backend_clock_set(local_time, clock_set_cb, NULL) != 0) {
		clock_set_cb(1, NULL);
	}
}

static void dialog_cancel_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED && !busy) {
		close_dialog();
	}
}

static lv_obj_t* create_roller(lv_obj_t* parent, const char* options,
		int width, lv_event_cb_t cb)
{
	lv_obj_t* roller = lv_roller_create(parent);
	lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);
	lv_obj_set_width(roller, width);
	lv_obj_set_style_text_font(roller, i18n_font(ROLLER_FONT_SIZE), 0);
	lv_obj_set_style_text_line_space(roller, 4, 0);
	lv_obj_add_style(roller, &txt_style, LV_STATE_DEFAULT);
	/* After the font: the height is counted in its lines. */
	lv_roller_set_visible_row_count(roller, ROLLER_ROWS);
	lv_obj_set_style_bg_color(roller, lv_color_hex(TT_COLOR_GREEN_NE),
			LV_PART_SELECTED);
	lv_obj_set_style_text_color(roller, lv_color_black(), LV_PART_SELECTED);
	if (cb != NULL) {
		lv_obj_add_event_cb(roller, cb, LV_EVENT_VALUE_CHANGED, NULL);
	}
	return roller;
}

static lv_obj_t* create_dialog_row(lv_obj_t* parent, bool numbers)
{
	lv_obj_t* row = lv_obj_create(parent);
	lv_obj_add_style(row, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(row, 0, 0);
	lv_obj_set_style_pad_column(row, 3, 0);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
			LV_FLEX_ALIGN_CENTER);
	if (numbers) {
		/* Numbers read left to right in every language, as the date and
		 * time are written. */
		lv_obj_set_style_base_dir(row, LV_BASE_DIR_LTR, 0);
	}
	return row;
}

static void separator_create(lv_obj_t* parent, const char* text)
{
	lv_obj_t* label = lv_label_create(parent);
	lv_label_set_text(label, text);
	lv_obj_set_style_text_font(label, i18n_font(18), 0);
}

static void open_dialog(void)
{
	char options[512];
	struct tm local;
	if (!tt_clock_now(&local)) {
		memset(&local, 0, sizeof(local));
		local.tm_year = TT_CLOCK_MIN_YEAR - 1900;
		local.tm_mday = 1;
	}
	int year = LV_CLAMP(TT_CLOCK_MIN_YEAR, local.tm_year + 1900,
			TT_CLOCK_MAX_YEAR);

	close_dialog();
	/* Covers the page and keeps touches away from it. */
	dialog = lv_obj_create(lv_obj_get_screen(page_handle));
	lv_obj_remove_style_all(dialog);
	lv_obj_set_size(dialog, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_bg_color(dialog, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(dialog, LV_OPA_60, 0);
	lv_obj_add_flag(dialog, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_t* panel = lv_obj_create(dialog);
	lv_obj_add_style(panel, &cont_style, LV_STATE_DEFAULT);
	lv_obj_set_size(panel, LV_PCT(96), LV_SIZE_CONTENT);
	lv_obj_set_style_max_height(panel, LV_PCT(100), 0);
	lv_obj_center(panel);
	lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(panel, 4, 0);
	lv_obj_set_style_pad_row(panel, 4, 0);

	lv_obj_t* title = lv_label_create(panel);
	lv_label_set_text(title, tr("Set date and time"));

	lv_obj_t* date_row = create_dialog_row(panel, true);
	number_options(options, sizeof(options), TT_CLOCK_MIN_YEAR,
			TT_CLOCK_MAX_YEAR, 4);
	roller_year = create_roller(date_row, options, 64, month_cb);
	lv_roller_set_selected(roller_year, (uint16_t)(year - TT_CLOCK_MIN_YEAR),
			LV_ANIM_OFF);
	separator_create(date_row, "/");
	number_options(options, sizeof(options), 1, 12, 2);
	roller_month = create_roller(date_row, options, 48, month_cb);
	lv_roller_set_selected(roller_month, (uint16_t)local.tm_mon, LV_ANIM_OFF);
	separator_create(date_row, "/");
	roller_day = create_roller(date_row, "01", 48, NULL);
	lv_roller_set_selected(roller_day, 0, LV_ANIM_OFF);
	fill_days();
	lv_roller_set_selected(roller_day, (uint16_t)(local.tm_mday - 1),
			LV_ANIM_OFF);

	lv_obj_t* time_row = create_dialog_row(panel, true);
	if (tt_clock_hour_format() == 12) {
		snprintf(options, sizeof(options), "12\n");
		number_options(options + 3, sizeof(options) - 3, 1, 11, 1);
		roller_hour = create_roller(time_row, options, 48, NULL);
		lv_roller_set_selected(roller_hour, (uint16_t)(local.tm_hour % 12),
				LV_ANIM_OFF);
	} else {
		number_options(options, sizeof(options), 0, 23, 2);
		roller_hour = create_roller(time_row, options, 48, NULL);
		lv_roller_set_selected(roller_hour, (uint16_t)local.tm_hour,
				LV_ANIM_OFF);
	}
	separator_create(time_row, ":");
	number_options(options, sizeof(options), 0, 59, 2);
	roller_minute = create_roller(time_row, options, 48, NULL);
	lv_roller_set_selected(roller_minute, (uint16_t)local.tm_min, LV_ANIM_OFF);
	roller_ampm = NULL;
	if (tt_clock_hour_format() == 12) {
		snprintf(options, sizeof(options), "%s\n%s", tr("AM"), tr("PM"));
		roller_ampm = create_roller(time_row, options, 64, NULL);
		lv_roller_set_selected(roller_ampm, local.tm_hour < 12 ? 0 : 1,
				LV_ANIM_OFF);
	}

	lv_obj_t* buttons = create_dialog_row(panel, false);
	/* As the other pages' OK and Cancel (SPACE_EVENLY lays out right to
	 * left past the edge). */
	lv_obj_set_flex_align(buttons, LV_FLEX_ALIGN_SPACE_BETWEEN,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_t* btn = tt_obj_btn_perc_create(buttons, dialog_set_cb, tr("Set"), 47);
	lv_obj_set_height(btn, DIALOG_BUTTON_HEIGHT);
	btn = tt_obj_btn_perc_create(buttons, dialog_cancel_cb, tr("Cancel"), 47);
	lv_obj_set_height(btn, DIALOG_BUTTON_HEIGHT);
}

static void set_time_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED &&
			!lv_obj_has_state(sw_ntp, LV_STATE_CHECKED)) {
		open_dialog();
	}
}

/* Page ***********************************************************************/

static void apply_snapshot(void)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	bool ntp = snapshot.ntp.valid ? snapshot.ntp.enabled :
			snapshot.datetime.ntp_enabled;
	if (ntp) {
		lv_obj_add_state(sw_ntp, LV_STATE_CHECKED);
		lv_obj_add_state(btn_set, LV_STATE_DISABLED);
	} else {
		lv_obj_clear_state(sw_ntp, LV_STATE_CHECKED);
		lv_obj_clear_state(btn_set, LV_STATE_DISABLED);
	}
	select_zone(snapshot.datetime.timezone);
	lv_dropdown_set_selected(dd_format,
			snapshot.datetime.hour_format == 12 ? 1 : 0);
	show_time();
}

static void menu_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
		return;
	}
	if (!page_is_shown()) {
		close_dialog();
		return;
	}
	apply_snapshot();
	seconds_to_poll = NTP_POLL_PERIOD_S;
	backend_ntp_refresh(refresh_cb, NULL);
	backend_datetime_refresh(refresh_cb, NULL);
}

static lv_obj_t* create_row(lv_obj_t* parent, const char* label_text,
		int label_width)
{
	lv_obj_t* row = lv_obj_create(parent);
	lv_obj_add_style(row, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(row, 2, 0);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* label = lv_label_create(row);
	i18n_label_set_text(label, label_text);
	if (label_width > 0) {
		lv_obj_set_width(label, LV_PCT(label_width));
	} else {
		/* The room the control leaves */
		lv_obj_set_width(label, 0);
		lv_obj_set_flex_grow(label, 1);
	}
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_AUTO, 0);
	return row;
}

static lv_obj_t* create_dropdown(lv_obj_t* row, const char* options,
		lv_event_cb_t cb)
{
	lv_obj_t* dd = tt_obj_dropdown_create(row, options, NULL);
	lv_obj_set_size(dd, LV_PCT(CONTROL_WIDTH), CONTROL_HEIGHT);
	lv_obj_add_event_cb(dd, cb, LV_EVENT_VALUE_CHANGED, NULL);
	return dd;
}

void scr_settings_sys_datetime_create(lv_obj_t* menu, lv_obj_t* btn)
{
	menu_handle = menu;
	page_handle = tt_obj_menu_page_create(menu, btn, menu_cb,
			tr("Date & Time"));
	lv_obj_set_flex_flow(page_handle, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(page_handle, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_row(page_handle, 2, 0);

	lv_obj_t* now = tt_obj_cont_create(page_handle);
	lv_obj_set_flex_flow(now, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(now, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_row(now, 0, 0);
	lbl_time = lv_label_create(now);
	lv_obj_set_style_text_font(lbl_time, i18n_font(28), 0);
	lv_label_set_text(lbl_time, "");
	lv_obj_t* date_row = lv_obj_create(now);
	lv_obj_add_style(date_row, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_set_size(date_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_clear_flag(date_row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(date_row, LV_FLEX_FLOW_ROW);
	lv_obj_set_style_pad_column(date_row, 14, 0);
	lbl_date = lv_label_create(date_row);
	lv_label_set_text(lbl_date, "");
	/* "UTC+02:00" is a code: ASCII digits in every language */
	lbl_offset = lv_label_create(date_row);
	lv_label_set_text(lbl_offset, "");
	i18n_western_digits(lbl_offset);
	lbl_status = lv_label_create(now);
	lv_obj_set_width(lbl_status, LV_PCT(100));
	lv_label_set_long_mode(lbl_status, LV_LABEL_LONG_SCROLL);
	lv_obj_set_style_text_align(lbl_status, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(lbl_status, "");

	lv_obj_t* row = create_row(page_handle, tr("Automatic time (NTP)"),
			SWITCH_LABEL_WIDTH);
	lv_obj_set_style_pad_column(row, 6, 0);
	sw_ntp = lv_switch_create(row);
	lv_obj_set_size(sw_ntp, 56, 30);
	lv_obj_set_style_bg_color(sw_ntp, lv_color_hex(TT_COLOR_GREEN_NE),
			LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_add_event_cb(sw_ntp, ntp_cb, LV_EVENT_VALUE_CHANGED, NULL);

	row = create_row(page_handle, tr("Region"), LABEL_WIDTH);
	dd_region = create_dropdown(row, "", region_cb);
	row = create_row(page_handle, tr("City"), LABEL_WIDTH);
	dd_city = create_dropdown(row, "", city_cb);
	row = create_row(page_handle, tr("Time format"), LABEL_WIDTH);
	char options[64];
	snprintf(options, sizeof(options), "%s\n%s", tr("24-hour"), tr("12-hour"));
	dd_format = create_dropdown(row, options, format_cb);

	btn_set = tt_obj_btn_perc_create(page_handle, set_time_cb,
			tr("Set date and time"), 100);
	lv_obj_set_height(btn_set, 40);
	lv_obj_set_style_bg_opa(btn_set, LV_OPA_40, LV_STATE_DISABLED);
	lv_obj_set_style_text_opa(btn_set, LV_OPA_40, LV_STATE_DISABLED);

	load_zones();
	fill_regions();
	apply_snapshot();
	lv_timer_create(update_cb, UPDATE_PERIOD_MS, NULL);
}
