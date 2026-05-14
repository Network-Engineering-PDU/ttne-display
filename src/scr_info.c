#include <stdio.h>

#include "lvgl/lvgl.h"
#include "scr_info.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"

/* Global variables ***********************************************************/

static lv_obj_t* lbl_name;
static lv_obj_t* lbl_pn;
static lv_obj_t* lbl_sn;
static lv_obj_t* lbl_mac;
static lv_obj_t* lbl_ip;
static lv_obj_t* lbl_lan1_ip;
static lv_obj_t* lbl_lan2_ip;
static lv_obj_t* lbl_wifi_ip;
static lv_obj_t* lbl_outlets;
static lv_obj_t* lbl_rated_curr;
static lv_obj_t* lbl_controller;
static lv_obj_t* lbl_type;
static lv_obj_t* lbl_version;
static lv_obj_t* lbl_om_version;
static lv_obj_t* lbl_pmb_version;
static lv_obj_t* lbl_display_version;
static lv_obj_t* lbl_uptime;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			LV_LOG_USER("Info cb");
			controller_get_sys_info();
			controller_get_pdu_info();
			controller_get_nw_if();
			char str[100];
			const models_info_t* info = models_get_info();
			const models_pdu_info_t* pdu_info = models_get_pdu_info();
			const models_nw_if_t* nw_if = models_get_nw_if();

			sprintf(str, "  %s: #%06X %s", "Name", TT_COLOR_GREEN_NE, info->product_name);
			lv_label_set_text(lbl_name, str);
			sprintf(str, "  %s: #%06X %s", "PN", TT_COLOR_GREEN_NE, info->product_pn);
			lv_label_set_text(lbl_pn, str);
			sprintf(str, "  %s: #%06X %s", "SN", TT_COLOR_GREEN_NE, info->product_sn);
			lv_label_set_text(lbl_sn, str);
			sprintf(str, "  %s: #%06X %s", "LAN MAC", TT_COLOR_GREEN_NE, info->lan_mac);
			lv_label_set_text(lbl_mac, str);

			/* Display IP(s) based on network mode */
			lv_obj_add_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(lbl_lan1_ip, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(lbl_lan2_ip, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(lbl_wifi_ip, LV_OBJ_FLAG_HIDDEN);

			if (nw_if->type == ETH_DHCP || nw_if->type == ETH_STATIC) {
				/* Single LAN mode */
				sprintf(str, "  %s: #%06X %s", "IP", TT_COLOR_GREEN_NE, nw_if->params.ip);
				lv_label_set_text(lbl_ip, str);
				lv_obj_clear_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);
			} else if (nw_if->type == WIFI_DHCP || nw_if->type == WIFI_STATIC) {
				/* WiFi Only mode */
				sprintf(str, "  %s: #%06X %s", "WiFi IP", TT_COLOR_GREEN_NE, nw_if->params.ip);
				lv_label_set_text(lbl_ip, str);
				lv_obj_clear_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);
			}

			/* Dual LAN: display both LAN IPs */
			if (nw_if->lan1_ip != NULL && strlen(nw_if->lan1_ip) > 0) {
				sprintf(str, "  %s: #%06X %s", "LAN1 IP", TT_COLOR_GREEN_NE, nw_if->lan1_ip);
				lv_label_set_text(lbl_lan1_ip, str);
				lv_obj_clear_flag(lbl_lan1_ip, LV_OBJ_FLAG_HIDDEN);
			}
			if (nw_if->lan2_ip != NULL && strlen(nw_if->lan2_ip) > 0) {
				sprintf(str, "  %s: #%06X %s", "LAN2 IP", TT_COLOR_GREEN_NE, nw_if->lan2_ip);
				lv_label_set_text(lbl_lan2_ip, str);
				lv_obj_clear_flag(lbl_lan2_ip, LV_OBJ_FLAG_HIDDEN);
			}
			/* LAN + WiFi: display WiFi IP */
			if (nw_if->wifi_ip != NULL && strlen(nw_if->wifi_ip) > 0) {
				sprintf(str, "  %s: #%06X %s", "WiFi IP", TT_COLOR_GREEN_NE, nw_if->wifi_ip);
				lv_label_set_text(lbl_wifi_ip, str);
				lv_obj_clear_flag(lbl_wifi_ip, LV_OBJ_FLAG_HIDDEN);
			}

			sprintf(str, "  %s: #%06X %d", "Outlets", TT_COLOR_GREEN_NE, pdu_info->n_outlets);
			lv_label_set_text(lbl_outlets, str);
			sprintf(str, "  %s: #%06X %d", "Rated current", TT_COLOR_GREEN_NE, pdu_info->rated_current);
			lv_label_set_text(lbl_rated_curr, str);
			sprintf(str, "  %s: #%06X %s", "Controller", TT_COLOR_GREEN_NE, pdu_info->controller);
			lv_label_set_text(lbl_controller, str);
			sprintf(str, "  %s: #%06X %s", "Type", TT_COLOR_GREEN_NE, pdu_info->type);
			lv_label_set_text(lbl_type, str);

			sprintf(str, "  %s: #%06X %s", "SW version", TT_COLOR_GREEN_NE, info->sw_version);
			lv_label_set_text(lbl_version, str);
			sprintf(str, "  %s: #%06X %s", "OM version", TT_COLOR_GREEN_NE, info->om_version);
			lv_label_set_text(lbl_om_version, str);
			sprintf(str, "  %s: #%06X %s", "PMB version", TT_COLOR_GREEN_NE, info->pmb_version);
			lv_label_set_text(lbl_pmb_version, str);
			sprintf(str, "  %s: #%06X %s", "Display version", TT_COLOR_GREEN_NE, GIT_VERSION);
			lv_label_set_text(lbl_display_version, str);
			sprintf(str, "  %s: #%06X %s", "Uptime (HH:MM)", TT_COLOR_GREEN_NE, info->uptime);
			lv_label_set_text(lbl_uptime, str);
		}
	}
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void scr_info_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* info_cont = tt_obj_menu_page_create(menu, btn, menu_cb, "Info");
	lv_obj_t* info_product_cont = tt_obj_cont_create(info_cont);
	tt_obj_label_color_create(info_product_cont, "Information");

	lbl_name = tt_obj_label_color_create(info_product_cont, "");
	lbl_pn = tt_obj_label_color_create(info_product_cont, "");
	lbl_sn = tt_obj_label_color_create(info_product_cont, "");
	lbl_mac = tt_obj_label_color_create(info_product_cont, "");
	lbl_ip = tt_obj_label_color_create(info_product_cont, "");
	lbl_lan1_ip = tt_obj_label_color_create(info_product_cont, "");
	lbl_lan2_ip = tt_obj_label_color_create(info_product_cont, "");
	lbl_wifi_ip = tt_obj_label_color_create(info_product_cont, "");
	lbl_outlets = tt_obj_label_color_create(info_product_cont, "");
	lbl_rated_curr = tt_obj_label_color_create(info_product_cont, "");
	lbl_controller = tt_obj_label_color_create(info_product_cont, "");
	lbl_type = tt_obj_label_color_create(info_product_cont, "");
	lbl_version = tt_obj_label_color_create(info_product_cont, "");
	lbl_om_version = tt_obj_label_color_create(info_product_cont, "");
	lbl_pmb_version = tt_obj_label_color_create(info_product_cont, "");
	lbl_display_version = tt_obj_label_color_create(info_product_cont, "");
	lbl_uptime = tt_obj_label_color_create(info_product_cont, "");

	/* Initialize IP labels as hidden */
	lv_obj_add_flag(lbl_ip, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(lbl_lan1_ip, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(lbl_lan2_ip, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(lbl_wifi_ip, LV_OBJ_FLAG_HIDDEN);
}
