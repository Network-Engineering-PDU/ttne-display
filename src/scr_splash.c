#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "scr_splash.h"
#include "tt_obj.h"
#include "screen.h"
#include "utils.h"
#include "tt_colors.h"
#include "controller.h"
#include "models.h"

#define TIMER_REFRESH_RATE 10000 // ms

/* Global variables ***********************************************************/

static lv_obj_t* splash_scr;

static lv_timer_t* timer_check;

static lv_obj_t* init_spinner;

static lv_obj_t* lbl_system;
static lv_obj_t* lbl_ip;
static lv_obj_t* lbl_power_easy;

static bool flag_init = false;

/* Function prototypes ********************************************************/

static void splash_cb(lv_event_t* e);
static void splash_timer_cb(lv_timer_t* timer);
static const char* get_iface_label(const models_nw_if_t* nw_if);
static void show_power_easy_message(void);

/* Callbacks ******************************************************************/

static void splash_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* scr = lv_event_get_user_data(e);

	if (lv_scr_act() != splash_scr) {
		return;
	}
	if (code == LV_EVENT_CLICKED) {
		lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
		lv_timer_pause(timer_check);
		lv_scr_load(scr);
	}
}

static void show_power_easy_message(void)
{
	if (lbl_power_easy == NULL) {
		return;
	}
	lv_label_set_text(lbl_power_easy, "POWER IT EASY");
	lv_obj_clear_flag(lbl_power_easy, LV_OBJ_FLAG_HIDDEN);
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
		//show_power_easy_message();
	}
	const char* iface = get_iface_label(nw_if);
	//sprintf(str, "%s: %s", "SYSTEM", info->product_name);
	sprintf(str, "%s: %s", "PowerIT Easy");
	lv_label_set_text(lbl_system, str);
	sprintf(str, "%s: %s %s", "IP", nw_if->params.ip, iface);
	lv_label_set_text(lbl_ip, str);
}

static const char* get_iface_label(const models_nw_if_t* nw_if)
{
	if (nw_if->type == WIFI_DHCP || nw_if->type == WIFI_STATIC) {
		return "(WiFi)";
	}

	bool has_wifi = nw_if->wifi_ip != NULL && strlen(nw_if->wifi_ip) > 0;
	bool is_wifi_ip = has_wifi && nw_if->params.ip != NULL && strcmp(nw_if->params.ip, nw_if->wifi_ip) == 0;
	bool is_not_lan_ip = has_wifi && nw_if->params.ip != NULL &&
		strcmp(nw_if->params.ip, nw_if->lan1_ip) != 0 &&
		strcmp(nw_if->params.ip, nw_if->lan2_ip) != 0;
	if (is_wifi_ip || is_not_lan_ip) {
		return "(WiFi)";
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
	lv_obj_add_event_cb(lv_layer_top(), splash_cb, LV_EVENT_ALL, prev_scr);

	init_spinner = tt_obj_spinner_inline_create(splash_scr,
			"Initializing system...");
	lbl_power_easy = tt_obj_label_create(splash_scr, "");
	lv_obj_add_flag(lbl_power_easy, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_text_font(lbl_power_easy, &lv_font_montserrat_18, 0);
	lv_obj_set_style_text_align(lbl_power_easy, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_align(lbl_power_easy, LV_ALIGN_CENTER, 0, 0);

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


void scr_splash_show()
{
	if (lv_scr_act() != splash_scr) {
		if (flag_init) {
			lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
		}
		lv_timer_resume(timer_check);
		lv_scr_load(splash_scr);
	}
}