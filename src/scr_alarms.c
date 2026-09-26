#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "scr_alarms.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define TIMER_REFRESH_RATE 5000 // ms

/* Global variables ***********************************************************/

/* Alarms are evaluated by the API so the display and the web UI always show
 * the same rules. This screen only fetches, shows and acknowledges them. */

static app_state_alarms_t view; /* what is currently drawn */
static bool view_valid;

static lv_obj_t* alarms_alarms_cont;
static lv_obj_t* lbl_no_alarms;
static lv_obj_t* badge;
static lv_obj_t* badge_lbl;

static lv_timer_t* timer;
static bool refresh_pending;
static bool ack_pending;
static bool api_error;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void alarm_cont_close_cb(lv_event_t* e);
static void alarms_timer_cb(lv_timer_t* t);
static void alarms_refresh_cb(int err, void* userdata);
static void alarm_ack_cb(int err, void* userdata);
static void request_refresh(void);
static void apply_snapshot(void);
static void redraw(void);
static bool view_changed(const app_state_alarms_t* alarms);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	(void)e;
}

static void alarm_cont_close_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED || ack_pending) {
		return;
	}

	lv_obj_t* cont = lv_event_get_user_data(e);
	const app_state_alarm_t* alarm = lv_obj_get_user_data(cont);
	if (alarm == NULL) {
		return;
	}

	/* The id is copied by the backend; the alarm pointer is not used again. */
	ack_pending = true;
	if (backend_alarm_ack(alarm->id, alarm_ack_cb, NULL) != 0) {
		ack_pending = false;
		tt_obj_info_box_create("Alarms", "Could not acknowledge alarm", 1);
	}
}

static void alarm_ack_cb(int err, void* userdata)
{
	(void)userdata;
	ack_pending = false;
	if (err != 0) {
		tt_obj_info_box_create("Alarms", "Could not acknowledge alarm", 1);
	}
	request_refresh();
}

static void alarms_timer_cb(lv_timer_t* t)
{
	(void)t;
	request_refresh();
}

static void alarms_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	refresh_pending = false;
	api_error = (err != 0);
	apply_snapshot();
}

/* Function definitions *******************************************************/

static void request_refresh(void)
{
	/* Callbacks may run synchronously (simulator), so set the flag first */
	if (refresh_pending) {
		return;
	}
	refresh_pending = true;
	if (backend_alarms_refresh(alarms_refresh_cb, NULL) != 0) {
		refresh_pending = false;
	}
}

static bool view_changed(const app_state_alarms_t* alarms)
{
	if (!view_valid || alarms->count != view.count) {
		return true;
	}
	for (int i = 0; i < alarms->count; i++) {
		if (memcmp(&alarms->items[i], &view.items[i],
				sizeof(alarms->items[i])) != 0) {
			return true;
		}
	}
	return false;
}

static void apply_snapshot(void)
{
	static app_state_snapshot_t snapshot;

	lv_label_set_text(lbl_no_alarms, api_error ?
			"Alarm data unavailable" : "No alarm has been triggered");

	app_state_get_snapshot(&snapshot);
	if (snapshot.alarms.valid && view_changed(&snapshot.alarms)) {
		view = snapshot.alarms;
		view_valid = true;
		redraw();
	}
}

/* Rebuilds the alarm list. Children 0 and 1 are the section title and the
 * "no alarm" label; everything after them is an alarm entry. */
static void redraw(void)
{
	uint32_t n_children = lv_obj_get_child_cnt(alarms_alarms_cont);
	int unacked = 0;

	for (uint32_t i = n_children; i > 2; i--) {
		lv_obj_del(lv_obj_get_child(alarms_alarms_cont, i - 1));
	}

	for (int i = 0; i < view.count; i++) {
		if (!view.items[i].ack) {
			unacked++;
			tt_obj_cont_alarm_create(alarms_alarms_cont, alarm_cont_close_cb,
					&view.items[i]);
		}
	}

	if (unacked == 0) {
		lv_obj_clear_flag(lbl_no_alarms, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(badge, LV_OBJ_FLAG_HIDDEN);
	} else {
		char str[16];
		lv_obj_add_flag(lbl_no_alarms, LV_OBJ_FLAG_HIDDEN);
		snprintf(str, sizeof(str), "%d", unacked > 99 ? 99 : unacked);
		lv_label_set_text(badge_lbl, str);
		lv_obj_clear_flag(badge, LV_OBJ_FLAG_HIDDEN);
	}
}

/* Public functions ***********************************************************/

void scr_alarms_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* alarms_cont = tt_obj_menu_page_create(menu, btn, menu_cb, "Alarms");

	alarms_alarms_cont = tt_obj_cont_create(alarms_cont);
	tt_obj_label_create(alarms_alarms_cont, "Alarms");
	lbl_no_alarms = tt_obj_label_color_create(alarms_alarms_cont,
			"No alarm has been triggered");

	/* Count badge on the main menu button */
	badge = lv_obj_create(btn);
	lv_obj_add_flag(badge, LV_OBJ_FLAG_FLOATING);
	lv_obj_clear_flag(badge, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(badge, 20, 20);
	lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(badge, lv_color_hex(TT_COLOR_ERROR), 0);
	lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(badge, 0, 0);
	lv_obj_set_style_pad_all(badge, 0, 0);
	lv_obj_align(badge, LV_ALIGN_TOP_RIGHT, 0, 0);
	badge_lbl = lv_label_create(badge);
	lv_obj_center(badge_lbl);
	lv_obj_add_flag(badge, LV_OBJ_FLAG_HIDDEN);

	/* Polled in the background so the badge is always current */
	timer = lv_timer_create(alarms_timer_cb, TIMER_REFRESH_RATE, NULL);
	request_refresh();
}
