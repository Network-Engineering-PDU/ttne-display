#include <lvgl/lvgl.h>
#include <stdio.h>
#include <unistd.h>

#include "ttne_display.h"
#include "tt_keyboard.h"
#include "utils.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"

#include "scr_init.h"
#include "scr_splash.h"
#include "scr_info.h"
#include "scr_alarms.h"
#include "scr_power.h"
#include "scr_outlets.h"
#include "scr_sensors.h"
#include "scr_settings.h"

#include "config.h"
#include "controller.h"
#include "screen.h"

/* Global variables ***********************************************************/

static lv_obj_t* menu;
static lv_obj_t* main_page;

/* Function prototypes ********************************************************/

static void menu_header_cb(lv_event_t* e);

/* Callbacks ******************************************************************/

static void menu_header_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu_header_btn = lv_event_get_user_data(e);
	if (code == LV_EVENT_CLICKED) {
		if (menu_header_btn != NULL) {
			lv_event_send(menu_header_btn, LV_EVENT_CLICKED, menu);
		}
	}
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void ttne_display(void)
{
	config_init();
	tt_styles_init();
	controller_init();
	screen_init();
	controller_get_sys_info();
	controller_get_pdu_info();

	uint8_t rotation = config_get_rotation();
	screen_set_rotation(rotation);

	menu = lv_menu_create(lv_scr_act());
	lv_obj_set_size(menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
	main_page = lv_menu_page_create(menu, NULL);
	lv_obj_t* main_cont = lv_menu_cont_create(main_page);
	lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_ROW_WRAP);
	// lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));

	lv_obj_set_style_bg_color(menu, lv_color_hex(TT_COLOR_BG2), 0);

	lv_obj_t* menu_header = lv_menu_get_main_header(menu);
	lv_obj_set_height(menu_header, 40);
	lv_obj_add_style(menu_header, &header_style, 0);
	lv_obj_add_flag(menu_header, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_t* menu_header_btn = lv_menu_get_main_header_back_btn(menu);
	lv_obj_set_size(menu_header_btn, 30, 30);
	lv_obj_set_flex_flow(menu_header_btn, LV_FLEX_FLOW_COLUMN);
	lv_obj_clear_flag(menu_header_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_clear_flag(menu_header_btn, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_add_event_cb(menu_header, menu_header_cb, LV_EVENT_ALL,
			menu_header_btn);

	lv_obj_t* icon = lv_obj_get_child(menu_header_btn, 0);
	lv_obj_set_layout(menu_header_btn, 0);
	lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

	lv_menu_set_page_title_static(main_page, "Network Engineering");

	lv_obj_t* btn_info = tt_obj_btn_mtx_create(main_cont, NULL, "INFO",
			ASSET("menu.png"));
	lv_obj_t* btn_alarms = tt_obj_btn_mtx_create(main_cont, NULL,"ALARMS",
			ASSET("alarms.png"));
	lv_obj_t* btn_power = tt_obj_btn_mtx_create(main_cont, NULL, "POWER",
			ASSET("power.png"));
	lv_obj_t* btn_outlets = tt_obj_btn_mtx_create(main_cont, NULL, "OUTLETS",
			ASSET("outlets.png"));
	lv_obj_t* btn_sensors = tt_obj_btn_mtx_create(main_cont, NULL, "SENSORS",
			ASSET("sensors.png"));
	lv_obj_t* btn_settings = tt_obj_btn_mtx_create(main_cont, NULL, "SETTINGS",
			ASSET("settings.png"));

	// Notifications test
	// lv_obj_t* notif_cont = tt_obj_cont_create(btn_alarms);
	// lv_obj_t* lbl = tt_obj_label_create(notif_cont, "1");
	// lv_obj_align(lbl, LV_ALIGN_TOP_RIGHT, 0, 0);
	// lv_obj_align(notif_cont, LV_ALIGN_TOP_RIGHT, 0, 0);
	// lv_obj_set_style_bg_color(notif_cont, lv_color_hex(TT_COLOR_ERROR), 0);
	// lv_obj_set_style_border_color(notif_cont, lv_color_hex(TT_COLOR_ERROR), 0);
	// lv_obj_set_size(notif_cont, 15, 15);
	// lv_obj_set_style_radius(notif_cont, LV_RADIUS_CIRCLE, 0);
	// Notifications test

	scr_info_create(menu, btn_info);
	scr_alarms_create(menu, btn_alarms);
	scr_power_create(menu, btn_power);
	scr_outlets_create(menu, btn_outlets);
	scr_sensors_create(menu, btn_sensors);
	scr_settings_create(menu, btn_settings);

	lv_menu_set_page(menu, main_page);
	lv_obj_t* menu_scr = lv_scr_act();
	scr_splash_create(menu_scr);
	scr_splash_show();
}

void ttne_display_idle_cb()
{
	scr_splash_show();
	if (lv_menu_get_cur_main_page(menu) != main_page) {
		lv_obj_t* menu_header_btn = lv_menu_get_main_header_back_btn(menu);
		lv_event_send(menu_header_btn, LV_EVENT_CLICKED, menu);
	}
}
