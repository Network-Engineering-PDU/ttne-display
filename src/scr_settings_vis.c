#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"

#include "scr_settings_vis.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "config.h"
#include "screen.h"

static lv_obj_t* dd_rotation;
static lv_obj_t* txt_splash;

static void rotate_cb(lv_event_t* e);
static void txt_inactivity_cb(lv_event_t* e);

static void rotate_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		uint16_t rotation = lv_dropdown_get_selected(dd_rotation);
		screen_set_rotation(rotation);
		config_set_rotation(rotation);
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
