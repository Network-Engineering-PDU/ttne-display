#include <stdio.h>
#include <time.h>

#include "lvgl/lvgl.h"

#include "scr_alarms.h"
#include "alarms.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define TIMER_REFRESH_RATE 5000 // ms

/* Global variables ***********************************************************/

static alarms_t alarms;

static lv_obj_t* alarms_alarms_cont;
static lv_obj_t* lbl_no_alarms;
static lv_obj_t* badge;
static lv_obj_t* badge_lbl;

static lv_timer_t* timer;
static bool power_pending;
static bool nw_pending;
static bool comm_error;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void alarm_cont_close_cb(lv_event_t* e);
static void alarms_timer_cb(lv_timer_t* timer);
static void power_refresh_cb(int err, void* userdata);
static void nw_refresh_cb(int err, void* userdata);
static void evaluate(void);
static void redraw(void);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	(void)e;
}

static void alarm_cont_close_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		lv_obj_t* cont = lv_event_get_user_data(e);
		const alarm_desc_t* alarm = lv_obj_get_user_data(cont);

		if (alarm != NULL) {
			alarms_ack(&alarms, alarm);
		}
		redraw();
	}
}

static void alarms_timer_cb(lv_timer_t* t)
{
	(void)t;
	/* Callbacks may run synchronously (simulator), so set the flag first */
	if (!power_pending) {
		power_pending = true;
		if (backend_power_refresh(power_refresh_cb, NULL) != 0) {
			power_pending = false;
		}
	}
	if (!nw_pending) {
		nw_pending = true;
		if (backend_network_info_refresh(nw_refresh_cb, NULL) != 0) {
			nw_pending = false;
		}
	}
	backend_pdu_info_refresh(NULL, NULL);
}

static void power_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	power_pending = false;
	comm_error = (err != 0);
	evaluate();
}

static void nw_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	nw_pending = false;
	if (err == 0) {
		evaluate();
	}
}

/* Function definitions *******************************************************/

static void evaluate(void)
{
	app_state_snapshot_t snapshot;

	app_state_get_snapshot(&snapshot);
	if (alarms_update(&alarms, &snapshot, comm_error, time(NULL))) {
		redraw();
	}
}

/* Rebuilds the alarm list. Children 0 and 1 are the section title and the
 * "no alarm" label; everything after them is an alarm entry. */
static void redraw(void)
{
	int unacked = alarms_unacked_count(&alarms);
	uint32_t n_children = lv_obj_get_child_cnt(alarms_alarms_cont);

	for (uint32_t i = n_children; i > 2; i--) {
		lv_obj_del(lv_obj_get_child(alarms_alarms_cont, i - 1));
	}

	for (int i = 0; i < alarms.n; i++) {
		if (!alarms.alarms[i].ack) {
			tt_obj_cont_alarm_create(alarms_alarms_cont, alarm_cont_close_cb,
					&alarms.alarms[i]);
		}
	}
	if (unacked == 0) {
		lv_obj_clear_flag(lbl_no_alarms, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(lbl_no_alarms, LV_OBJ_FLAG_HIDDEN);
	}

	if (unacked > 0) {
		char str[8];
		snprintf(str, sizeof(str), "%d", unacked > 99 ? 99 : unacked);
		lv_label_set_text(badge_lbl, str);
		lv_obj_clear_flag(badge, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(badge, LV_OBJ_FLAG_HIDDEN);
	}
}

/* Public functions ***********************************************************/

void scr_alarms_create(lv_obj_t* menu, lv_obj_t* btn)
{
	alarms_init(&alarms);

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

	/* Alarms are evaluated in the background so the badge is always current */
	timer = lv_timer_create(alarms_timer_cb, TIMER_REFRESH_RATE, NULL);
	alarms_timer_cb(timer);
}
