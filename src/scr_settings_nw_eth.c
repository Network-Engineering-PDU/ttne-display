#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "i18n.h"
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
#define NW_SUCCESS_MSG_MS 2000 // ms
#define MAX_NW_CONN_RETRIES 5

#define NW_TYPE_UNCONF 1
#define NW_TYPE_ETH_DHCP 2
#define NW_TYPE_ETH_STATIC 3
#define NW_TYPE_WIFI_DHCP 4
#define NW_TYPE_WIFI_STATIC 5

#define DEFAULT_LAN1_IP "192.168.1.100"
#define DEFAULT_LAN2_IP "192.168.1.200"
#define DEFAULT_WIFI_IP "192.168.1.150"
#define DEFAULT_SUBNET_MASK "255.255.255.0"
#define DEFAULT_GATEWAY "192.168.1.1"
#define DEFAULT_DNS "8.8.8.8"

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

static lv_obj_t* save_msgbox;
static int nw_conn_retries;
static bool nw_info_refresh_pending;
static bool nw_if_refresh_pending;

static lv_obj_t* dd;
static lv_obj_t* btn_dhcp;

/* Mode containers */
static lv_obj_t* cont_single_lan;
static lv_obj_t* cont_wifi_only;
static lv_obj_t* cont_dual_lan;
static lv_obj_t* cont_lan_wifi_eth;
static lv_obj_t* cont_lan_wifi_wifi;

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

/* LAN + WiFi Ethernet fields */
static lv_obj_t* lbl_lw_eth_ip;
static lv_obj_t* txt_lw_eth_ip;
static lv_obj_t* lbl_lw_eth_mask;
static lv_obj_t* txt_lw_eth_mask;
static lv_obj_t* lbl_lw_eth_gw;
static lv_obj_t* txt_lw_eth_gw;
static lv_obj_t* lbl_lw_eth_dns;
static lv_obj_t* txt_lw_eth_dns;

/* LAN + WiFi WiFi fields */
static lv_obj_t* lbl_lw_wifi_ip;
static lv_obj_t* txt_lw_wifi_ip;
static lv_obj_t* lbl_lw_wifi_mask;
static lv_obj_t* txt_lw_wifi_mask;
static lv_obj_t* lbl_lw_wifi_gw;
static lv_obj_t* txt_lw_wifi_gw;
static lv_obj_t* lbl_lw_wifi_dns;
static lv_obj_t* txt_lw_wifi_dns;
static lv_obj_t* txt_lw_wifi_ssid;
static lv_obj_t* txt_lw_wifi_pass;
static lv_obj_t* cbx_lw_wifi_pass;

/* Dual LAN fields */
static lv_obj_t* lbl_lan1;
static lv_obj_t* lbl_lan1_ip;
static lv_obj_t* txt_lan1_ip;
static lv_obj_t* lbl_lan1_mask;
static lv_obj_t* txt_lan1_mask;
static lv_obj_t* lbl_lan1_gateway;
static lv_obj_t* txt_lan1_gateway;
static lv_obj_t* lbl_lan2;
static lv_obj_t* lbl_lan2_ip;
static lv_obj_t* txt_lan2_ip;
static lv_obj_t* lbl_lan2_mask;
static lv_obj_t* txt_lan2_mask;
static lv_obj_t* lbl_lan2_gateway;
static lv_obj_t* txt_lan2_gateway;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void update_cb(lv_event_t* e);
static void nw_if_timer_cb(lv_timer_t* timer);
static void msg_box_nw_if_cb(lv_event_t* e);
static void btn_nw_settings_cb(lv_event_t* e);
static void txt_cb(lv_event_t* e);
static void txt_num_cb(lv_event_t* e);
static void cbx_pass_cb(lv_event_t* e);

static void update_data();
static void set_lan_wifi_static_fields_hidden(bool hidden);
static bool is_static_network(const app_state_nw_if_t* nw_if);
static void network_if_refresh_cb(int err, void* userdata);
static void network_if_save_cb(int err, void* userdata);
static void network_info_refresh_cb(int err, void* userdata);
static void load_network_form(const app_state_nw_if_t* nw_if);
static uint16_t determine_network_mode(const app_state_nw_if_t* nw_if);
static const char* sanitize_dns(const char* dns);
static const char* default_if_empty(const char* value, const char* default_value);

/* Appends to buf like sprintf, but never past size: translated texts can be
 * longer than the English ones. Returns the new length. */
static int append_fmt(char* buf, size_t size, int len, const char* fmt, ...)
{
	if (len < 0 || (size_t)len >= size) {
		return len;
	}
	va_list args;
	va_start(args, fmt);
	int n = vsnprintf(buf + len, size - (size_t)len, fmt, args);
	va_end(args);
	if (n < 0) {
		return len;
	}
	return (size_t)len + (size_t)n < size ? len + n : (int)size - 1;
}

static void close_save_msgbox(void)
{
	if (save_msgbox != NULL && lv_obj_is_valid(save_msgbox)) {
		lv_msgbox_close(save_msgbox);
	}
	save_msgbox = NULL;
}

static void save_msgbox_timer_cb(lv_timer_t* timer)
{
	(void)timer;
	close_save_msgbox();
}

static void show_save_wait_msgbox(void)
{
	close_save_msgbox();
	save_msgbox = tt_obj_info_box_create(tr("Network settings"),
			tr("Applying network settings...\nPlease wait a moment."), 0);
	lv_obj_t* close_btn = lv_msgbox_get_close_btn(save_msgbox);
	if (close_btn != NULL) {
		lv_obj_add_flag(close_btn, LV_OBJ_FLAG_HIDDEN);
	}
}

static void show_save_success(const char* message)
{
	if (save_msgbox == NULL || !lv_obj_is_valid(save_msgbox)) {
		return;
	}
	i18n_label_set_text(lv_msgbox_get_text(save_msgbox), message);
	lv_timer_t* timer = lv_timer_create(save_msgbox_timer_cb,
			NW_SUCCESS_MSG_MS, NULL);
	lv_timer_set_repeat_count(timer, 1);
}

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

static const char* default_if_empty(const char* value, const char* default_value)
{
	return value != NULL && value[0] != '\0' ? value : default_value;
}

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(menu);
		if (curr_page == page) {
			app_state_snapshot_t snapshot;
			app_state_get_snapshot(&snapshot);
			if (snapshot.nw_if.valid) {
				load_network_form(&snapshot.nw_if);
			}
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

static void load_network_form(const app_state_nw_if_t* nw_if)
{
	if (nw_if == NULL) {
		return;
	}

	uint16_t saved_mode;
	if (nw_if->nw_mode >= 0) {
		saved_mode = nw_if->nw_mode;
	} else if (!nw_if->valid) {
		saved_mode = NW_SINGLE_LAN;
	} else {
		saved_mode = determine_network_mode(nw_if);
	}
	lv_dropdown_set_selected(dd, saved_mode);

	if (nw_if->dhcp) {
		lv_obj_add_state(btn_dhcp, LV_STATE_CHECKED);
	} else {
		lv_obj_clear_state(btn_dhcp, LV_STATE_CHECKED);
	}

	lv_textarea_set_text(txt_lan1_ip,
			default_if_empty(nw_if->lan1_ip, DEFAULT_LAN1_IP));
	lv_textarea_set_text(txt_lan2_ip,
			default_if_empty(nw_if->lan2_ip, DEFAULT_LAN2_IP));
	lv_textarea_set_text(txt_lan1_mask,
			default_if_empty(nw_if->mask, DEFAULT_SUBNET_MASK));
	lv_textarea_set_text(txt_lan1_gateway,
			default_if_empty(default_if_empty(nw_if->lan1_gateway, nw_if->gw),
					DEFAULT_GATEWAY));
	lv_textarea_set_text(txt_lan2_mask,
			default_if_empty(nw_if->mask, DEFAULT_SUBNET_MASK));
	lv_textarea_set_text(txt_lan2_gateway,
			default_if_empty(nw_if->lan2_gateway, DEFAULT_GATEWAY));

	switch (saved_mode) {
	case NW_SINGLE_LAN:
		lv_textarea_set_text(txt_ip,
				default_if_empty(default_if_empty(nw_if->lan1_ip, nw_if->ip),
						DEFAULT_LAN1_IP));
		break;
	case NW_LAN_WIFI:
		if (strlen(nw_if->lan1_ip) > 0) {
			lv_textarea_set_text(txt_lan1_ip, nw_if->lan1_ip);
		}
		break;
	default:
		lv_textarea_set_text(txt_ip,
				default_if_empty(nw_if->ip, DEFAULT_LAN1_IP));
		break;
	}

	lv_textarea_set_text(txt_mask,
			default_if_empty(nw_if->mask, DEFAULT_SUBNET_MASK));
	lv_textarea_set_text(txt_gw,
			default_if_empty(nw_if->gw, DEFAULT_GATEWAY));
	lv_textarea_set_text(txt_dns,
			sanitize_dns(default_if_empty(nw_if->dns, DEFAULT_DNS)));
	lv_textarea_set_text(txt_wifi_ssid, nw_if->ssid);
	lv_textarea_set_text(txt_lw_eth_ip,
			default_if_empty(default_if_empty(nw_if->lan1_ip, nw_if->ip),
					DEFAULT_LAN1_IP));
	lv_textarea_set_text(txt_lw_eth_mask,
			default_if_empty(nw_if->mask, DEFAULT_SUBNET_MASK));
	lv_textarea_set_text(txt_lw_eth_gw,
			default_if_empty(nw_if->gw, DEFAULT_GATEWAY));
	lv_textarea_set_text(txt_lw_eth_dns,
			sanitize_dns(default_if_empty(nw_if->dns, DEFAULT_DNS)));
	lv_textarea_set_text(txt_lw_wifi_ip,
			default_if_empty(nw_if->wifi_ip, DEFAULT_WIFI_IP));
	lv_textarea_set_text(txt_lw_wifi_mask,
			default_if_empty(nw_if->wifi_mask, DEFAULT_SUBNET_MASK));
	lv_textarea_set_text(txt_lw_wifi_gw,
			default_if_empty(nw_if->wifi_gateway, DEFAULT_GATEWAY));
	lv_textarea_set_text(txt_lw_wifi_dns,
			sanitize_dns(default_if_empty(nw_if->wifi_dns, DEFAULT_DNS)));
	lv_textarea_set_text(txt_lw_wifi_ssid, nw_if->ssid);
	/* The API intentionally does not return the stored WiFi secret: say
	 * whether one is kept (left empty, saving keeps it). Keep a password
	 * already entered in this session when a refresh returns empty. */
	const char* pass_hint = nw_if->pass_saved ? tr("Saved password") :
			tr("WiFi password");
	lv_textarea_set_placeholder_text(txt_wifi_pass, pass_hint);
	lv_textarea_set_placeholder_text(txt_lw_wifi_pass, pass_hint);
	if (nw_if->pass[0] != '\0') {
		lv_textarea_set_text(txt_wifi_pass, nw_if->pass);
		lv_textarea_set_text(txt_lw_wifi_pass, nw_if->pass);
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
		show_save_success(tr("Network settings applied\nConnected to Internet"));
		return;
	}
	if (nw_conn_retries > MAX_NW_CONN_RETRIES) {
		nw_conn_retries = 0;
		lv_timer_del(timer);
		close_save_msgbox();
		tt_obj_info_box_create_timed(tr("ERROR"),
				tr("Can not connect to Internet"), 1, TIMER_MSG_BOX_PERIOD);
		return;
	}
}

static void msg_box_nw_if_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			lv_msgbox_close(obj);
			show_save_wait_msgbox();
			if (backend_network_if_save(&nw_ifaces,
					network_if_save_cb, NULL) != 0) {
				close_save_msgbox();
				tt_obj_info_box_create(tr("ERROR"),
						tr("Can not apply network configuration"), 1);
			}
			return;
		}
		lv_msgbox_close(obj);
	}
}

static void network_if_save_cb(int err, void* userdata)
{
	(void)userdata;

	if (err != 0) {
		close_save_msgbox();
		tt_obj_info_box_create_timed(tr("ERROR"),
				tr("Can not apply network configuration"), 1,
				TIMER_MSG_BOX_PERIOD);
		return;
	}

	bool wifi_mode = nw_ifaces.nw_mode == NW_WIFI_ONLY ||
			nw_ifaces.nw_mode == NW_LAN_WIFI;
	if (is_static_network(&nw_ifaces) && !wifi_mode) {
		show_save_success(tr("Network settings applied"));
		return;
	}

	nw_conn_retries = 0;
	if (save_msgbox != NULL && lv_obj_is_valid(save_msgbox)) {
		i18n_label_set_text(lv_msgbox_get_text(save_msgbox),
				tr("Checking network connection...\nPlease wait a moment."));
	}
	lv_timer_create(nw_if_timer_cb, TIMER_NW_CHECK_PERIOD, NULL);
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
				nw_type = tr("Single LAN");
				if (dhcp) {
					nw_ifaces.type = NW_TYPE_ETH_DHCP;
				} else {
					nw_ifaces.type = NW_TYPE_ETH_STATIC;
				}
				break;
			case NW_WIFI_ONLY:
				nw_type = tr("WiFi Only");
				if (dhcp) {
					nw_ifaces.type = NW_TYPE_WIFI_DHCP;
				} else {
					nw_ifaces.type = NW_TYPE_WIFI_STATIC;
				}
				break;
			case NW_DUAL_LAN:
				nw_type = tr("Dual LAN");
				if (dhcp) {
					nw_ifaces.type = NW_TYPE_ETH_DHCP;
				} else {
					nw_ifaces.type = NW_TYPE_ETH_STATIC;
				}
				break;
			case NW_LAN_WIFI:
				nw_type = tr("LAN + WiFi");
				if (dhcp) {
					nw_ifaces.type = NW_TYPE_ETH_DHCP;
				} else {
					nw_ifaces.type = NW_TYPE_ETH_STATIC;
				}
				break;
			default:
				nw_type = tr("Unknown");
				break;
		}
		
		const char* nw_dhcp;
		if (dhcp) {
			nw_dhcp = tr("Enabled");
		} else {
			nw_dhcp = tr("Disabled");
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
		nw_ifaces.wifi_mask[0] = '\0';
		nw_ifaces.wifi_gateway[0] = '\0';
		nw_ifaces.wifi_dns[0] = '\0';
		
		/* Populate multi-interface IP fields based on selected mode */
		switch (selected_mode) {
			case NW_SINGLE_LAN:
				snprintf(nw_ifaces.lan1_ip, sizeof(nw_ifaces.lan1_ip), "%s",
						lv_textarea_get_text(txt_ip));
				nw_ifaces.eth_interface[0] = '\0';
				nw_ifaces.lan2_ip[0] = '\0';
				nw_ifaces.lan1_gateway[0] = '\0';
				nw_ifaces.lan2_gateway[0] = '\0';
				nw_ifaces.wifi_ip[0] = '\0';
				break;
			case NW_WIFI_ONLY:
				nw_ifaces.lan1_ip[0] = '\0';
				nw_ifaces.lan2_ip[0] = '\0';
				nw_ifaces.lan1_gateway[0] = '\0';
				nw_ifaces.lan2_gateway[0] = '\0';
				if (dhcp) {
					nw_ifaces.wifi_ip[0] = '\0';
				} else {
					snprintf(nw_ifaces.wifi_ip,
							sizeof(nw_ifaces.wifi_ip), "%s",
							lv_textarea_get_text(txt_ip));
				}
				break;
			case NW_DUAL_LAN:
				snprintf(nw_ifaces.lan1_ip, sizeof(nw_ifaces.lan1_ip), "%s",
						lv_textarea_get_text(txt_lan1_ip));
				snprintf(nw_ifaces.lan2_ip, sizeof(nw_ifaces.lan2_ip), "%s",
						lv_textarea_get_text(txt_lan2_ip));
				snprintf(nw_ifaces.lan1_gateway,
						sizeof(nw_ifaces.lan1_gateway), "%s",
						lv_textarea_get_text(txt_lan1_gateway));
				snprintf(nw_ifaces.lan2_gateway,
						sizeof(nw_ifaces.lan2_gateway), "%s",
						lv_textarea_get_text(txt_lan2_gateway));
				if (strcmp(nw_ifaces.lan1_ip, nw_ifaces.lan2_ip) == 0) {
					tt_obj_info_box_create_timed(tr("ERROR"),
							tr("LAN1 and LAN2 IP must be different"), 1,
							TIMER_MSG_BOX_PERIOD);
					return;
				}
				if (!dhcp && (nw_ifaces.lan1_gateway[0] == '\0' ||
						nw_ifaces.lan2_gateway[0] == '\0')) {
					tt_obj_info_box_create_timed(tr("ERROR"),
							tr("LAN1 and LAN2 gateways are required"), 1,
							TIMER_MSG_BOX_PERIOD);
					return;
				}
				snprintf(nw_ifaces.gw, sizeof(nw_ifaces.gw), "%s",
						strcmp(nw_ifaces.eth_interface, "eth0") == 0 ?
						nw_ifaces.lan2_gateway : nw_ifaces.lan1_gateway);
				nw_ifaces.wifi_ip[0] = '\0';
				break;
			case NW_LAN_WIFI:
				snprintf(nw_ifaces.ip, sizeof(nw_ifaces.ip), "%s",
						lv_textarea_get_text(txt_lw_eth_ip));
				snprintf(nw_ifaces.mask, sizeof(nw_ifaces.mask), "%s",
						lv_textarea_get_text(txt_lw_eth_mask));
				snprintf(nw_ifaces.gw, sizeof(nw_ifaces.gw), "%s",
						lv_textarea_get_text(txt_lw_eth_gw));
				snprintf(nw_ifaces.dns, sizeof(nw_ifaces.dns), "%s",
						sanitize_dns(lv_textarea_get_text(txt_lw_eth_dns)));
				snprintf(nw_ifaces.ssid, sizeof(nw_ifaces.ssid), "%s",
						lv_textarea_get_text(txt_lw_wifi_ssid));
				snprintf(nw_ifaces.pass, sizeof(nw_ifaces.pass), "%s",
						lv_textarea_get_text(txt_lw_wifi_pass));
				snprintf(nw_ifaces.lan1_ip, sizeof(nw_ifaces.lan1_ip), "%s",
						nw_ifaces.ip);
				nw_ifaces.lan2_ip[0] = '\0';
				snprintf(nw_ifaces.lan1_gateway,
						sizeof(nw_ifaces.lan1_gateway), "%s", nw_ifaces.gw);
				nw_ifaces.lan2_gateway[0] = '\0';
				snprintf(nw_ifaces.wifi_mask, sizeof(nw_ifaces.wifi_mask), "%s",
						lv_textarea_get_text(txt_lw_wifi_mask));
				snprintf(nw_ifaces.wifi_gateway,
						sizeof(nw_ifaces.wifi_gateway), "%s",
						lv_textarea_get_text(txt_lw_wifi_gw));
				snprintf(nw_ifaces.wifi_dns, sizeof(nw_ifaces.wifi_dns), "%s",
						sanitize_dns(lv_textarea_get_text(txt_lw_wifi_dns)));
				if (dhcp) {
					nw_ifaces.wifi_ip[0] = '\0';
				} else {
					snprintf(nw_ifaces.wifi_ip,
							sizeof(nw_ifaces.wifi_ip), "%s",
							lv_textarea_get_text(txt_lw_wifi_ip));
					if (nw_ifaces.wifi_ip[0] == '\0' ||
							nw_ifaces.wifi_mask[0] == '\0') {
						tt_obj_info_box_create_timed(tr("ERROR"),
								tr("WiFi IP and subnet mask are required"), 1,
								TIMER_MSG_BOX_PERIOD);
						return;
					}
					if (strcmp(nw_ifaces.ip, nw_ifaces.wifi_ip) == 0) {
						tt_obj_info_box_create_timed(tr("ERROR"),
								tr("Ethernet and WiFi IP must be different"), 1,
								TIMER_MSG_BOX_PERIOD);
						return;
					}
				}
				break;
			default:
				nw_ifaces.lan1_ip[0] = '\0';
				nw_ifaces.lan2_ip[0] = '\0';
				nw_ifaces.lan1_gateway[0] = '\0';
				nw_ifaces.lan2_gateway[0] = '\0';
				nw_ifaces.wifi_ip[0] = '\0';
				break;
		}

		/* A WiFi profile without a password never connects: an empty
		 * field is only fine while the PDU keeps this SSID's password. */
		if ((selected_mode == NW_WIFI_ONLY || selected_mode == NW_LAN_WIFI) &&
				nw_ifaces.pass[0] == '\0' &&
				!(snapshot.nw_if.pass_saved &&
				strcmp(nw_ifaces.ssid, snapshot.nw_if.ssid) == 0)) {
			tt_obj_info_box_create_timed(tr("ERROR"),
					tr("Enter the WiFi password"), 1, TIMER_MSG_BOX_PERIOD);
			return;
		}

		char msg[2048];
		int len = 0;
		len = append_fmt(msg, sizeof(msg), len, tr("Are you sure you want to save this network settings?\n\n"
				"Type: " TT_COLOR_GREEN_NE_STR " %s\n"
				"DHCP: " TT_COLOR_GREEN_NE_STR " %s\n"), nw_type, nw_dhcp);
		
		/* Add mode-specific information */
		if (selected_mode == NW_WIFI_ONLY || selected_mode == NW_LAN_WIFI) {
			len = append_fmt(msg, sizeof(msg), len, tr("WiFi SSID: " TT_COLOR_GREEN_NE_STR " %s\n"
				"WiFi Pass: " TT_COLOR_GREEN_NE_STR " *******\n"),
				nw_ifaces.ssid);
		}
		
		if (selected_mode == NW_SINGLE_LAN || selected_mode == NW_DUAL_LAN || selected_mode == NW_LAN_WIFI) {
			if (selected_mode == NW_DUAL_LAN) {
				len = append_fmt(msg, sizeof(msg), len, tr("LAN1 IP: " TT_COLOR_GREEN_NE_STR " %s\n"
					"LAN1 Mask: " TT_COLOR_GREEN_NE_STR " %s\n"
					"LAN1 Gateway: " TT_COLOR_GREEN_NE_STR " %s\n"
					"LAN2 IP: " TT_COLOR_GREEN_NE_STR " %s\n"
					"LAN2 Mask: " TT_COLOR_GREEN_NE_STR " %s\n"
					"LAN2 Gateway: " TT_COLOR_GREEN_NE_STR " %s\n"),
					lv_textarea_get_text(txt_lan1_ip),
					lv_textarea_get_text(txt_lan1_mask),
					lv_textarea_get_text(txt_lan1_gateway),
					lv_textarea_get_text(txt_lan2_ip),
					lv_textarea_get_text(txt_lan2_mask),
					lv_textarea_get_text(txt_lan2_gateway));
			}
			
			if (selected_mode == NW_LAN_WIFI && !dhcp) {
				len = append_fmt(msg, sizeof(msg), len,
						tr("Ethernet IP: " TT_COLOR_GREEN_NE_STR " %s\n"
						"Ethernet Mask: " TT_COLOR_GREEN_NE_STR " %s\n"
						"Ethernet Gateway: " TT_COLOR_GREEN_NE_STR " %s\n"
						"Ethernet DNS: " TT_COLOR_GREEN_NE_STR " %s\n"
						"WiFi IP: " TT_COLOR_GREEN_NE_STR " %s\n"
						"WiFi Mask: " TT_COLOR_GREEN_NE_STR " %s\n"
						"WiFi Gateway: " TT_COLOR_GREEN_NE_STR " %s\n"
						"WiFi DNS: " TT_COLOR_GREEN_NE_STR " %s"),
						nw_ifaces.ip, nw_ifaces.mask, nw_ifaces.gw,
						nw_ifaces.dns, nw_ifaces.wifi_ip,
						nw_ifaces.wifi_mask, nw_ifaces.wifi_gateway,
						nw_ifaces.wifi_dns);
			} else if (!lv_obj_has_flag(txt_ip, LV_OBJ_FLAG_HIDDEN)) {
				len = append_fmt(msg, sizeof(msg), len, tr("IP: " TT_COLOR_GREEN_NE_STR " %s\n"
					"Mask: " TT_COLOR_GREEN_NE_STR " %s\n"
					"Gateway: " TT_COLOR_GREEN_NE_STR " %s\n"
					"DNS: " TT_COLOR_GREEN_NE_STR " %s"),
					nw_ifaces.ip,
					nw_ifaces.mask,
					nw_ifaces.gw,
					nw_ifaces.dns);
			}
		}
		
		tt_obj_msg_box_create(tr("Network settings"),
				msg, tr("Connecting to internet..."), msg_box_nw_if_cb);
	}
}

static void txt_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_LATIN);
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
		if (obj == txt_dns || obj == txt_lw_eth_dns ||
				obj == txt_lw_wifi_dns) {
			lv_textarea_set_text(obj,
					sanitize_dns(lv_textarea_get_text(obj)));
		}
	}
}

static void cbx_pass_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* password = obj == cbx_lw_wifi_pass ?
				txt_lw_wifi_pass : txt_wifi_pass;
		lv_textarea_set_password_mode(password,
				!(lv_obj_get_state(obj) & LV_STATE_CHECKED));
	}
}

/* Function definitions *******************************************************/

static bool is_static_network(const app_state_nw_if_t* nw_if)
{
	return nw_if != NULL &&
			(nw_if->type == NW_TYPE_WIFI_STATIC ||
			 nw_if->type == NW_TYPE_ETH_STATIC);
}

static void set_lan_wifi_static_fields_hidden(bool hidden)
{
	lv_obj_t* fields[] = {
		lbl_lw_eth_ip, txt_lw_eth_ip,
		lbl_lw_eth_mask, txt_lw_eth_mask,
		lbl_lw_eth_gw, txt_lw_eth_gw,
		lbl_lw_eth_dns, txt_lw_eth_dns,
		lbl_lw_wifi_ip, txt_lw_wifi_ip,
		lbl_lw_wifi_mask, txt_lw_wifi_mask,
		lbl_lw_wifi_gw, txt_lw_wifi_gw,
		lbl_lw_wifi_dns, txt_lw_wifi_dns,
	};
	for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
		if (hidden) {
			lv_obj_add_flag(fields[i], LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_clear_flag(fields[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static void update_data()
{
	uint16_t selected = lv_dropdown_get_selected(dd);
	bool dhcp_enabled = (lv_obj_get_state(btn_dhcp) & LV_STATE_CHECKED) != 0;

	/* Hide all mode containers first */
	lv_obj_add_flag(cont_single_lan, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(cont_wifi_only, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(cont_dual_lan, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(cont_lan_wifi_eth, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(cont_lan_wifi_wifi, LV_OBJ_FLAG_HIDDEN);

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
			/* WiFi only: static addressing uses the common IP fields. */
			lv_obj_clear_flag(cont_wifi_only, LV_OBJ_FLAG_HIDDEN);
			if (!dhcp_enabled) {
				lv_obj_clear_flag(cont_single_lan, LV_OBJ_FLAG_HIDDEN);
			}
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
			lv_obj_clear_flag(cont_lan_wifi_eth, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(cont_lan_wifi_wifi, LV_OBJ_FLAG_HIDDEN);
			set_lan_wifi_static_fields_hidden(dhcp_enabled);
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
	lv_obj_t* nw_cont = tt_obj_menu_page_create(menu, btn, menu_cb, tr("Network Setup"));
	lv_obj_t* nw_cont2 = tt_obj_cont_create(nw_cont);

	lv_obj_t* lbl_conn = tt_obj_label_create(nw_cont2, tr("Connection type"));
	lv_label_set_long_mode(lbl_conn, LV_LABEL_LONG_SCROLL);
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
	
	const char* options = tr("Single LAN\nWiFi Only\nDual LAN\nLAN + WiFi");
	dd = tt_obj_dropdown_create(controls_cont, options, update_cb);
	lv_obj_set_width(dd, 140);
	lv_obj_set_height(dd, 36);
	
	btn_dhcp = tt_obj_btn_toggle_create(controls_cont, update_cb, "DHCP");
	lv_obj_add_style(btn_dhcp, &btn_style, 0);
	lv_obj_set_width(btn_dhcp, 140);
	lv_obj_set_height(btn_dhcp, 36);

	/* Single LAN mode container */
	cont_single_lan = tt_obj_cont_create(nw_cont2);

	/* Single LAN fields */
	lbl_ip = tt_obj_label_create(cont_single_lan, tr("IP Address"));
	txt_ip = tt_obj_address_txt_create(cont_single_lan, tr("IP Address"), txt_num_cb);
	lv_textarea_set_text(txt_ip,
			default_if_empty(nw_if->ip, DEFAULT_LAN1_IP));

	lbl_mask = tt_obj_label_create(cont_single_lan, tr("Subnet Mask"));
	txt_mask = tt_obj_address_txt_create(cont_single_lan, tr("Subnet Mask"), txt_num_cb);
	lv_textarea_set_text(txt_mask,
			default_if_empty(nw_if->mask, DEFAULT_SUBNET_MASK));

	lbl_gw = tt_obj_label_create(cont_single_lan, tr("Gateway IP"));
	txt_gw = tt_obj_address_txt_create(cont_single_lan, tr("Gateway IP"), txt_num_cb);
	lv_textarea_set_text(txt_gw,
			default_if_empty(nw_if->gw, DEFAULT_GATEWAY));

	lbl_dns = tt_obj_label_create(cont_single_lan, "DNS");
	txt_dns = tt_obj_address_txt_create(cont_single_lan, "DNS", txt_num_cb);
	lv_textarea_set_accepted_chars(txt_dns, "0123456789.");
	lv_textarea_set_text(txt_dns,
			sanitize_dns(default_if_empty(nw_if->dns, DEFAULT_DNS)));

	/* WiFi only mode container */
	cont_wifi_only = tt_obj_cont_create(nw_cont2);

	/* WiFi only fields */
	lbl_wifi_ssid = tt_obj_label_create(cont_wifi_only, tr("WiFi SSID"));
	txt_wifi_ssid = tt_obj_address_txt_create(cont_wifi_only, tr("WiFi SSID"), txt_cb);
	lv_textarea_set_text(txt_wifi_ssid, nw_if->ssid);

	lbl_wifi_pass = tt_obj_label_create(cont_wifi_only, tr("WiFi password"));
	txt_wifi_pass = tt_obj_address_txt_create(cont_wifi_only, tr("WiFi password"), txt_cb);
	lv_textarea_set_text(txt_wifi_pass, nw_if->pass);
	cbx_pass = tt_obj_checkbox_create(cont_wifi_only, tr("Show WiFi password"), cbx_pass_cb);
	lv_textarea_set_password_mode(txt_wifi_pass, true);

	/* LAN + WiFi Ethernet configuration */
	cont_lan_wifi_eth = tt_obj_cont_create(nw_cont2);
	tt_obj_label_create(cont_lan_wifi_eth, tr("Ethernet Configuration"));

	lbl_lw_eth_ip = tt_obj_label_create(cont_lan_wifi_eth, tr("Ethernet IP Address"));
	txt_lw_eth_ip = tt_obj_address_txt_create(cont_lan_wifi_eth,
			tr("Ethernet IP Address"), txt_num_cb);
	lbl_lw_eth_mask = tt_obj_label_create(cont_lan_wifi_eth, tr("Ethernet Subnet Mask"));
	txt_lw_eth_mask = tt_obj_address_txt_create(cont_lan_wifi_eth,
			tr("Ethernet Subnet Mask"), txt_num_cb);
	lbl_lw_eth_gw = tt_obj_label_create(cont_lan_wifi_eth, tr("Ethernet Gateway"));
	txt_lw_eth_gw = tt_obj_address_txt_create(cont_lan_wifi_eth,
			tr("Ethernet Gateway"), txt_num_cb);
	lbl_lw_eth_dns = tt_obj_label_create(cont_lan_wifi_eth, tr("Ethernet DNS"));
	txt_lw_eth_dns = tt_obj_address_txt_create(cont_lan_wifi_eth,
			tr("Ethernet DNS"), txt_num_cb);
	lv_textarea_set_accepted_chars(txt_lw_eth_dns, "0123456789.");

	/* LAN + WiFi WiFi configuration */
	cont_lan_wifi_wifi = tt_obj_cont_create(nw_cont2);
	tt_obj_label_create(cont_lan_wifi_wifi, tr("WiFi Configuration"));

	lbl_lw_wifi_ip = tt_obj_label_create(cont_lan_wifi_wifi, tr("WiFi IP Address"));
	txt_lw_wifi_ip = tt_obj_address_txt_create(cont_lan_wifi_wifi,
			tr("WiFi IP Address"), txt_num_cb);
	lbl_lw_wifi_mask = tt_obj_label_create(cont_lan_wifi_wifi, tr("WiFi Subnet Mask"));
	txt_lw_wifi_mask = tt_obj_address_txt_create(cont_lan_wifi_wifi,
			tr("WiFi Subnet Mask"), txt_num_cb);
	lbl_lw_wifi_gw = tt_obj_label_create(cont_lan_wifi_wifi, tr("WiFi Gateway"));
	txt_lw_wifi_gw = tt_obj_address_txt_create(cont_lan_wifi_wifi,
			tr("WiFi Gateway"), txt_num_cb);
	lbl_lw_wifi_dns = tt_obj_label_create(cont_lan_wifi_wifi, tr("WiFi DNS"));
	txt_lw_wifi_dns = tt_obj_address_txt_create(cont_lan_wifi_wifi,
			tr("WiFi DNS"), txt_num_cb);
	lv_textarea_set_accepted_chars(txt_lw_wifi_dns, "0123456789.");
	tt_obj_label_create(cont_lan_wifi_wifi, tr("WiFi SSID"));
	txt_lw_wifi_ssid = tt_obj_address_txt_create(cont_lan_wifi_wifi,
			tr("WiFi SSID"), txt_cb);
	tt_obj_label_create(cont_lan_wifi_wifi, tr("WiFi password"));
	txt_lw_wifi_pass = tt_obj_address_txt_create(cont_lan_wifi_wifi,
			tr("WiFi password"), txt_cb);
	cbx_lw_wifi_pass = tt_obj_checkbox_create(cont_lan_wifi_wifi,
			tr("Show WiFi password"), cbx_pass_cb);
	lv_textarea_set_password_mode(txt_lw_wifi_pass, true);

	lv_textarea_set_text(txt_lw_eth_ip,
			default_if_empty(default_if_empty(nw_if->lan1_ip, nw_if->ip),
					DEFAULT_LAN1_IP));
	lv_textarea_set_text(txt_lw_eth_mask,
			default_if_empty(nw_if->mask, DEFAULT_SUBNET_MASK));
	lv_textarea_set_text(txt_lw_eth_gw,
			default_if_empty(nw_if->gw, DEFAULT_GATEWAY));
	lv_textarea_set_text(txt_lw_eth_dns,
			sanitize_dns(default_if_empty(nw_if->dns, DEFAULT_DNS)));
	lv_textarea_set_text(txt_lw_wifi_ip,
			default_if_empty(nw_if->wifi_ip, DEFAULT_WIFI_IP));
	lv_textarea_set_text(txt_lw_wifi_mask,
			default_if_empty(nw_if->wifi_mask, DEFAULT_SUBNET_MASK));
	lv_textarea_set_text(txt_lw_wifi_gw,
			default_if_empty(nw_if->wifi_gateway, DEFAULT_GATEWAY));
	lv_textarea_set_text(txt_lw_wifi_dns,
			sanitize_dns(default_if_empty(nw_if->wifi_dns, DEFAULT_DNS)));
	lv_textarea_set_text(txt_lw_wifi_ssid, nw_if->ssid);
	lv_textarea_set_text(txt_lw_wifi_pass, nw_if->pass);

	/* Dual LAN mode container */
	cont_dual_lan = tt_obj_cont_create(nw_cont2);

	/* Dual LAN fields */
	lbl_lan1 = tt_obj_label_create(cont_dual_lan, tr("LAN1 Interface"));
	lbl_lan1_ip = tt_obj_label_create(cont_dual_lan, tr("LAN1 IP Address"));
	txt_lan1_ip = tt_obj_address_txt_create(cont_dual_lan, tr("LAN1 IP Address"), txt_num_cb);

	lbl_lan1_mask = tt_obj_label_create(cont_dual_lan, tr("LAN1 Subnet Mask"));
	txt_lan1_mask = tt_obj_address_txt_create(cont_dual_lan, tr("LAN1 Subnet Mask"), txt_num_cb);
	lbl_lan1_gateway = tt_obj_label_create(cont_dual_lan, tr("LAN1 Gateway"));
	txt_lan1_gateway = tt_obj_address_txt_create(cont_dual_lan, tr("LAN1 Gateway"), txt_num_cb);

	lbl_lan2 = tt_obj_label_create(cont_dual_lan, tr("LAN2 Interface"));
	lbl_lan2_ip = tt_obj_label_create(cont_dual_lan, tr("LAN2 IP Address"));
	txt_lan2_ip = tt_obj_address_txt_create(cont_dual_lan, tr("LAN2 IP Address"), txt_num_cb);

	lbl_lan2_mask = tt_obj_label_create(cont_dual_lan, tr("LAN2 Subnet Mask"));
	txt_lan2_mask = tt_obj_address_txt_create(cont_dual_lan, tr("LAN2 Subnet Mask"), txt_num_cb);
	lbl_lan2_gateway = tt_obj_label_create(cont_dual_lan, tr("LAN2 Gateway"));
	txt_lan2_gateway = tt_obj_address_txt_create(cont_dual_lan, tr("LAN2 Gateway"), txt_num_cb);

	tt_obj_btn_std_create(nw_cont2, btn_nw_settings_cb, tr("Save settings"));

	load_network_form(nw_if);
	if (!nw_if->valid) {
		update_data();
	}
	if (!nw_if->valid && !nw_if_refresh_pending &&
			backend_network_if_refresh(network_if_refresh_cb, NULL) == 0) {
		nw_if_refresh_pending = true;
	}
}
