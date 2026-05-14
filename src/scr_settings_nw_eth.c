#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "scr_settings_nw_eth.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"
#include "config.h"
#include "screen.h"
#include "ttne_display.h"
#include "runbg.h"

#define TIMER_NW_CHECK_PERIOD 2000 // ms
#define TIMER_MSG_BOX_PERIOD 10000 // ms
#define MAX_NW_CONN_RETRIES 5

typedef enum {
    NW_SINGLE_LAN  = 0,  // Single ethernet (eth0 or eth1)
    NW_WIFI_ONLY   = 1,  // WiFi only
    NW_DUAL_LAN    = 2,  // Both eth0 and eth1
    NW_LAN_WIFI    = 3,  // Ethernet + WiFi
    NW_ETH         = NW_SINGLE_LAN,
    NW_WIFI        = NW_WIFI_ONLY,
} dd_opts_t;

/* Global variables ***********************************************************/

static models_nw_if_t nw_ifaces;

static lv_obj_t* loader_scr;

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
static bool is_ethernet();
static bool is_static();
static void save_nw_if_async(const models_nw_if_t* nw_if);
static models_nw_if_t clone_nw_if(const models_nw_if_t* nw_if);
static void free_nw_if(models_nw_if_t* nw_if);
static uint16_t determine_network_mode(const models_nw_if_t* nw_if);

/* Callbacks ******************************************************************/

/**
 * Determine the network mode from saved network interface configuration
 * by examining the multi-interface IP fields
 */
static uint16_t determine_network_mode(const models_nw_if_t* nw_if)
{
	bool has_lan1 = (nw_if->lan1_ip != NULL && strlen(nw_if->lan1_ip) > 0);
	bool has_lan2 = (nw_if->lan2_ip != NULL && strlen(nw_if->lan2_ip) > 0);
	bool has_wifi = (nw_if->wifi_ip != NULL && strlen(nw_if->wifi_ip) > 0);
	
	/* Determine mode based on multi-interface IP configuration */
	if (has_wifi && has_lan1 && has_lan2) {
		return NW_LAN_WIFI;  /* Both LAN interfaces and WiFi */
	} else if (has_lan1 && has_lan2) {
		return NW_DUAL_LAN;  /* Both LAN interfaces */
	} else if (has_wifi && !has_lan1 && !has_lan2) {
		return NW_WIFI_ONLY; /* WiFi only */
	} else {
		return NW_SINGLE_LAN; /* Single LAN or unknown */
	}
}

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
			
			/* Determine the correct network mode from saved configuration */
			uint16_t saved_mode = determine_network_mode(nw_if);
			lv_dropdown_set_selected(dd, saved_mode);
			
			(void)is_static(); // Not used
			if (nw_if->dhcp) {
				lv_obj_add_state(btn_dhcp, LV_STATE_CHECKED);
			} else {
				lv_obj_clear_state(btn_dhcp, LV_STATE_CHECKED);
			}
			
			/* Load IP configurations based on saved mode */
			switch (saved_mode) {
				case NW_SINGLE_LAN:
					/* For single LAN, restore IP from lan1_ip if available, else from params.ip */
					if (nw_if->lan1_ip != NULL && strlen(nw_if->lan1_ip) > 0) {
						lv_textarea_set_text(txt_ip, nw_if->lan1_ip);
					} else {
						lv_textarea_set_text(txt_ip, nw_if->params.ip);
					}
					break;
				
				case NW_DUAL_LAN:
					/* For dual LAN, restore from lan1_ip and lan2_ip */
					if (nw_if->lan1_ip != NULL) {
						lv_textarea_set_text(txt_lan1_ip, nw_if->lan1_ip);
					}
					if (nw_if->lan2_ip != NULL) {
						lv_textarea_set_text(txt_lan2_ip, nw_if->lan2_ip);
					}
					break;
				
				case NW_LAN_WIFI:
					/* For LAN+WiFi, restore LAN IP from lan1_ip */
					if (nw_if->lan1_ip != NULL && strlen(nw_if->lan1_ip) > 0) {
						lv_textarea_set_text(txt_lan1_ip, nw_if->lan1_ip);
					}
					break;
				
				default:
					lv_textarea_set_text(txt_ip, nw_if->params.ip);
					break;
			}
			
			/* Always load common fields */
			lv_textarea_set_text(txt_mask, nw_if->params.mask);
			lv_textarea_set_text(txt_gw, nw_if->params.gw);
			lv_textarea_set_text(txt_dns, nw_if->params.dns);
			lv_textarea_set_text(txt_wifi_ssid, nw_if->params.ssid);
			lv_textarea_set_text(txt_wifi_pass, nw_if->params.pass);
			
			/* Populate dual LAN masks from saved configuration */
			if (saved_mode == NW_DUAL_LAN || saved_mode == NW_LAN_WIFI) {
				if (!nw_if->dhcp && strlen(nw_if->params.mask) > 0) {
					lv_textarea_set_text(txt_lan1_mask, nw_if->params.mask);
					lv_textarea_set_text(txt_lan2_mask, nw_if->params.mask);
				}
			}
			
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
	lv_obj_t* scr = timer->user_data;
	lv_obj_t* msg_box_conn;

	// For static IP configuration, return immediately without checking internet
	if (nw_ifaces.type == ETH_STATIC || nw_ifaces.type == WIFI_STATIC) {
		retries = 0;
		lv_timer_del(timer);
		lv_scr_load(scr);
		lv_obj_del(loader_scr);
		msg_box_conn = tt_obj_info_box_create("INFO", "Configuration Applied", 0);
		lv_timer_create(msg_box_timer_cb, TIMER_MSG_BOX_PERIOD, msg_box_conn);
		return;
	}

	// For DHCP, check internet connectivity
	// Call API network info
	controller_get_nw_info();
	const models_nw_info_t* nw_info = models_get_nw_info();
	bool connected = (bool)nw_info->connected;
	retries++;
	LV_LOG_USER("Connected: %s (%d/%d)", connected ? "yes" : "no",
			retries, MAX_NW_CONN_RETRIES);

	if (connected) {
		retries = 0;
		lv_timer_del(timer);
		lv_scr_load(scr);
		lv_obj_del(loader_scr);
		msg_box_conn = tt_obj_info_box_create("INFO", "Connected to Internet", 0);
		lv_timer_create(msg_box_timer_cb, TIMER_MSG_BOX_PERIOD, msg_box_conn);
		return;

	}
	if (retries > MAX_NW_CONN_RETRIES) {
		retries = 0;
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

static models_nw_if_t clone_nw_if(const models_nw_if_t* nw_if)
{
	models_nw_if_t copy = *nw_if;
	copy.params.ip = strdup(nw_if->params.ip);
	copy.params.mask = strdup(nw_if->params.mask);
	copy.params.gw = strdup(nw_if->params.gw);
	copy.params.dns = strdup(nw_if->params.dns);
	copy.params.ssid = strdup(nw_if->params.ssid);
	copy.params.pass = strdup(nw_if->params.pass);
	copy.eth_interface = strdup(nw_if->eth_interface != NULL ? nw_if->eth_interface : "");
	return copy;
}

static void free_nw_if(models_nw_if_t* nw_if)
{
	free((void*)nw_if->params.ip);
	free((void*)nw_if->params.mask);
	free((void*)nw_if->params.gw);
	free((void*)nw_if->params.dns);
	free((void*)nw_if->params.ssid);
	free((void*)nw_if->params.pass);
	free((void*)nw_if->eth_interface);
}

static void save_nw_if_async(const models_nw_if_t* nw_if)
{
	models_nw_if_t copy = clone_nw_if(nw_if);
	pid_t pid = fork();

	if (pid == 0) {
		pid_t worker_pid = fork();
		if (worker_pid == 0) {
			controller_put_nw_if(&copy);
			free_nw_if(&copy);
			_exit(0);
		}
		_exit(0);
	}

	if (pid > 0) {
		waitpid(pid, NULL, 0);
	} else {
		controller_put_nw_if(nw_if);
	}

	free_nw_if(&copy);
}

static void msg_box_nw_if_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			lv_obj_t* prev_scr = lv_scr_act();
			save_nw_if_async(&nw_ifaces);
			lv_timer_create(nw_if_timer_cb, TIMER_NW_CHECK_PERIOD, prev_scr);
			char* txt = lv_event_get_user_data(e);
			loader_scr = tt_obj_loader_create(txt, NULL);
			lv_obj_add_event_cb(loader_scr, loader_cb, LV_EVENT_ALL, prev_scr);
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
		const models_nw_if_t* current_nw_if = models_get_nw_if();
		nw_ifaces.eth_interface = current_nw_if->eth_interface;
		uint16_t selected_mode = lv_dropdown_get_selected(dd);
		
		/* Determine network type based on selected mode */
		switch (selected_mode) {
			case NW_SINGLE_LAN:
				nw_type = "Single LAN";
				if (dhcp) {
					nw_ifaces.type = ETH_DHCP;
				} else {
					nw_ifaces.type = ETH_STATIC;
				}
				break;
			case NW_WIFI_ONLY:
				nw_type = "WiFi Only";
				if (dhcp) {
					nw_ifaces.type = WIFI_DHCP;
				} else {
					nw_ifaces.type = WIFI_STATIC;
				}
				break;
			case NW_DUAL_LAN:
				nw_type = "Dual LAN";
				if (dhcp) {
					nw_ifaces.type = ETH_DHCP;
				} else {
					nw_ifaces.type = ETH_STATIC;
				}
				break;
			case NW_LAN_WIFI:
				nw_type = "LAN + WiFi";
				if (dhcp) {
					nw_ifaces.type = ETH_DHCP;
				} else {
					nw_ifaces.type = ETH_STATIC;
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
		nw_ifaces.params.ip = lv_textarea_get_text(txt_ip);
		nw_ifaces.params.mask = lv_textarea_get_text(txt_mask);
		nw_ifaces.params.gw = lv_textarea_get_text(txt_gw);
		nw_ifaces.params.dns = lv_textarea_get_text(txt_dns);
		nw_ifaces.params.ssid = lv_textarea_get_text(txt_wifi_ssid);
		nw_ifaces.params.pass = lv_textarea_get_text(txt_wifi_pass);
		if (nw_ifaces.params.pass == NULL) {
			nw_ifaces.params.pass = "";
		}
		
		/* Populate multi-interface IP fields based on selected mode */
		switch (selected_mode) {
			case NW_SINGLE_LAN:
				nw_ifaces.lan1_ip = lv_textarea_get_text(txt_ip);
				nw_ifaces.lan2_ip = "";
				nw_ifaces.wifi_ip = "";
				break;
			case NW_WIFI_ONLY:
				nw_ifaces.lan1_ip = "";
				nw_ifaces.lan2_ip = "";
				nw_ifaces.wifi_ip = lv_textarea_get_text(txt_wifi_ssid); /* Will use wifi params */
				break;
			case NW_DUAL_LAN:
				nw_ifaces.lan1_ip = lv_textarea_get_text(txt_lan1_ip);
				nw_ifaces.lan2_ip = lv_textarea_get_text(txt_lan2_ip);
				nw_ifaces.wifi_ip = "";
				break;
			case NW_LAN_WIFI:
				nw_ifaces.lan1_ip = lv_textarea_get_text(txt_lan1_ip);
				nw_ifaces.lan2_ip = "";
				nw_ifaces.wifi_ip = lv_textarea_get_text(txt_wifi_ssid); /* Will use wifi params */
				break;
			default:
				nw_ifaces.lan1_ip = "";
				nw_ifaces.lan2_ip = "";
				nw_ifaces.wifi_ip = "";
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
					nw_ifaces.params.ssid);
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
					nw_ifaces.params.ip,
					nw_ifaces.params.mask,
					nw_ifaces.params.gw,
					nw_ifaces.params.dns);
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
	const models_nw_if_t* nw_if = models_get_nw_if();

	// Settings / Network
	lv_obj_t* nw_cont = tt_obj_menu_page_create(menu, btn, menu_cb, "Network Setup");
	lv_obj_t* nw_cont2 = tt_obj_cont_create(nw_cont);

	tt_obj_label_create(nw_cont2, "Connection type");
	char* options = "Single LAN\nWiFi Only\nDual LAN\nLAN + WiFi";
	dd = tt_obj_dropdown_create(nw_cont2, options, update_cb);
	btn_dhcp = tt_obj_btn_toggle_create(nw_cont2, update_cb, "DHCP");
	lv_obj_add_style(btn_dhcp, &btn_style, 0);
	lv_obj_set_height(btn_dhcp, 36);

	/* Single LAN mode container */
	cont_single_lan = tt_obj_cont_create(nw_cont2);

	/* Single LAN fields */
	lbl_ip = tt_obj_label_create(cont_single_lan, "IP Address");
	txt_ip = tt_obj_txt_create(cont_single_lan, "IP Address", txt_num_cb);
	lv_textarea_set_text(txt_ip, nw_if->params.ip);

	lbl_mask = tt_obj_label_create(cont_single_lan, "Subnet Mask");
	txt_mask = tt_obj_txt_create(cont_single_lan, "Subnet Mask", txt_num_cb);
	lv_textarea_set_text(txt_mask, nw_if->params.mask);

	lbl_gw = tt_obj_label_create(cont_single_lan, "Gateway IP");
	txt_gw = tt_obj_txt_create(cont_single_lan, "Gateway IP", txt_num_cb);
	lv_textarea_set_text(txt_gw, nw_if->params.gw);

	lbl_dns = tt_obj_label_create(cont_single_lan, "DNS");
	txt_dns = tt_obj_txt_create(cont_single_lan, "DNS", txt_num_cb);
	lv_textarea_set_text(txt_dns, nw_if->params.dns);

	/* WiFi only mode container */
	cont_wifi_only = tt_obj_cont_create(nw_cont2);

	/* WiFi only fields */
	lbl_wifi_ssid = tt_obj_label_create(cont_wifi_only, "WiFi SSID");
	txt_wifi_ssid = tt_obj_txt_create(cont_wifi_only, "WiFi SSID", txt_cb);
	lv_textarea_set_text(txt_wifi_ssid, nw_if->params.ssid);

	lbl_wifi_pass = tt_obj_label_create(cont_wifi_only, "WiFi password");
	txt_wifi_pass = tt_obj_txt_create(cont_wifi_only, "WiFi password", txt_cb);
	lv_textarea_set_text(txt_wifi_pass, nw_if->params.pass);
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
