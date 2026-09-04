#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "scr_splash.h"
#include "tt_obj.h"
#include "screen.h"
#include "utils.h"
#include "tt_colors.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define NW_TYPE_WIFI_DHCP 4
#define NW_TYPE_WIFI_STATIC 5

#define DEFAULT_ETH_IP "192.168.1.100"
#define DEFAULT_SECONDARY_IP "192.168.1.200"

#define NW_MODE_SINGLE_LAN 0
#define NW_MODE_WIFI_ONLY 1
#define NW_MODE_DUAL_LAN 2
#define NW_MODE_LAN_WIFI 3

#define TIMER_REFRESH_RATE 10000 // ms

/* Global variables ***********************************************************/

static lv_obj_t* splash_scr;
static lv_obj_t* next_scr;

static lv_timer_t* timer_check;
static bool nw_refresh_pending;

static lv_obj_t* init_spinner;

static lv_obj_t* info_cont;
static lv_obj_t* lbl_system;
static lv_obj_t* lbl_ip;

static bool flag_init = false;

/* Function prototypes ********************************************************/

static void splash_cb(lv_event_t* e);
static void splash_fetch_cb(int err, void* userdata);
static void splash_network_fetch_cb(int err, void* userdata);
static void splash_update_display(void);
static void splash_timer_cb(lv_timer_t* timer);

static bool has_ip(const char* ip)
{
	return ip != NULL && ip[0] != '\0' && strcmp(ip, "N/A") != 0 &&
			strcmp(ip, "wifi") != 0;
}

static const char* first_ip(const char* first, const char* second,
		const char* fallback)
{
	if (has_ip(first)) {
		return first;
	}
	if (has_ip(second)) {
		return second;
	}
	return fallback;
}

/* Callbacks ******************************************************************/

static void splash_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
	}

	if (lv_scr_act() != splash_scr) {
		return;
	}
	if (code == LV_EVENT_CLICKED) {
		lv_timer_pause(timer_check);
		if (next_scr != NULL) {
			lv_scr_load(next_scr);
		}
	}
}

static void splash_fetch_cb(int err, void* userdata)
{
	(void) err;
	(void) userdata;

	splash_update_display();
}

static void splash_network_fetch_cb(int err, void* userdata)
{
	nw_refresh_pending = false;
	splash_fetch_cb(err, userdata);
}

static void splash_update_display(void)
{
	char str[192];
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	const app_state_system_info_t* info = &snapshot.system_info;
	const app_state_nw_if_t* nw_if = &snapshot.nw_if;
	
	// Check if initialization is complete
	if (!flag_init && info->valid && strcmp(info->product_name, "N/A") != 0) {
		flag_init = true;
		if (init_spinner != NULL) {
			lv_obj_del(init_spinner);
			init_spinner = NULL;
		}
	}
	
	snprintf(str, sizeof(str), "%s", "PowerIT Easy");
	lv_label_set_text(lbl_system, str);

	int nw_mode = nw_if->nw_mode;
	if (nw_mode < NW_MODE_SINGLE_LAN || nw_mode > NW_MODE_LAN_WIFI) {
		nw_mode = (nw_if->type == NW_TYPE_WIFI_DHCP ||
				nw_if->type == NW_TYPE_WIFI_STATIC) ?
				NW_MODE_WIFI_ONLY : NW_MODE_SINGLE_LAN;
	}
	lv_obj_set_height(info_cont,
			(nw_mode == NW_MODE_SINGLE_LAN ||
			 nw_mode == NW_MODE_WIFI_ONLY) ? 60 : 80);

	const char* eth_ip = first_ip(nw_if->lan1_ip, nw_if->ip,
			DEFAULT_ETH_IP);
	const char* wifi_ip = first_ip(nw_if->wifi_ip, nw_if->ip,
			DEFAULT_ETH_IP);

	switch (nw_mode) {
	case NW_MODE_DUAL_LAN:
		snprintf(str, sizeof(str), "IP: %s (ETH1)\nIP: %s (ETH2)",
				eth_ip, first_ip(nw_if->lan2_ip, NULL,
						DEFAULT_SECONDARY_IP));
		break;
	case NW_MODE_WIFI_ONLY:
		snprintf(str, sizeof(str), "IP: %s (WiFi)", wifi_ip);
		break;
	case NW_MODE_LAN_WIFI:
		snprintf(str, sizeof(str), "IP: %s (ETH)\nIP: %s (WiFi)",
				eth_ip, first_ip(nw_if->wifi_ip, NULL,
						DEFAULT_SECONDARY_IP));
		break;
	case NW_MODE_SINGLE_LAN:
	default:
		snprintf(str, sizeof(str), "IP: %s (ETH)", eth_ip);
		break;
	}
	lv_label_set_text(lbl_ip, str);
}

static void splash_timer_cb(lv_timer_t* timer)
{
	(void) timer;
	
	backend_system_info_refresh(splash_fetch_cb, NULL);
	if (!nw_refresh_pending &&
			backend_network_if_refresh(splash_network_fetch_cb, NULL) == 0) {
		nw_refresh_pending = true;
	}
	
	splash_update_display();
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

lv_obj_t* scr_splash_create(lv_obj_t* prev_scr)
{
	splash_scr = lv_obj_create(NULL);
	next_scr = prev_scr;

	lv_obj_set_size(splash_scr, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_radius(splash_scr, 0, 0);
	lv_obj_set_style_bg_color(splash_scr, lv_color_hex(TT_COLOR_BG1), 0);

	lv_obj_add_flag(splash_scr, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_t* logo = lv_img_create(splash_scr);
	lv_img_set_src(logo, ASSET("ne_logo.png"));
	lv_obj_add_event_cb(splash_scr, splash_cb, LV_EVENT_CLICKED, prev_scr);

	init_spinner = tt_obj_spinner_inline_create(splash_scr,
			"Initializing system...");
	info_cont = tt_obj_cont_create(splash_scr);
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

void scr_splash_set_next_scr(lv_obj_t* l_next_scr)
{
	next_scr = l_next_scr;
}

void scr_splash_show()
{
	if (lv_scr_act() != splash_scr) {
		lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
		lv_timer_resume(timer_check);
		lv_scr_load(splash_scr);
	}
}
