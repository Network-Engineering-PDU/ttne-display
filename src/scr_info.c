#include <stdio.h>

#include "i18n.h"
#include "lvgl/lvgl.h"
#include "scr_info.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "app/app_state.h"
#include "backend/backend.h"

#ifndef GIT_VERSION
#define GIT_VERSION "Unknown"
#endif

/* Global variables ***********************************************************/

static lv_obj_t* lbl_name;
static lv_obj_t* lbl_pn;
static lv_obj_t* lbl_sn;
static lv_obj_t* lbl_mac;
static lv_obj_t* lbl_ip;
static lv_obj_t* lbl_outlets;
static lv_obj_t* lbl_rated_curr;
static lv_obj_t* lbl_controller;
static lv_obj_t* lbl_version;
static lv_obj_t* lbl_om_version;
static lv_obj_t* lbl_pmb_version;
static lv_obj_t* lbl_display_version;
static lv_obj_t* lbl_uptime;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void refresh_info_display(void);
static void info_refresh_cb(int err, void* userdata);
static void configure_info_value_label(lv_obj_t* label);
static const char* display_version(void);

/* Callbacks ******************************************************************/

static void configure_info_value_label(lv_obj_t* label)
{
	lv_obj_set_width(label, LV_PCT(100));
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
	lv_obj_set_scrollbar_mode(label, LV_SCROLLBAR_MODE_OFF);
}

static const char* display_version(void)
{
	return GIT_VERSION[0] != '\0' ? GIT_VERSION : tr("Unknown");
}

static void refresh_info_display(void)
{
	char str[256];
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	const app_state_system_info_t* info = &snapshot.system_info;
	const app_state_pdu_info_t* pdu_info = &snapshot.pdu_info;

	snprintf(str, sizeof(str), "  %s: #%06X %s#", tr("Name"), TT_COLOR_GREEN_NE, "PowerIT Easy");
	i18n_label_set_text(lbl_name, str);
	snprintf(str, sizeof(str), "  %s: #%06X %s#", tr("PN"), TT_COLOR_GREEN_NE, info->product_pn);
	i18n_label_set_text(lbl_pn, str);
	snprintf(str, sizeof(str), "  %s: #%06X %s#", tr("SN"), TT_COLOR_GREEN_NE, info->product_sn);
	i18n_label_set_text(lbl_sn, str);
	snprintf(str, sizeof(str), "  %s: #%06X %s#", "LAN MAC", TT_COLOR_GREEN_NE, info->lan_mac);
	i18n_label_set_text(lbl_mac, str);
	snprintf(str, sizeof(str), "  %s: #%06X %s#", "IP", TT_COLOR_GREEN_NE, info->ip);
	i18n_label_set_text(lbl_ip, str);

	snprintf(str, sizeof(str), "  %s: #%06X %d#", tr("Outlets"), TT_COLOR_GREEN_NE, pdu_info->n_outlets);
	i18n_label_set_text(lbl_outlets, str);
	char value[16];
	char colored[64];
	snprintf(value, sizeof(value), "%d", pdu_info->rated_current);
	i18n_format_value(colored, sizeof(colored), TT_COLOR_GREEN_NE, value, tr("A"));
	snprintf(str, sizeof(str), "  %s: %s", tr("Rated current"), colored);
	i18n_label_set_text(lbl_rated_curr, str);
	snprintf(str, sizeof(str), "  %s: #%06X %s#", tr("Controller"), TT_COLOR_GREEN_NE, pdu_info->controller);
	i18n_label_set_text(lbl_controller, str);

	snprintf(str, sizeof(str), "  %s: #%06X %s#", tr("SW version"), TT_COLOR_GREEN_NE, info->sw_version);
	i18n_label_set_text(lbl_version, str);
	snprintf(str, sizeof(str), "  %s: #%06X %s#", tr("OM version"), TT_COLOR_GREEN_NE, info->om_version);
	i18n_label_set_text(lbl_om_version, str);
	snprintf(str, sizeof(str), "  %s: #%06X %s#", tr("PMB version"), TT_COLOR_GREEN_NE, info->pmb_version);
	i18n_label_set_text(lbl_pmb_version, str);
	snprintf(str, sizeof(str), "  %s: #%06X %s#", tr("Display version"), TT_COLOR_GREEN_NE, display_version());
	i18n_label_set_text(lbl_display_version, str);
	snprintf(str, sizeof(str), "  %s: #%06X %s#", tr("Uptime (DDD:HH:MM)"), TT_COLOR_GREEN_NE, info->uptime);
	i18n_label_set_text(lbl_uptime, str);
}

static void info_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	if (err == 0) {
		refresh_info_display();
	}
}

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			LV_LOG_USER("Info cb - page change detected");
			backend_system_info_refresh(info_refresh_cb, NULL);
			backend_pdu_info_refresh(info_refresh_cb, NULL);
		}
	}
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void scr_info_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* info_cont = tt_obj_menu_page_create(menu, btn, menu_cb, tr("Info"));
	lv_obj_t* info_product_cont = tt_obj_cont_create(info_cont);
	tt_obj_label_color_create(info_product_cont, tr("Information"));

	lbl_name = tt_obj_label_color_create(info_product_cont, "");
	lbl_pn = tt_obj_label_color_create(info_product_cont, "");
	lbl_sn = tt_obj_label_color_create(info_product_cont, "");
	lbl_mac = tt_obj_label_color_create(info_product_cont, "");
	lbl_ip = tt_obj_label_color_create(info_product_cont, "");
	lbl_outlets = tt_obj_label_color_create(info_product_cont, "");
	lbl_rated_curr = tt_obj_label_color_create(info_product_cont, "");
	lbl_controller = tt_obj_label_color_create(info_product_cont, "");
	lbl_version = tt_obj_label_color_create(info_product_cont, "");
	lbl_om_version = tt_obj_label_color_create(info_product_cont, "");
	lbl_pmb_version = tt_obj_label_color_create(info_product_cont, "");
	lbl_display_version = tt_obj_label_color_create(info_product_cont, "");
	lbl_uptime = tt_obj_label_color_create(info_product_cont, "");

	configure_info_value_label(lbl_version);
	configure_info_value_label(lbl_om_version);
	configure_info_value_label(lbl_pmb_version);
	configure_info_value_label(lbl_display_version);
	configure_info_value_label(lbl_uptime);

	refresh_info_display();
}
