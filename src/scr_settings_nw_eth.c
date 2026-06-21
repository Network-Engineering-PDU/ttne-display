#include <stdio.h>
#include <string.h>
#include "lvgl/lvgl.h"
#include "scr_settings_nw_eth.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "app/app_state.h"
#include "backend/backend.h"
#include "config.h"
#include "screen.h"
#include "ttne_display.h"

#define TIMER_NW_CHECK_PERIOD 2000 // ms
#define TIMER_MSG_BOX_PERIOD 10000 // ms
#define MAX_NW_CONN_RETRIES 5

#define NW_TYPE_UNCONF 1
#define NW_TYPE_ETH_DHCP 2
#define NW_TYPE_ETH_STATIC 3
#define NW_TYPE_WIFI_DHCP 4
#define NW_TYPE_WIFI_STATIC 5

typedef enum {
    NW_SINGLE_LAN  = 0,  // Single ethernet (eth0 or eth1)
    NW_WIFI_ONLY   = 1,  // WiFi only
    NW_DUAL_LAN    = 2,  // Both eth0 and eth1
    NW_LAN_WIFI    = 3,  // Ethernet + WiFi
    NW_ETH         = NW_SINGLE_LAN,
    NW_WIFI        = NW_WIFI_ONLY,
} dd_opts_t;

/* Global variables ***********************************************************/

static app_state_nw_if_t nw_ifaces;

static lv_obj_t* loader_scr;
static lv_obj_t* pending_prev_scr;
static int nw_conn_retries;
static bool nw_info_refresh_pending;
static bool nw_if_refresh_pending;

static lv_obj_t* dd;
static lv_obj_t* btn_dhcp;

/* Mode containers */
static lv_obj_t* cont_single_lan;
static lv_obj_t* cont_wifi_only;
static lv_obj_t* cont_dual_lan;

/* Single LAN fields */
static lv_obj_t* lbl_ip;
static lv_obj_t* lbl_mask;
static lv_obj_t* lbl_gw;
static lv_obj_t* lbl_dns;
static lv_obj_t* txt_ip;
static lv_obj_t* txt_mask;
static lv_obj_t* txt_gw;
static lv_obj_t* txt_dns;

/* WiFi only fields */
static lv_obj_t* lbl_wifi_ssid;
static lv_obj_t* txt_wifi_ssid;
static lv_obj_t* lbl_wifi_pass;
static lv_obj_t* txt_wifi_pass;
static lv_obj_t* cbx_pass;

/* Dual LAN fields */
static lv_obj_t* lbl_lan1;
static lv_obj_t* lbl_lan1_ip;
static lv_obj_t* txt_lan1_ip;
static lv_obj_t* lbl_lan1_mask;
static lv_obj_t* txt_lan1_mask;
static lv_obj_t* lbl_lan2;
static lv_obj_t* lbl_lan2_ip;
static lv_obj_t* txt_lan2_ip;
static lv_obj_t* lbl_lan2_mask;
static lv_obj_t* txt_lan2_mask;

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
static bool is_static_network(const app_state_nw_if_t* nw_if);
static void network_if_refresh_cb(int err, void* userdata);
static void network_if_save_cb(int err, void* userdata);
static void network_info_refresh_cb(int err, void* userdata);
static void load_network_form(const app_state_nw_if_t* nw_if);
static uint16_t determine_network_mode(const app_state_nw_if_t* nw_if);
static const char* sanitize_dns(const char* dns);

/* Callbacks ******************************************************************/

/**
 * Determine the network mode from saved network interface configuration
 * by examining the multi-interface IP fields and SSID
 */
static uint16_t determine_network_mode(const app_state_nw_if_t* nw_if)
{
	bool has_lan1 = strlen(nw_if->lan1_ip) > 0;
	bool has_lan2 = strlen(nw_if->lan2_ip) > 0;
	bool has_wifi_marker = strlen(nw_if->wifi_ip) > 0;
	bool has_ssid = strlen(nw_if->ssid) > 0;
	
	/* Determine mode based on multi-interface configuration */
	if (has_wifi_marker && has_lan1 && has_lan2) {
		return NW_LAN_WIFI;  /* Both LAN interfaces and WiFi marker */
	} else if (has_lan1 && has_lan2) {
		return NW_DUAL_LAN;  /* Both LAN interfaces */
	} else if ((has_wifi_marker || has_ssid) && !has_lan1 && !has_lan2) {
		return NW_WIFI_ONLY; /* WiFi marker or SSID present, no LAN IPs */
	} else if (has_wifi_marker && has_ssid && !has_lan1 && has_lan2) {
		return NW_LAN_WIFI;  /* WiFi marker and SSID, with one LAN - treat as LAN+WiFi */
	} else if (has_ssid && has_lan1 && !has_lan2 && has_wifi_marker) {
		return NW_LAN_WIFI;  /* SSID + LAN1 + wifi_marker = LAN+WiFi */
	} else {
		return NW_SINGLE_LAN; /* Default to Single LAN */
	}
}

static const char* sanitize_dns(const char* dns)
{
	static char sanitized[256];
	size_t j = 0;

	if (dns == NULL) {
		dns = "";
	}

	for (size_t i = 0; dns[i] != '\0' && j < sizeof(sanitized) - 1; i++) {
		if (dns[i] != ',') {
			sanitized[j++] = dns[i];
		}
	}
	sanitized[j] = '\0';

	return sanitized;
}

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(menu);
		if (curr_page == page) {
			if (!nw_if_refresh_pending &&
					backend_network_if_refresh(network_if_refresh_cb, NULL) == 0) {
				nw_if_refresh_pending = true;
			}
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

static void load_network_form(const app_state_nw_if_t* nw_if)
{
	if (nw_if == NULL || !nw_if->valid) {
		return;
	}

	uint16_t saved_mode;
	if (nw_if->nw_mode >= 0) {
		saved_mode = nw_if->nw_mode;
	} else {
		saved_mode = determine_network_mode(nw_if);
	}
	lv_dropdown_set_selected(dd, saved_mode);

	if (nw_if->dhcp) {
		lv_obj_add_state(btn_dhcp, LV_STATE_CHECKED);
	} else {
		lv_obj_clear_state(btn_dhcp, LV_STATE_CHECKED);
	}

	switch (saved_mode) {
	case NW_SINGLE_LAN:
		lv_textarea_set_text(txt_ip,
				strlen(nw_if->lan1_ip) > 0 ? nw_if->lan1_ip : nw_if->ip);
		break;
	case NW_DUAL_LAN:
		lv_textarea_set_text(txt_lan1_ip, nw_if->lan1_ip);
		lv_textarea_set_text(txt_lan2_ip, nw_if->lan2_ip);
		break;
	case NW_LAN_WIFI:
		if (strlen(nw_if->lan1_ip) > 0) {
			lv_textarea_set_text(txt_lan1_ip, nw_if->lan1_ip);
		}
		break;
	default:
		lv_textarea_set_text(txt_ip, nw_if->ip);
		break;
	}

	lv_textarea_set_text(txt_mask, nw_if->mask);
	lv_textarea_set_text(txt_gw, nw_if->gw);
	lv_textarea_set_text(txt_dns, sanitize_dns(nw_if->dns));
	lv_textarea_set_text(txt_wifi_ssid, nw_if->ssid);
	lv_textarea_set_text(txt_wifi_pass, nw_if->pass);

	if ((saved_mode == NW_DUAL_LAN || saved_mode == NW_LAN_WIFI) &&
			!nw_if->dhcp && strlen(nw_if->mask) > 0) {
		lv_textarea_set_text(txt_lan1_mask, nw_if->mask);
		lv_textarea_set_text(txt_lan2_mask, nw_if->mask);
	}

	update_data();
}

static void network_if_refresh_cb(int err, void* userdata)
{
	(void)userdata;
	nw_if_refresh_pending = false;
	if (err != 0) {
		return;
	}

	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	load_network_form(&snapshot.nw_if);
}

static void nw_if_timer_cb(lv_timer_t* timer)
{
	if (nw_info_refresh_pending) {
		return;
	}

	if (backend_network_info_refresh(network_info_refresh_cb, timer) == 0) {
		nw_info_refresh_pending = true;
	}
}

static void network_info_refresh_cb(int err, void* userdata)
{
	lv_timer_t* timer = userdata;
	lv_obj_t* scr = timer != NULL ? timer->user_data : pending_prev_scr;
	lv_obj_t* msg_box_conn;

	nw_info_refresh_pending = false;
	if (timer == NULL) {
		return;
	}

	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	bool connected = err == 0 && snapshot.nw_info.valid &&
			snapshot.nw_info.connected;
	nw_conn_retries++;
	LV_LOG_USER("Connected: %s (%d/%d)", connected ? "yes" : "no",
			nw_conn_retries, MAX_NW_CONN_RETRIES);

	if (connected) {
		nw_conn_retries = 0;
		lv_timer_del(timer);
		lv_scr_load(scr);
		lv_obj_del(loader_scr);
		msg_box_conn = tt_obj_info_box_create("INFO", "Connected to Internet", 0);
		lv_timer_create(msg_box_timer_cb, TIMER_MSG_BOX_PERIOD, msg_box_conn);
		return;
	}
	if (nw_conn_retries > MAX_NW_CONN_RETRIES) {
		nw_conn_retries = 0;
		lv_timer_del(timer);
		lv_scr_load(scr);
		lv_obj_del(loader_scr);
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
			pending_prev_scr = lv_scr_act();
			char* txt = lv_event_get_user_data(e);
			loader_scr = tt_obj_loader_create(txt, NULL);
			lv_obj_add_event_cb(loader_scr, loader_cb, LV_EVENT_ALL,
					pending_prev_scr);
			lv_scr_load(loader_scr);
			backend_network_if_save(&nw_ifaces, network_if_save_cb, NULL);
		}
		lv_msgbox_close(obj);
	}
}

static void network_if_save_cb(int err, void* userdata)
{
	(void)userdata;
	lv_obj_t* msg_box_conn;

	if (err != 0) {
		lv_scr_load(pending_prev_scr);
		if (loader_scr != NULL) {
			lv_obj_del(loader_scr);
			loader_scr = NULL;
		}
		msg_box_conn = tt_obj_info_box_create("ERROR",
				"Can not apply network configuration", 1);
		lv_timer_create(msg_box_timer_cb, TIMER_MSG_BOX_PERIOD, msg_box_conn);
		return;
	}

	if (is_static_network(&nw_ifaces)) {
		lv_scr_load(pending_prev_scr);
		if (loader_scr != NULL) {
			lv_obj_del(loader_scr);
			loader_scr = NULL;
		}
		msg_box_conn = tt_obj_info_box_create("INFO",
				"     Configuration Applied\n     Please wait a moment...", 0);
		lv_timer_create(msg_box_timer_cb, TIMER_MSG_BOX_PERIOD, msg_box_conn);
		return;
	}

	nw_conn_retries = 0;
	lv_timer_create(nw_if_timer_cb, TIMER_NW_CHECK_PERIOD, pending_prev_scr);
}

static void btn_nw_settings_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		bool dhcp = (bool)(lv_obj_get_state(btn_dhcp) & LV_STATE_CHECKED);
		const char* nw_type;
		app_state_snapshot_t snapshot;
		app_state_get_snapshot(&snapshot);
		snprintf(nw_ifaces.eth_interface, sizeof(nw_ifaces.eth_interface),
				"%s", snapshot.nw_if.eth_interface);
		uint16_t selected_mode = lv_dropdown_get_selected(dd);
		
		/* Determine network type based on selected mode */
		switch (selected_mode) {
			case NW_SINGLE_LAN:
				nw_type = "Single LAN";
				if (dhcp) {
					nw_ifaces.type = NW_TYPE_ETH_DHCP;
				} else {
					nw_ifaces.type = NW_TYPE_ETH_STATIC;
				}
				break;
			case NW_WIFI_ONLY:
				nw_type = "WiFi Only";
				if (dhcp) {
					nw_ifaces.type = NW_TYPE_WIFI_DHCP;
				} else {
					nw_ifaces.type = NW_TYPE_WIFI_STATIC;
				}
				break;
			case NW_DUAL_LAN:
				nw_type = "Dual LAN";
				if (dhcp) {
					nw_ifaces.type = NW_TYPE_ETH_DHCP;
				} else {
					nw_ifaces.type = NW_TYPE_ETH_STATIC;
				}
				break;
			case NW_LAN_WIFI:
				nw_type = "LAN + WiFi";
				if (dhcp) {
					nw_ifaces.type = NW_TYPE_ETH_DHCP;
				} else {
					nw_ifaces.type = NW_TYPE_ETH_STATIC;
				}
				break;
			default:
				nw_type = "Unknown";
				break;
		}
		
		const char* nw_dhcp;
		if (dhcp) {
			nw_dhcp = "Enabled";
		} else {
			nw_dhcp = "Disabled";
		}
		nw_ifaces.dhcp = dhcp;
		nw_ifaces.nw_mode = selected_mode;  /* Store the selected network mode */
		snprintf(nw_ifaces.ip, sizeof(nw_ifaces.ip), "%s",
				lv_textarea_get_text(txt_ip));
		snprintf(nw_ifaces.mask, sizeof(nw_ifaces.mask), "%s",
				lv_textarea_get_text(txt_mask));
		snprintf(nw_ifaces.gw, sizeof(nw_ifaces.gw), "%s",
				lv_textarea_get_text(txt_gw));
		snprintf(nw_ifaces.dns, sizeof(nw_ifaces.dns), "%s",
				sanitize_dns(lv_textarea_get_text(txt_dns)));
		snprintf(nw_ifaces.ssid, sizeof(nw_ifaces.ssid), "%s",
				lv_textarea_get_text(txt_wifi_ssid));
		snprintf(nw_ifaces.pass, sizeof(nw_ifaces.pass), "%s",
				lv_textarea_get_text(txt_wifi_pass));
		
		/* Populate multi-interface IP fields based on selected mode */
		switch (selected_mode) {
			case NW_SINGLE_LAN:
				snprintf(nw_ifaces.lan1_ip, sizeof(nw_ifaces.lan1_ip), "%s",
						lv_textarea_get_text(txt_ip));
				nw_ifaces.lan2_ip[0] = '\0';
				nw_ifaces.wifi_ip[0] = '\0';
				break;
			case NW_WIFI_ONLY:
				nw_ifaces.lan1_ip[0] = '\0';
				nw_ifaces.lan2_ip[0] = '\0';
				snprintf(nw_ifaces.wifi_ip, sizeof(nw_ifaces.wifi_ip), "%s",
						"wifi");
				break;
			case NW_DUAL_LAN:
				snprintf(nw_ifaces.lan1_ip, sizeof(nw_ifaces.lan1_ip), "%s",
						lv_textarea_get_text(txt_lan1_ip));
				snprintf(nw_ifaces.lan2_ip, sizeof(nw_ifaces.lan2_ip), "%s",
						lv_textarea_get_text(txt_lan2_ip));
				nw_ifaces.wifi_ip[0] = '\0';
				break;
			case NW_LAN_WIFI:
				snprintf(nw_ifaces.lan1_ip, sizeof(nw_ifaces.lan1_ip), "%s",
						lv_textarea_get_text(txt_lan1_ip));
				nw_ifaces.lan2_ip[0] = '\0';
				snprintf(nw_ifaces.wifi_ip, sizeof(nw_ifaces.wifi_ip), "%s",
						"wifi");
				break;
			default:
				nw_ifaces.lan1_ip[0] = '\0';
				nw_ifaces.lan2_ip[0] = '\0';
				nw_ifaces.wifi_ip[0] = '\0';
				break;
		}

		char msg[1500];
		int len = 0;
		len += sprintf(msg, "Are you sure you want to save this network settings?\n\n"
				"Type: " TT_COLOR_GREEN_NE_STR " %s\n" \
				"DHCP: " TT_COLOR_GREEN_NE_STR " %s\n", nw_type, nw_dhcp);
		
		/* Add mode-specific information */
		if (selected_mode == NW_WIFI_ONLY || selected_mode == NW_LAN_WIFI) {
			if (!lv_obj_has_flag(txt_wifi_ssid, LV_OBJ_FLAG_HIDDEN)) {
				len += sprintf(msg + len, "WiFi SSID: " TT_COLOR_GREEN_NE_STR " %s\n"
					"WiFi Pass: " TT_COLOR_GREEN_NE_STR " *******\n",
					nw_ifaces.ssid);
			}
		}
		
		if (selected_mode == NW_SINGLE_LAN || selected_mode == NW_DUAL_LAN || selected_mode == NW_LAN_WIFI) {
			if (selected_mode == NW_DUAL_LAN) {
				len += sprintf(msg + len, "LAN1 IP: " TT_COLOR_GREEN_NE_STR " %s\n"
					"LAN1 Mask: " TT_COLOR_GREEN_NE_STR " %s\n"
					"LAN2 IP: " TT_COLOR_GREEN_NE_STR " %s\n"
					"LAN2 Mask: " TT_COLOR_GREEN_NE_STR " %s\n",
					lv_textarea_get_text(txt_lan1_ip),
					lv_textarea_get_text(txt_lan1_mask),
					lv_textarea_get_text(txt_lan2_ip),
					lv_textarea_get_text(txt_lan2_mask));
			}
			
			if (!lv_obj_has_flag(txt_ip, LV_OBJ_FLAG_HIDDEN)) {
				len += sprintf(msg + len, "IP: " TT_COLOR_GREEN_NE_STR " %s\n" \
					"Mask: " TT_COLOR_GREEN_NE_STR " %s\n" \
					"Gateway: " TT_COLOR_GREEN_NE_STR " %s\n" \
					"DNS: " TT_COLOR_GREEN_NE_STR " %s",
					nw_ifaces.ip,
					nw_ifaces.mask,
					nw_ifaces.gw,
					nw_ifaces.dns);
			}
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
		if (obj == txt_dns) {
			lv_textarea_set_text(txt_dns,
					sanitize_dns(lv_textarea_get_text(txt_dns)));
		}
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

static bool is_static_network(const app_state_nw_if_t* nw_if)
{
	return nw_if != NULL &&
			(nw_if->type == NW_TYPE_WIFI_STATIC ||
			 nw_if->type == NW_TYPE_ETH_STATIC);
}

static void update_data()
{
	uint16_t selected = lv_dropdown_get_selected(dd);
	bool dhcp_enabled = (lv_obj_get_state(btn_dhcp) & LV_STATE_CHECKED) != 0;

	/* Hide all mode containers first */
	lv_obj_add_flag(cont_single_lan, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(cont_wifi_only, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(cont_dual_lan, LV_OBJ_FLAG_HIDDEN);

	/* Handle DHCP/Static IP fields visibility for containers that use them */
	if (dhcp_enabled) {
		lv_obj_add_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(txt_ip, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_mask, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(txt_mask, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_gw, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(txt_gw, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lbl_dns, LV_OBJ_FLAG_HIDDEN);
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

	/* Mode-specific container and field visibility */
	switch (selected) {
		case NW_SINGLE_LAN:
			/* Single LAN: show single LAN container */
			lv_obj_clear_flag(cont_single_lan, LV_OBJ_FLAG_HIDDEN);
			break;

		case NW_WIFI_ONLY:
			/* WiFi only: show WiFi container */
			lv_obj_clear_flag(cont_wifi_only, LV_OBJ_FLAG_HIDDEN);
			break;

		case NW_DUAL_LAN:
			/* Dual LAN: show dual LAN container */
			lv_obj_clear_flag(cont_dual_lan, LV_OBJ_FLAG_HIDDEN);
			if (!dhcp_enabled) {
				/* Show gateway and DNS for dual LAN static config */
				lv_obj_clear_flag(lbl_gw, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(txt_gw, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(lbl_dns, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(txt_dns, LV_OBJ_FLAG_HIDDEN);
			}
			break;

		case NW_LAN_WIFI:
			/* LAN & WiFi: show single LAN container + WiFi container */
			lv_obj_clear_flag(cont_single_lan, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(cont_wifi_only, LV_OBJ_FLAG_HIDDEN);
			break;

		default:
			break;
	}
}

/* Public functions ***********************************************************/

void scr_settings_nw_eth_create(lv_obj_t* menu, lv_obj_t* btn)
{
	app_state_snapshot_t snapshot;
	app_state_get_snapshot(&snapshot);
	const app_state_nw_if_t* nw_if = &snapshot.nw_if;

	// Settings / Network
	lv_obj_t* nw_cont = tt_obj_menu_page_create(menu, btn, menu_cb, "Network Setup");
	lv_obj_t* nw_cont2 = tt_obj_cont_create(nw_cont);

	lv_obj_t* lbl_conn = tt_obj_label_create(nw_cont2, "Connection type");
	lv_label_set_long_mode(lbl_conn, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_scrollbar_mode(lbl_conn, LV_SCROLLBAR_MODE_OFF);

	lv_obj_t* controls_cont = lv_obj_create(nw_cont2);
	lv_obj_set_size(controls_cont, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(controls_cont, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_align(controls_cont, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_scrollbar_mode(controls_cont, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(controls_cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_opa(controls_cont, LV_OPA_0, 0);
	lv_obj_set_style_border_width(controls_cont, 0, 0);
	lv_obj_set_style_pad_all(controls_cont, 0, 0);
	lv_obj_set_style_pad_column(controls_cont, 6, 0);
	
	char* options = "Single LAN\nWiFi Only\nDual LAN\nLAN + WiFi";
	dd = tt_obj_dropdown_create(controls_cont, options, update_cb);
	lv_obj_set_width(dd, 130);
	lv_obj_set_height(dd, 36);
	
	btn_dhcp = tt_obj_btn_toggle_create(controls_cont, update_cb, "DHCP");
	lv_obj_add_style(btn_dhcp, &btn_style, 0);
	lv_obj_set_height(btn_dhcp, 36);
	lv_obj_set_width(btn_dhcp, 90);

	/* Single LAN mode container */
	cont_single_lan = tt_obj_cont_create(nw_cont2);

	/* Single LAN fields */
	lbl_ip = tt_obj_label_create(cont_single_lan, "IP Address");
	txt_ip = tt_obj_txt_create(cont_single_lan, "IP Address", txt_num_cb);
	lv_textarea_set_text(txt_ip, nw_if->ip);

	lbl_mask = tt_obj_label_create(cont_single_lan, "Subnet Mask");
	txt_mask = tt_obj_txt_create(cont_single_lan, "Subnet Mask", txt_num_cb);
	lv_textarea_set_text(txt_mask, nw_if->mask);

	lbl_gw = tt_obj_label_create(cont_single_lan, "Gateway IP");
	txt_gw = tt_obj_txt_create(cont_single_lan, "Gateway IP", txt_num_cb);
	lv_textarea_set_text(txt_gw, nw_if->gw);

	lbl_dns = tt_obj_label_create(cont_single_lan, "DNS");
	txt_dns = tt_obj_txt_create(cont_single_lan, "DNS", txt_num_cb);
	lv_textarea_set_accepted_chars(txt_dns, "0123456789.");
	lv_textarea_set_text(txt_dns, sanitize_dns(nw_if->dns));

	/* WiFi only mode container */
	cont_wifi_only = tt_obj_cont_create(nw_cont2);

	/* WiFi only fields */
	lbl_wifi_ssid = tt_obj_label_create(cont_wifi_only, "WiFi SSID");
	txt_wifi_ssid = tt_obj_txt_create(cont_wifi_only, "WiFi SSID", txt_cb);
	lv_textarea_set_text(txt_wifi_ssid, nw_if->ssid);

	lbl_wifi_pass = tt_obj_label_create(cont_wifi_only, "WiFi password");
	txt_wifi_pass = tt_obj_txt_create(cont_wifi_only, "WiFi password", txt_cb);
	lv_textarea_set_text(txt_wifi_pass, nw_if->pass);
	cbx_pass = tt_obj_checkbox_create(cont_wifi_only, "Show WiFi password", cbx_pass_cb);
	lv_textarea_set_password_mode(txt_wifi_pass, true);

	/* Dual LAN mode container */
	cont_dual_lan = tt_obj_cont_create(nw_cont2);

	/* Dual LAN fields */
	lbl_lan1 = tt_obj_label_create(cont_dual_lan, "LAN1 Interface");
	lbl_lan1_ip = tt_obj_label_create(cont_dual_lan, "LAN1 IP Address");
	txt_lan1_ip = tt_obj_txt_create(cont_dual_lan, "LAN1 IP Address", txt_num_cb);

	lbl_lan1_mask = tt_obj_label_create(cont_dual_lan, "LAN1 Subnet Mask");
	txt_lan1_mask = tt_obj_txt_create(cont_dual_lan, "LAN1 Subnet Mask", txt_num_cb);

	lbl_lan2 = tt_obj_label_create(cont_dual_lan, "LAN2 Interface");
	lbl_lan2_ip = tt_obj_label_create(cont_dual_lan, "LAN2 IP Address");
	txt_lan2_ip = tt_obj_txt_create(cont_dual_lan, "LAN2 IP Address", txt_num_cb);

	lbl_lan2_mask = tt_obj_label_create(cont_dual_lan, "LAN2 Subnet Mask");
	txt_lan2_mask = tt_obj_txt_create(cont_dual_lan, "LAN2 Subnet Mask", txt_num_cb);

	tt_obj_btn_std_create(nw_cont2, btn_nw_settings_cb, "Save settings");
}
