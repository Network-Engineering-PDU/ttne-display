#include "lvgl/lvgl.h"

#include "scr_alarms.h"
#include "alarms.h"
#include "tt_obj.h"


/* Global variables ***********************************************************/

static alarms_t alarms;

/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void alarm_cont_close_cb(lv_event_t* e);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(obj);
		if (curr_page == page) {
			LV_LOG_USER("Alarms cb");
		}
	}
}

static void alarm_cont_close_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* cont = lv_event_get_user_data(e);
		lv_obj_del(cont);
	}
}

/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void scr_alarms_create(lv_obj_t* menu, lv_obj_t* btn)
{
	alarms_init(&alarms);

	lv_obj_t* alarms_cont = tt_obj_menu_page_create(menu, btn, menu_cb, "Alarms");

	// lv_obj_t* alarms_settings_cont = tt_obj_cont_create(alarms_cont);
	// tt_obj_label_create(alarms_settings_cont, "Alarms settings");
	// tt_obj_btn_toggle_create(alarms_settings_cont, NULL, "BUZZER");
	// tt_obj_btn_toggle_create(alarms_settings_cont, NULL, "RELAY");
	// tt_obj_btn_toggle_create(alarms_settings_cont, NULL, "EMAIL");
	// tt_obj_btn_toggle_create(alarms_settings_cont, NULL, "SNMP TRAP");

	lv_obj_t* alarms_alarms_cont = tt_obj_cont_create(alarms_cont);
	tt_obj_label_create(alarms_alarms_cont, "Alarms");

	// tt_obj_label_create(alarms_alarms_cont, "Alarm demo: work in progress...");

	static alarm_desc_t alarm;
	alarm.time = "Now";
	alarm.type = ALARM_INFO;
	alarm.desc = "No alarm has been triggered";
	alarm.path = "/PDU/Info";
	alarm.code = "#000";
	alarms_new(&alarms, &alarm);

	// alarm.time = "2023/02/15 16:20:00";
	// alarm.type = ALARM_ERROR;
	// alarm.desc = "Underflow";
	// alarm.path = "/PDU/Phase current Power";
	// alarm.code = "#301";
	// alarms_new(&alarms, &alarm);

	// alarm.time = "2023/02/12 02:03:43";
	// alarm.type = ALARM_WARNING;
	// alarm.desc = "Underflow";
	// alarm.path = "/PDU/Phase current Powrop Power";
	// alarm.code = "#308ewr98tn9cv8908wer098twe908";
	// alarms_new(&alarms, &alarm);

	// alarm.time = "2023/02/13 17:03:39";
	// alarm.type = ALARM_CRITICAL_ERROR;
	// alarm.desc = "Temperature";
	// alarm.path = "/PDU/Phase current Powrop Power";
	// alarm.code = "#309";
	// alarms_new(&alarms, &alarm);

	// alarm.time = "2023/01/14 16:11:42";
	// alarm.type = ALARM_INFO;
	// alarm.desc = "Pressure";
	// alarm.path = "/PDU/Phase current Power Powrop";
	// alarm.code = "#350";
	// alarms_new(&alarms, &alarm);

	for (int i = 0; i < alarms.n; i++) {
		tt_obj_cont_alarm_create(alarms_alarms_cont, alarm_cont_close_cb,
				&alarms.alarms[i]);
	}
}
