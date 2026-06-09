#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "lvgl/lvgl.h"
#include "scr_splash.h"
#include "tt_obj.h"
#include "screen.h"
#include "utils.h"
#include "tt_colors.h"
#include "controller.h"
#include "models.h"
#include "config.h"
#include "scr_login.h"
#include "ttne_display.h"

#define TIMER_REFRESH_RATE 10000 // ms

/* Global variables ***********************************************************/

static lv_obj_t* splash_scr;

static lv_timer_t* timer_check;

static lv_obj_t* init_spinner;

static lv_obj_t* lbl_system;
static lv_obj_t* lbl_ip;

static bool flag_init = false;
static bool login_shown = false;
static lv_obj_t* splash_prev_scr = NULL;

/* Function prototypes ********************************************************/

static void splash_cb(lv_event_t* e);
static void splash_timer_cb(lv_timer_t* timer);
static const char* get_iface_label(const models_nw_if_t* nw_if);

/* Callbacks ******************************************************************/

static void splash_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (lv_scr_act() != splash_scr) {
		return;
	}
	if (code == LV_EVENT_CLICKED) {
		lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
		lv_timer_pause(timer_check);
		lv_scr_load(splash_prev_scr);
		if (flag_init && !login_shown && !config_get_skip_login()) {
			lv_obj_t* menu = lv_obj_get_parent(ttne_get_main_page());
			if (menu != NULL) {
				lv_menu_set_page(menu, scr_login_get_page());
			}
		}
	}
}

static void splash_timer_cb(lv_timer_t* timer)
{
	(void) timer;
	controller_get_sys_info();
	controller_get_nw_if();
	char str[100];
	const models_info_t* info = models_get_info();
	const models_nw_if_t* nw_if = models_get_nw_if();
	// TODO: remove tap cb al inicializar la pantalla
	// TODO: check a started_flag
	if (!flag_init && strcmp(info->product_name, "N/A") != 0) { // API iniciada
		flag_init = true;
		lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
		lv_obj_del(init_spinner);
	}
	const char* iface = get_iface_label(nw_if);
	sprintf(str, "%s", "PowerIT Easy");
	lv_label_set_text(lbl_system, str);
	sprintf(str, "%s: %s %s", "IP", nw_if->params.ip, iface);
	lv_label_set_text(lbl_ip, str);
}

static const char* get_iface_label(const models_nw_if_t* nw_if)
{
	if (nw_if->type == WIFI_DHCP || nw_if->type == WIFI_STATIC) {
		return "(WIFI)";
	}

	if (nw_if->type == ETH_DHCP || nw_if->type == ETH_STATIC) {
		const char* eth_interface =
				nw_if->eth_interface != NULL ? nw_if->eth_interface : "";
		if (strcmp(eth_interface, "eth0") == 0) {
			return "(ETH2)";
		}
		if (strcmp(eth_interface, "eth1") == 0) {
			return "(ETH1)";
		}
		return "(ETH)";
	}

	return "";
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

lv_obj_t* scr_splash_create(lv_obj_t* prev_scr)
{
	splash_scr = lv_obj_create(NULL);

	lv_obj_set_size(splash_scr, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_radius(splash_scr, 0, 0);
	lv_obj_set_style_bg_color(splash_scr, lv_color_hex(TT_COLOR_BG1), 0);

	lv_obj_add_flag(splash_scr, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_t* logo = lv_img_create(splash_scr);
	lv_img_set_src(logo, ASSET("ne_logo.png"));
	lv_obj_add_event_cb(lv_layer_top(), splash_cb, LV_EVENT_ALL, NULL);

	splash_prev_scr = prev_scr;
	login_shown = config_get_skip_login();

	init_spinner = tt_obj_spinner_inline_create(splash_scr,
		"Initializing system...");
	lv_obj_t* info_cont = tt_obj_cont_create(splash_scr);
	lv_obj_set_size(info_cont, 200, 60);

	if (screen_is_landscape()) {
		lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 30);
		lv_obj_align(info_cont, LV_ALIGN_BOTTOM_MID, 0, -20);
	} else {
		lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 50);
		lv_obj_align(info_cont, LV_ALIGN_BOTTOM_MID, 0, -50);
	}

	lbl_system = tt_obj_label_create(info_cont, NULL);
	lbl_ip = tt_obj_label_create(info_cont, NULL);

	timer_check = lv_timer_create(splash_timer_cb, TIMER_REFRESH_RATE, NULL);
	lv_timer_pause(timer_check);
	splash_timer_cb(timer_check);

	return splash_scr;
}


void scr_splash_show(bool force)
{
	if (lv_scr_act() == splash_scr && !force) {
		return;
	}

	if (flag_init) {
		lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
	}
	lv_timer_resume(timer_check);
	lv_scr_load(splash_scr);
}

void scr_splash_set_prev(lv_obj_t* prev_scr)
{
	if (prev_scr != NULL) {
		splash_prev_scr = prev_scr;
	}
}

void scr_splash_login_completed(void)
{
	login_shown = true;
}
