#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_settings_nw.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"

#define TIMER_NW_CHECK_PERIOD 2000 // ms
#define TIMER_MSG_BOX_PERIOD 10000 // ms
#define MAX_NW_CONN_RETRIES 5

typedef enum {
    NW_ETH  = 0,
    NW_WIFI = 1,
} dd_opts_t;

/* Global variables ***********************************************************/

static models_nw_if_t nw_ifaces;

static lv_obj_t* loader_scr;

static lv_obj_t* dd;
static lv_obj_t* btn_dhcp;

static lv_obj_t* lbl_wifi_ssid;
static lv_obj_t* lbl_wifi_pass;
static lv_obj_t* lbl_ip;
static lv_obj_t* lbl_mask;
static lv_obj_t* lbl_gw;
static lv_obj_t* lbl_dns;

static lv_obj_t* cbx_pass;

static lv_obj_t* txt_wifi_ssid;
static lv_obj_t* txt_wifi_pass;
static lv_obj_t* txt_ip;
static lv_obj_t* txt_mask;
static lv_obj_t* txt_gw;
static lv_obj_t* txt_dns;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void update_cb(lv_event_t* e);
static void msg_box_timer_cb(lv_timer_t* timer);
static void nw_if_timer_cb(lv_timer_t* timer);
static void loader_cb(lv_event_t* e);
static void msg_box_nw_if_cb(lv_event_t* e);
static void btn_nw_settings_cb(lv_event_t* e);
static void txt_cb(lv_event_t* e);
static void txt_num_cb(lv_event_t* e);
static void cbx_pass_cb(lv_event_t* e);

static void update_data();
static bool is_ethernet();
static bool is_static();

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(menu);
		if (curr_page == page) {
			controller_get_nw_if();
			const models_nw_if_t* nw_if = models_get_nw_if();
			if (is_ethernet()) {
				lv_dropdown_set_selected(dd, NW_ETH);
			} else {
				lv_dropdown_set_selected(dd, NW_WIFI);
			}
			(void)is_static(); // Not used
			if (nw_if->dhcp) {
				lv_obj_add_state(btn_dhcp, LV_STATE_CHECKED);
			} else {
				lv_obj_clear_state(btn_dhcp, LV_STATE_CHECKED);
			}
			lv_textarea_set_text(txt_ip, nw_if->params.ip);
			lv_textarea_set_text(txt_mask, nw_if->params.mask);
			lv_textarea_set_text(txt_gw, nw_if->params.gw);
			lv_textarea_set_text(txt_dns, nw_if->params.dns);
			// TODO: uncomment
			// lv_textarea_set_text(txt_wifi_ssid, nw_if->params.ssid);
			// lv_textarea_set_text(txt_wifi_pass, nw_if->params.pass);
			update_data();
		}
	}
}

static void update_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		update_data();
	}
}

static void msg_box_timer_cb(lv_timer_t* timer)
{
	lv_timer_del(timer);
	lv_msgbox_close(timer->user_data);
}

static void nw_if_timer_cb(lv_timer_t* timer)
{
	static int retries = 0;
	// Call API network info
	controller_get_nw_info();
	const models_nw_info_t* nw_info = models_get_nw_info();
	bool connected = (bool)nw_info->connected;
	retries++;
	LV_LOG_USER("Connected: %s (%d/%d)", connected ? "yes" : "no",
			retries, MAX_NW_CONN_RETRIES);
	lv_obj_t* scr = timer->user_data;
	lv_obj_t* msg_box_conn;
	if (connected) {
		lv_timer_del(timer);
		lv_scr_load(scr);
		lv_obj_del(loader_scr);
		msg_box_conn = tt_obj_info_box_create("INFO", "Connected to Internet", 0);
		lv_timer_create(msg_box_timer_cb, TIMER_MSG_BOX_PERIOD, msg_box_conn);
		return;

	}
	if (retries > MAX_NW_CONN_RETRIES) {
		lv_timer_del(timer);
		msg_box_conn = tt_obj_info_box_create("ERROR", "Can not connect to Internet", 1);
		lv_timer_create(msg_box_timer_cb, TIMER_MSG_BOX_PERIOD, msg_box_conn);
		return;
	}
}

static void loader_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);
	lv_obj_t* scr = lv_event_get_user_data(e);

	if (code == LV_EVENT_CLICKED) {
		lv_scr_load(scr);
		lv_obj_del(obj);
	}
}

static void msg_box_nw_if_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			controller_put_nw_if(&nw_ifaces);
			lv_timer_create(nw_if_timer_cb, TIMER_NW_CHECK_PERIOD, lv_scr_act());
			char* txt = lv_event_get_user_data(e);
			loader_scr = tt_obj_loader_create(txt, NULL);
			//TODO: remove (touch for del [debug])
			lv_obj_add_event_cb(loader_scr, loader_cb, LV_EVENT_ALL, lv_scr_act());
			lv_scr_load(loader_scr);
		}
		lv_msgbox_close(obj);
	}
}

static void btn_nw_settings_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		bool dhcp = (bool)(lv_obj_get_state(btn_dhcp) & LV_STATE_CHECKED);
		const char* nw_type;
		if (lv_dropdown_get_selected(dd) == 0) { // Ethernet
			nw_type = "Ethernet";
			if (dhcp) {
				nw_ifaces.type = ETH_DHCP;
			} else {
				nw_ifaces.type = ETH_STATIC;
			}
		} else { // WIFI
			nw_type = "WiFi";
			if (dhcp) {
				nw_ifaces.type = WIFI_DHCP;
			} else {
				nw_ifaces.type = WIFI_STATIC;
			}
		}
		const char* nw_dhcp;
		if (dhcp) {
			nw_dhcp = "Enabled";
		} else {
			nw_dhcp = "Disabled";
		}
		nw_ifaces.dhcp = dhcp;
		nw_ifaces.params.ip = lv_textarea_get_text(txt_ip);
		nw_ifaces.params.mask = lv_textarea_get_text(txt_mask);
		nw_ifaces.params.gw = lv_textarea_get_text(txt_gw);
		nw_ifaces.params.dns = lv_textarea_get_text(txt_dns);
		nw_ifaces.params.ssid = lv_textarea_get_text(txt_wifi_ssid);
		nw_ifaces.params.pass = lv_textarea_get_text(txt_wifi_pass);
		if (nw_ifaces.params.pass == NULL) {
			nw_ifaces.params.pass = "";
		}

		char msg[1000];
		int len = 0;
		len += sprintf(msg, "Are you sure you want to save this network settings?\n\n"
				"Type: " TT_COLOR_GREEN_NE_STR " %s\n" \
				"DHCP: " TT_COLOR_GREEN_NE_STR " %s\n", nw_type, nw_dhcp);
		if (!lv_obj_has_flag(txt_wifi_ssid, LV_OBJ_FLAG_HIDDEN)) { // WiFi
			len += sprintf(msg + len, "SSID: " TT_COLOR_GREEN_NE_STR " %s\n"
				"Pass: " TT_COLOR_GREEN_NE_STR " *******\n",
				nw_ifaces.params.ssid);
		}
		if (!lv_obj_has_flag(txt_ip, LV_OBJ_FLAG_HIDDEN)) { // Static
			len += sprintf(msg + len, "IP: " TT_COLOR_GREEN_NE_STR " %s\n" \
				"Mask: " TT_COLOR_GREEN_NE_STR " %s\n" \
				"Gateway: " TT_COLOR_GREEN_NE_STR " %s\n" \
				"DNS: " TT_COLOR_GREEN_NE_STR " %s",
				nw_ifaces.params.ip,
				nw_ifaces.params.mask,
				nw_ifaces.params.gw,
				nw_ifaces.params.dns);
		} 
		tt_obj_msg_box_create("Network settings",
				msg, "Connecting to internet...", msg_box_nw_if_cb);
	}
}

static void txt_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_ABC);
		lv_scr_load(kb_scr);
	}
	if (code == LV_EVENT_READY) {
		// Ready event from keyboard
	}
}

static void txt_num_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_NUM);
		lv_scr_load(kb_scr);
	}
	if (code == LV_EVENT_READY) {
		// Ready event from keyboard
	}
}

static void cbx_pass_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_obj_get_state(obj) & LV_STATE_CHECKED) {
			lv_textarea_set_password_mode(txt_wifi_pass, false);
		} else {
			lv_textarea_set_password_mode(txt_wifi_pass, true);
		}
	}
}

/* Function definitions *******************************************************/

static bool is_ethernet()
{
	const models_nw_if_t* nw_if = models_get_nw_if();
	return (nw_if->type == ETH_DHCP || nw_if->type == ETH_STATIC);
}

static bool is_static()
{
	const models_nw_if_t* nw_if = models_get_nw_if();
	return (nw_if->type == WIFI_STATIC || nw_if->type == ETH_STATIC);
}

static void update_data()
{
	if (lv_dropdown_get_selected(dd) == 0) { // Ethernet
		lv_obj_add_flag(lbl_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(txt_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_wifi_pass, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(txt_wifi_pass, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(cbx_pass, LV_OBJ_FLAG_HIDDEN);
	} else { // WIFI
		lv_obj_clear_flag(lbl_wifi_pass, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(txt_wifi_pass, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(txt_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(cbx_pass, LV_OBJ_FLAG_HIDDEN);
	}
	if (lv_obj_get_state(btn_dhcp) & LV_STATE_CHECKED) {
		lv_obj_add_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_mask, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_gw, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_dns, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(txt_ip, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(txt_mask, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(txt_gw, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(txt_dns, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_clear_flag(txt_ip, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(txt_mask, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(txt_gw, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(txt_dns, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_mask, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_gw, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(lbl_dns, LV_OBJ_FLAG_HIDDEN);
	}
}

/* Public functions ***********************************************************/

void scr_settings_nw_create(lv_obj_t* menu, lv_obj_t* btn)
{
	const models_nw_if_t* nw_if = models_get_nw_if();

	// Settings / Network
	lv_obj_t* nw_cont = tt_obj_menu_page_create(menu, btn, menu_cb,
			"Settings / NW setup");

	lv_obj_t* nw_cont2 = tt_obj_cont_create(nw_cont);

	tt_obj_label_create(nw_cont2, "Conenction type");
	char* options = "Ethernet\nWiFi";
	dd = tt_obj_dropdown_create(nw_cont2, options, update_cb);
	btn_dhcp = tt_obj_btn_toggle_create(nw_cont2, update_cb, "DHCP");
	lv_obj_add_style(btn_dhcp, &btn_style, 0);
	lv_obj_set_height(btn_dhcp, 36);

	lbl_wifi_ssid = tt_obj_label_create(nw_cont2, "WiFi SSID");
	txt_wifi_ssid = tt_obj_txt_create(nw_cont2, "WiFi SSID", txt_cb);
	lv_textarea_set_text(txt_wifi_ssid, nw_if->params.ssid);
	lv_obj_add_flag(lbl_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(txt_wifi_ssid, LV_OBJ_FLAG_HIDDEN);

	lbl_wifi_pass = tt_obj_label_create(nw_cont2, "WiFi password");
	txt_wifi_pass = tt_obj_txt_create(nw_cont2, "WiFi password", txt_cb);
	lv_textarea_set_text(txt_wifi_pass, nw_if->params.pass);
	cbx_pass = tt_obj_checkbox_create(nw_cont2,
			"Show WiFi password", cbx_pass_cb);
	lv_textarea_set_password_mode(txt_wifi_pass, true);
	lv_obj_add_flag(lbl_wifi_pass, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(txt_wifi_pass, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(cbx_pass, LV_OBJ_FLAG_HIDDEN);

	lbl_ip = tt_obj_label_create(nw_cont2, "IP Address");
	txt_ip = tt_obj_txt_create(nw_cont2, "IP Address", txt_num_cb);
	lv_textarea_set_text(txt_ip, nw_if->params.ip);

	lbl_mask = tt_obj_label_create(nw_cont2, "Subnet Mask");
	txt_mask = tt_obj_txt_create(nw_cont2, "Subnet Mask", txt_num_cb);
	lv_textarea_set_text(txt_mask, nw_if->params.mask);

	lbl_gw = tt_obj_label_create(nw_cont2, "Gateway IP");
	txt_gw = tt_obj_txt_create(nw_cont2, "Gateway IP", txt_num_cb);
	lv_textarea_set_text(txt_gw, nw_if->params.gw);

	lbl_dns = tt_obj_label_create(nw_cont2, "DNS");
	txt_dns = tt_obj_txt_create(nw_cont2, "DNS", txt_num_cb);
	lv_textarea_set_text(txt_dns, nw_if->params.dns);

	tt_obj_btn_std_create(nw_cont2, btn_nw_settings_cb, "Save settings");
}
