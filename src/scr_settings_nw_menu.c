#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_settings_nw_menu.h"
#include "scr_settings_nw.h"
#include "scr_settings_nw_snmp.h"
#include "scr_settings_nw_modbus.h"
#include "scr_settings_nw_ssh.h"
#include "scr_settings_nw_blue.h"
#include "scr_settings_nw_ntp_sntp.h"
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

/* Global variables ***********************************************************/
static lv_obj_t* menu;


/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			LV_LOG_USER("Network menu opened");
		}
	}
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void scr_settings_nw_menu_create(lv_obj_t* l_menu, lv_obj_t* btn)
{
	menu = l_menu;

	/* Create the settings page directly */
    lv_obj_t* nw_menu_page = tt_obj_menu_page_create(menu, btn, NULL, "Networks");

	//lv_obj_t* nw_menu_cont = tt_obj_menu_page_create(menu, btn, menu_cb,"Networks");
	//lv_obj_t* nw_options_cont = tt_obj_cont_create(nw_menu_cont);

	/* Apply Flex layout directly to the page. 
       In LVGL, menu pages usually have a 'scrollable' part or are objects themselves 
       that can act as containers.
    */
    lv_obj_set_flex_flow(nw_menu_page, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(
        nw_menu_page,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

	// Create buttons for each network protocol
	lv_obj_t* btn_ethernet = tt_obj_btn_mtx_create(nw_menu_page, NULL, "Ethernet", ASSET("menu.png"));
	
	lv_obj_t* btn_snmp = tt_obj_btn_mtx_create(nw_menu_page, NULL, "SNMP", ASSET("menu.png"));
	
	lv_obj_t* btn_modbus = tt_obj_btn_mtx_create(nw_menu_page, NULL, "Modbus", ASSET("menu.png"));
	
	lv_obj_t* btn_ssh = tt_obj_btn_mtx_create(nw_menu_page, NULL, "SSH", ASSET("menu.png"));
	
	lv_obj_t* btn_bluetooth = tt_obj_btn_mtx_create(nw_menu_page, NULL, "Bluetooth", ASSET("menu.png"));
	
	lv_obj_t* btn_ntp_sntp = tt_obj_btn_mtx_create(nw_menu_page, NULL, "NTP-SNTP", ASSET("menu.png"));

	// Link buttons to their respective screen pages
	scr_settings_nw_create(menu, btn_ethernet);
	scr_settings_nw_snmp_create(menu, btn_snmp);
	scr_settings_nw_modbus_create(menu, btn_modbus);
	scr_settings_nw_ssh_create(menu, btn_ssh);
	scr_settings_nw_blue_create(menu, btn_bluetooth);
	scr_settings_nw_ntp_sntp_create(menu, btn_ntp_sntp);
}
