#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "scr_splash.h"
#include "tt_obj.h"
#include "config.h"
#include "screen.h"
#include "utils.h"
#include "tt_colors.h"
#include "controller.h"
#include "models.h"

#define TIMER_REFRESH_RATE 10000 // ms

/* Global variables ***********************************************************/

static lv_obj_t* splash_scr;
static lv_obj_t* tap_overlay;
static lv_obj_t* menu_scr_ref;
static lv_obj_t* login_scr_ref;
static lv_obj_t* nav_target;
static bool session_authenticated = false;

static lv_timer_t* timer_check;

static lv_obj_t* init_spinner;

static lv_obj_t* lbl_system;
static lv_obj_t* lbl_ip;

static bool flag_init = false;

/* Function prototypes ********************************************************/

static void splash_cb(lv_event_t* e);
static void splash_timer_cb(lv_timer_t* timer);
static void splash_refresh_nav_target(void);
static void splash_set_tap_enabled(bool enabled);
static const char* get_iface_label(const models_nw_if_t* nw_if);

/* Callbacks ******************************************************************/

static void splash_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (lv_scr_act() != splash_scr) {
		return;
	}
	if (code == LV_EVENT_CLICKED && nav_target != NULL) {
		splash_set_tap_enabled(false);
		lv_timer_pause(timer_check);
		lv_scr_load(nav_target);
	}
}

static void splash_set_tap_enabled(bool enabled)
{
	if (tap_overlay == NULL) {
		return;
	}
	if (enabled) {
		lv_obj_add_flag(tap_overlay, LV_OBJ_FLAG_CLICKABLE);
	} else {
		lv_obj_clear_flag(tap_overlay, LV_OBJ_FLAG_CLICKABLE);
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
		splash_set_tap_enabled(true);
		lv_obj_del(init_spinner);
		init_spinner = NULL;
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

static void splash_refresh_nav_target(void)
{
	if (config_get_skip_login() || session_authenticated) {
		nav_target = menu_scr_ref;
	} else {
		nav_target = login_scr_ref;
	}
}

void scr_splash_on_login_success(void)
{
	session_authenticated = true;
	splash_refresh_nav_target();
}

lv_obj_t* scr_splash_create(lv_obj_t* menu_scr, lv_obj_t* login_scr)
{
	menu_scr_ref = menu_scr;
	login_scr_ref = login_scr;
	session_authenticated = false;

	splash_scr = lv_obj_create(NULL);

	lv_obj_set_size(splash_scr, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_radius(splash_scr, 0, 0);
	lv_obj_set_style_bg_color(splash_scr, lv_color_hex(TT_COLOR_BG1), 0);

	lv_obj_t* logo = lv_img_create(splash_scr);
	lv_img_set_src(logo, ASSET("ne_logo.png"));

	splash_refresh_nav_target();

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

	tap_overlay = lv_obj_create(splash_scr);
	lv_obj_remove_style_all(tap_overlay);
	lv_obj_set_size(tap_overlay, LV_PCT(100), LV_PCT(100));
	lv_obj_clear_flag(tap_overlay, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(tap_overlay, splash_cb, LV_EVENT_CLICKED, NULL);
	splash_set_tap_enabled(false);
	lv_obj_move_foreground(tap_overlay);

	timer_check = lv_timer_create(splash_timer_cb, TIMER_REFRESH_RATE, NULL);
	lv_timer_pause(timer_check);
	splash_timer_cb(timer_check);

#ifdef SIMULATOR_ENABLED
	if (!flag_init) {
		flag_init = true;
		splash_set_tap_enabled(true);
		if (init_spinner != NULL) {
			lv_obj_del(init_spinner);
			init_spinner = NULL;
		}
	}
#endif

	return splash_scr;
}


void scr_splash_show()
{
	splash_refresh_nav_target();
	if (lv_scr_act() != splash_scr) {
		if (flag_init) {
			splash_set_tap_enabled(true);
		}
		lv_timer_resume(timer_check);
		lv_scr_load(splash_scr);
	}
}