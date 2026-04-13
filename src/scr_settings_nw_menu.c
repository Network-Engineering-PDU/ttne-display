#include <stdio.h>

#include "lvgl/lvgl.h"

#include "scr_settings_nw_menu.h"
#include "scr_settings_nw.h"
//#include "scr_settings_nw_eth.h"
//#include "scr_settings_nw_snmp.h"
//#include "scr_settings_nw_modbus.h"
//#include "scr_settings_nw_ssh.h"
//#include "scr_settings_nw_blue.h"
//#include "scr_settings_nw_ntp_sntp.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"

/* Global variables ***********************************************************/
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
			LV_LOG_USER("Network menu opened");
		}
	}
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void scr_settings_nw_menu_create(lv_obj_t* menu, lv_obj_t* btn)
{

}
