#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"

#include "scr_settings_vis.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "config.h"
#include "screen.h"

#define TIMER_ROT 5000 // ms

extern void reset_program();

static lv_obj_t* dd_rotation;
static lv_obj_t* txt_splash;
static lv_obj_t* msg_box_rot;

static lv_timer_t* timer_rot;

static void rotate_cb(lv_event_t* e);
static void timer_rot_cb(lv_timer_t* timer);
static void msg_box_rot_cb(lv_event_t* e);
static void txt_inactivity_cb(lv_event_t* e);
static void revert_rot();

static void rotate_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		uint16_t rotation = lv_dropdown_get_selected(dd_rotation);
		screen_set_rotation(rotation);

		if (timer_rot != NULL) {
			lv_timer_del(timer_rot);
		}
		timer_rot = lv_timer_create(timer_rot_cb, TIMER_ROT, NULL);

		char rot_str[16];
		lv_dropdown_get_selected_str(dd_rotation, rot_str, sizeof(rot_str));
		int rot_int = atoi(rot_str);

		char msg[300];
		snprintf(msg, sizeof(msg),
				"Are you sure you want to save screen rotation?\nRotation: "
				TT_COLOR_GREEN_NE_STR
				" %d deg#\n(changes will be reverted in 5 seconds)",
				rot_int);
		msg_box_rot = tt_obj_msg_box_create("Screen rotation", msg, NULL,
				msg_box_rot_cb);
	}
}

static void timer_rot_cb(lv_timer_t* timer)
{
	if (msg_box_rot != NULL && lv_obj_is_valid(msg_box_rot)) {
		lv_msgbox_close(msg_box_rot);
	}
	lv_timer_del(timer);
	timer_rot = NULL;
	msg_box_rot = NULL;
	revert_rot();
}

static void msg_box_rot_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (timer_rot != NULL) {
			lv_timer_del(timer_rot);
			timer_rot = NULL;
		}

		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			config_set_rotation(lv_dropdown_get_selected(dd_rotation));
			reset_program();
		} else {
			revert_rot();
		}

		msg_box_rot = NULL;
		lv_msgbox_close(obj);
	}
}

static void txt_inactivity_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_NUM);
		lv_scr_load(kb_scr);
	} else if (code == LV_EVENT_READY) {
		int t = atoi(lv_textarea_get_text(txt_splash));
		if (t < 1 || t > 300) {
			tt_obj_info_box_create("ERROR",
					"Inactivity time must be in the interval [1-300] (minutes)",
					1);
			return;
		}
		config_set_inactivity_time(t);
	}
}

static void revert_rot()
{
	screen_set_rotation(config_get_rotation());
	lv_dropdown_set_selected(dd_rotation, config_get_rotation());
}

void scr_settings_vis_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* cont = tt_obj_menu_page_create(menu, btn, NULL,
			"Settings / Visual");

	lv_obj_t* vis_cont = tt_obj_cont_create(cont);

	tt_obj_label_create(vis_cont, "Screen rotation");
	dd_rotation = tt_obj_dropdown_create(vis_cont,
			"0 deg\n90 deg\n180 deg\n270 deg", rotate_cb);
	lv_dropdown_set_selected(dd_rotation, config_get_rotation());

	tt_obj_label_create(vis_cont, "Screensaver time (min)");
	txt_splash = tt_obj_txt_create(vis_cont, "Time in minutes",
			txt_inactivity_cb);
	char inactivity_time_str[10];
	snprintf(inactivity_time_str, sizeof(inactivity_time_str), "%d",
			config_get_inactivity_time());
	lv_textarea_set_text(txt_splash, inactivity_time_str);
}
