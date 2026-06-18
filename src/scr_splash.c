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

#define NW_TYPE_ETH_DHCP 2
#define NW_TYPE_ETH_STATIC 3
#define NW_TYPE_WIFI_DHCP 4
#define NW_TYPE_WIFI_STATIC 5

#define TIMER_REFRESH_RATE 10000 // ms

/* Global variables ***********************************************************/

static lv_obj_t* splash_scr;
static lv_obj_t* next_scr;

static lv_timer_t* timer_check;

static lv_obj_t* init_spinner;

static lv_obj_t* lbl_system;
static lv_obj_t* lbl_ip;

static bool flag_init = false;

/* Function prototypes ********************************************************/

static void splash_cb(lv_event_t* e);
static void splash_fetch_cb(int err, void* userdata);
static void splash_update_display(void);
static void splash_timer_cb(lv_timer_t* timer);

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

static void splash_update_display(void)
{
	char str[192];
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	const app_state_system_info_t* info = &snapshot.system_info;
	const app_state_nw_if_t* nw_if = &snapshot.nw_if;
	const char* ip = nw_if->ip;
	
	// Check if initialization is complete
	if (!flag_init && info->valid && strcmp(info->product_name, "N/A") != 0) {
		flag_init = true;
		if (init_spinner != NULL) {
			lv_obj_del(init_spinner);
			init_spinner = NULL;
		}
	}
	
	const char* iface = "";
	if (nw_if->type == NW_TYPE_ETH_DHCP || nw_if->type == NW_TYPE_ETH_STATIC) {
		iface = "(ETH)";
	} else if (nw_if->type == NW_TYPE_WIFI_DHCP || nw_if->type == NW_TYPE_WIFI_STATIC) {
		iface = "(WIFI)";
	}

	if (ip == NULL || ip[0] == '\0') {
		if (nw_if->lan1_ip[0] != '\0') {
			ip = nw_if->lan1_ip;
			iface = "(LAN1)";
		} else if (nw_if->wifi_ip[0] != '\0') {
			ip = nw_if->wifi_ip;
			iface = "(WIFI)";
		} else if (strcmp(info->ip, "N/A") != 0) {
			ip = info->ip;
		} else {
			ip = "";
		}
	}
	
	snprintf(str, sizeof(str), "%s", "PowerIT Easy");
	lv_label_set_text(lbl_system, str);
	snprintf(str, sizeof(str), "%s: %s %s", "IP", ip, iface);
	lv_label_set_text(lbl_ip, str);
}

static void splash_timer_cb(lv_timer_t* timer)
{
	(void) timer;
	
	backend_system_info_refresh(splash_fetch_cb, NULL);
	backend_network_if_refresh(splash_fetch_cb, NULL);
	
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
