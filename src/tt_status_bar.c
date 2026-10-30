#include "tt_status_bar.h"

#include <stdio.h>
#include <string.h>

#include "i18n.h"
#include "tt_clock.h"
#include "tt_colors.h"
#include "tt_styles.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define UPDATE_PERIOD_MS 1000
/* Seconds between Wi-Fi signal requests, and between date and time
 * settings requests until the API answers (it may start after the
 * display). */
#define POLL_PERIOD_S 10

#define WIFI_WIDTH 24
#define WIFI_HEIGHT 18
#define WIFI_BARS 3
#define WIFI_ARC_WIDTH 3
#define WIFI_COLOR_OFF 0x5A5A5A

static lv_obj_t* lbl_time;
static lv_obj_t* wifi;
static lv_obj_t* wifi_bars[WIFI_BARS];
static lv_obj_t* wifi_dot;
static lv_obj_t* wifi_cross;
static uint32_t datetime_revision;
static uint32_t wifi_revision;
static bool datetime_pending;
static bool wifi_pending;
static int seconds_to_poll;
static char shown_time[40];

static void datetime_cb(int err, void* userdata)
{
	(void)err;
	(void)userdata;
	datetime_pending = false;
}

static void wifi_cb(int err, void* userdata)
{
	(void)err;
	(void)userdata;
	wifi_pending = false;
}

/* Bars lit for a signal in dBm. */
static int wifi_level(int signal)
{
	if (signal == 0 || signal >= -60) {
		return 3;
	}
	return signal >= -70 ? 2 : 1;
}

static void show_wifi(const app_state_wifi_t* state)
{
	if (!state->valid || !state->enabled) {
		lv_obj_add_flag(wifi, LV_OBJ_FLAG_HIDDEN);
		return;
	}
	lv_obj_clear_flag(wifi, LV_OBJ_FLAG_HIDDEN);
	int level = state->connected ? wifi_level(state->signal) : 0;
	lv_color_t on = lv_color_hex(TT_COLOR_GREEN_NE);
	lv_color_t off = lv_color_hex(WIFI_COLOR_OFF);
	for (int i = 0; i < WIFI_BARS; i++) {
		lv_obj_set_style_arc_color(wifi_bars[i], i < level ? on : off,
				LV_PART_MAIN);
	}
	lv_obj_set_style_bg_color(wifi_dot, state->connected ? on : off, 0);
	if (state->connected) {
		lv_obj_add_flag(wifi_cross, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_clear_flag(wifi_cross, LV_OBJ_FLAG_HIDDEN);
	}
}

static void show_time(void)
{
	struct tm local;
	char text[sizeof(shown_time)];
	if (tt_clock_now(&local)) {
		tt_clock_format_time(text, sizeof(text), &local, false);
	} else {
		snprintf(text, sizeof(text), "--:--");
	}
	if (strcmp(text, shown_time) != 0) {
		snprintf(shown_time, sizeof(shown_time), "%s", text);
		lv_label_set_text(lbl_time, shown_time);
	}
}

/* Callbacks can come before the request functions return (simulator). */
static void request_datetime(void)
{
	datetime_pending = true;
	if (backend_datetime_refresh(datetime_cb, NULL) != 0) {
		datetime_pending = false;
	}
}

static void request_wifi(void)
{
	wifi_pending = true;
	if (backend_wifi_status_refresh(wifi_cb, NULL) != 0) {
		wifi_pending = false;
	}
}

static void update_cb(lv_timer_t* timer)
{
	(void)timer;
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);

	if (snapshot.datetime_revision != datetime_revision) {
		datetime_revision = snapshot.datetime_revision;
		tt_clock_apply(&snapshot.datetime);
		shown_time[0] = '\0';
	}
	if (snapshot.wifi_revision != wifi_revision) {
		wifi_revision = snapshot.wifi_revision;
		show_wifi(&snapshot.wifi);
	}
	show_time();

	if (--seconds_to_poll > 0) {
		return;
	}
	seconds_to_poll = POLL_PERIOD_S;
	if (!snapshot.datetime.valid && !datetime_pending) {
		request_datetime();
	}
	if (!wifi_pending) {
		request_wifi();
	}
}

static lv_obj_t* wifi_create(lv_obj_t* parent)
{
	lv_obj_t* icon = lv_obj_create(parent);
	lv_obj_add_style(icon, &invisible_cont_style, LV_STATE_DEFAULT);
	/* Right to left would mirror the rings' angles. */
	lv_obj_set_style_base_dir(icon, LV_BASE_DIR_LTR, 0);
	lv_obj_set_size(icon, WIFI_WIDTH, WIFI_HEIGHT);
	lv_obj_set_style_pad_all(icon, 0, 0);
	lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

	/* Rings around the dot at the bottom middle, each drawn as the upper
	 * quarter of a circle. */
	int cx = WIFI_WIDTH / 2;
	int cy = WIFI_HEIGHT - 2;
	for (int i = 0; i < WIFI_BARS; i++) {
		int radius = 6 + i * 5;
		lv_obj_t* arc = lv_arc_create(icon);
		lv_obj_set_size(arc, radius * 2, radius * 2);
		lv_obj_set_pos(arc, cx - radius, cy - radius);
		lv_arc_set_bg_angles(arc, 225, 315);
		lv_obj_set_style_pad_all(arc, 0, LV_PART_MAIN);
		lv_obj_set_style_arc_width(arc, WIFI_ARC_WIDTH, LV_PART_MAIN);
		lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);
		lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_INDICATOR);
		lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
		lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
		wifi_bars[i] = arc;
	}
	wifi_dot = lv_obj_create(icon);
	lv_obj_remove_style_all(wifi_dot);
	lv_obj_set_size(wifi_dot, 4, 4);
	lv_obj_set_pos(wifi_dot, cx - 2, cy - 3);
	lv_obj_set_style_radius(wifi_dot, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_opa(wifi_dot, LV_OPA_COVER, 0);

	/* Not connected: a red slash across the icon. */
	static lv_point_t cross_points[] = {
		{3, 2}, {WIFI_WIDTH - 3, WIFI_HEIGHT - 1},
	};
	wifi_cross = lv_line_create(icon);
	lv_line_set_points(wifi_cross, cross_points, 2);
	lv_obj_set_style_line_width(wifi_cross, 2, 0);
	lv_obj_set_style_line_color(wifi_cross, lv_color_hex(TT_COLOR_ERROR), 0);
	lv_obj_set_style_line_rounded(wifi_cross, true, 0);

	lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
	return icon;
}

void tt_status_bar_create(lv_obj_t* header)
{
	/* The header holds the back button and the page title; the title takes
	 * the room left, on one line that moves back and forth when it doesn't
	 * fit. */
	lv_obj_t* title = lv_obj_get_child(header, 1);
	if (title != NULL) {
		/* Width 0 and grow: a scrolling label otherwise takes its text's
		 * width and runs under the time. */
		lv_obj_set_width(title, 0);
		lv_obj_set_flex_grow(title, 1);
		lv_label_set_long_mode(title, LV_LABEL_LONG_SCROLL);
	}

	lv_obj_t* cont = lv_obj_create(header);
	lv_obj_add_style(cont, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(cont, 0, 0);
	lv_obj_set_style_pad_column(cont, 8, 0);
	lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
	/* START: with END, LVGL measures a content-sized row too narrow. */
	lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			LV_FLEX_ALIGN_CENTER);

	lbl_time = lv_label_create(cont);
	lv_label_set_text(lbl_time, "");
	wifi = wifi_create(cont);

	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	tt_clock_apply(&snapshot.datetime);
	datetime_revision = snapshot.datetime_revision;
	wifi_revision = snapshot.wifi_revision;
	show_time();

	request_datetime();
	request_wifi();
	seconds_to_poll = POLL_PERIOD_S;
	lv_timer_create(update_cb, UPDATE_PERIOD_MS, NULL);
}
