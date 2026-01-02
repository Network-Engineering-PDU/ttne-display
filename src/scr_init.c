#include <stdio.h>

#include "lvgl/lvgl.h"
#include "scr_init.h"
#include "scr_splash.h"
#include "tt_obj.h"
#include "controller.h"
#include "models.h"

#define TIMER_REFRESH_RATE 3000 // ms

/* Global variables ***********************************************************/

static lv_obj_t* init_scr;

static lv_timer_t* timer;

/* Function prototypes ********************************************************/

static void init_timer_cb(lv_timer_t* timer);

/* Callbacks ******************************************************************/

static void init_timer_cb(lv_timer_t* timer)
{
	controller_get_sys_info();
	const models_info_t* info = models_get_info();
	if (strcmp(info->ip, "N/A") != 0) {
		lv_timer_del(timer);
		scr_splash_show(true);
	}
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

lv_obj_t* scr_init_create()
{
	init_scr = lv_obj_create(NULL);
	lv_obj_set_size(init_scr, LV_PCT(100), LV_PCT(100));
	// lv_obj_set_style_radius(splash_scr, 0, 0);
	// lv_obj_set_style_bg_color(splash_scr, lv_color_hex(TT_COLOR_BG1), 0);

	// lv_obj_add_flag(splash_scr, LV_OBJ_FLAG_CLICKABLE);

	tt_obj_spinner_create(init_scr, "Initializing system");

	timer = lv_timer_create(init_timer_cb, TIMER_REFRESH_RATE, NULL);

	return init_scr;
}

void scr_init_show()
{
	if (lv_scr_act() != init_scr) {
		lv_scr_load(init_scr);
	}
}
