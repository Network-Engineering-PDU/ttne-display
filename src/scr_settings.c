#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include "lvgl/lvgl.h"

#include "scr_settings.h"
#include "scr_settings_nw.h"
#include "scr_diagnose.h"
#include "scr_keyboard.h"
#include "scr_support.h"
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

#define TIMER_ROT 5000 // ms
#define TIMER_CHECK_UPDATE 1000 // ms

#ifdef SIMULATOR_ENABLED
#define MOUNT_DIR "/home/guille"
#else
#define MOUNT_DIR "/run/mount"
#endif

extern void reset_program();

/* Global variables ***********************************************************/

static lv_obj_t* menu;
static lv_obj_t* diagnose_page;
static lv_obj_t* support_page;

static lv_obj_t* txt_splash;
static lv_obj_t* txt_modbus_addr;

static lv_obj_t* btn_snmp;
static lv_obj_t* btn_modbus;
static lv_obj_t* btn_ssh;

static lv_obj_t* dd;
static lv_obj_t* msg_box_rot;
static lv_timer_t* timer_rot;
static lv_timer_t* timer_check_update;

static bool running = false;
static pid_t update_pid;
static char update_dev[20];


/* Function prototypes ********************************************************/

static void menu_cb(lv_event_t* e);
static void rotate_cb(lv_event_t* e);
static void loader_cb(lv_event_t* e);
static void timer_rot_cb(lv_timer_t* timer);
static void timer_check_update_cb(lv_timer_t* timer);

static void btn_nw_reset_cb(lv_event_t* e);
static void btn_ssh_cb(lv_event_t* e);
static void btn_snmp_cb(lv_event_t* e);
static void btn_modbus_cb(lv_event_t* e);
static void btn_update_cb(lv_event_t* e);
static void btn_fact_reset_cb(lv_event_t* e);
static void btn_reboot_cb(lv_event_t* e);
static void btn_diag_cb(lv_event_t* e);
static void btn_support_cb(lv_event_t* e);

static void msg_box_rot_cb(lv_event_t* e);
static void msg_box_nw_reset_cb(lv_event_t* e);
static void msg_box_ssh_cb(lv_event_t* e);
static void msg_box_snmp_cb(lv_event_t* e);
static void msg_box_modbus_cb(lv_event_t* e);
static void msg_box_fact_reset_cb(lv_event_t* e);
static void msg_box_reboot_cb(lv_event_t* e);
static void msg_box_update_cb(lv_event_t* e);

static void txt_inactivity_cb(lv_event_t* e);
static void txt_modbus_addr_cb(lv_event_t* e);

static void revert_rot();
static char* get_last_element(const char* str);

/* Callbacks ******************************************************************/

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* curr_page = lv_event_get_user_data(e);
		lv_obj_t* page = lv_menu_get_cur_main_page(menu);
		if (curr_page == page) {
			controller_get_nw_services();
			const models_nw_services_t* nw_services = models_get_nw_services();
			tt_obj_btn_toggle_set_state(btn_snmp, nw_services->snmp);
			tt_obj_btn_toggle_set_state(btn_modbus, nw_services->modbus);
			tt_obj_btn_toggle_set_state(btn_ssh, nw_services->ssh);
			controller_get_modbus();
			const models_modbus_t* modbus = models_get_modbus();
			char modbus_addr_str[10];
			sprintf(modbus_addr_str, "%d", modbus->addr);
			lv_textarea_set_text(txt_modbus_addr, modbus_addr_str);
			if (!running) {
				running = true;
				LV_LOG_USER("Settings open");
			}
		}
	} else if (code == LV_EVENT_CLICKED) {
		if (running) {
			running = false;
			LV_LOG_USER("Settings close");
		}
	}
}

static void rotate_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	uint16_t rotation = 0;

	if (code == LV_EVENT_VALUE_CHANGED) {
		rotation = lv_dropdown_get_selected(dd);
		screen_set_rotation(rotation);
		timer_rot = lv_timer_create(timer_rot_cb, TIMER_ROT, NULL);
		char rot_str[5];
		lv_dropdown_get_selected_str(dd, rot_str, 0);
		int rot_int = atoi(rot_str);
		char msg[300];
		sprintf(msg, "Are you sure you want to save screen rotation?\nRotation: "
				TT_COLOR_GREEN_NE_STR
				" %d deg#\n(changes will be reverted in 5 seconds)",
				rot_int);
		msg_box_rot = tt_obj_msg_box_create("Screen rotation", msg, NULL,
				msg_box_rot_cb);
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

static void timer_rot_cb(lv_timer_t* timer)
{
	lv_msgbox_close(msg_box_rot);
	lv_timer_del(timer);
	revert_rot();
}

static void timer_check_update_cb(lv_timer_t* timer)
{
	int running;
	int ret_code = runbg_check(update_pid, &running);
	if (ret_code != 0) {
		//TODO: handle
		LV_LOG_USER("Update ERROR!");
	}
	if (running) {
		return;
	} else {
		lv_timer_del(timer);
		pid_t pid = runbg_run("/usr/bin/usb_autorun.sh", "remove", update_dev,
				NULL);
		runbg_check_wait(pid);
	}
}

static void btn_nw_reset_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		tt_obj_msg_box_create("Network reset",
				"Are you sure you want to reset the network configuration?",
				"Resetting network cofiguration...", msg_box_nw_reset_cb);
	}
}

static void btn_ssh_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* btn = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		char msg[100];
		sprintf(msg, "Are you sure you want to " TT_COLOR_GREEN_NE_STR
				" %s# SSH?", (lv_obj_get_state(btn) & LV_STATE_CHECKED) ?
				"enable" : "disable");
		tt_obj_msg_box_create("SSH service", msg, NULL, msg_box_ssh_cb);
	}
}

static void btn_snmp_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* btn = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		char msg[100];
		sprintf(msg, "Are you sure you want to " TT_COLOR_GREEN_NE_STR
				" %s# SNMP?", (lv_obj_get_state(btn) & LV_STATE_CHECKED) ?
				"enable" : "disable");
		tt_obj_msg_box_create("SNMP service", msg, NULL, msg_box_snmp_cb);
	}
}

static void btn_modbus_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* btn = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		char msg[100];
		sprintf(msg, "Are you sure you want to " TT_COLOR_GREEN_NE_STR
				" %s# Modbus?", (lv_obj_get_state(btn) & LV_STATE_CHECKED) ?
				"enable" : "disable");
		tt_obj_msg_box_create("Modbus service", msg, NULL, msg_box_modbus_cb);
	}
}


static void btn_update_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		FILE* file = fopen("/proc/mounts", "r");
		if (file == NULL) {
			LV_LOG_ERROR("Error opening file for reading.");
			return;
		}
		char line[500];
		char dev[50];
		char path[50];
		int n_devices = 0;
 
		while (fgets(line, 500, file) != NULL) {
			sscanf(line, "%s %s %*s %*s %*s %*s", dev, path);
			if (strstr(path, MOUNT_DIR) != NULL) {
				n_devices++;
				char* dev_name = get_last_element(path);
				char msg[200];
				sprintf(msg, "USB device " TT_COLOR_GREEN_NE_STR
						" %s# detected. Do you want to update the device?",
						dev_name);
				strcpy(update_dev, get_last_element(dev));
				tt_obj_msg_box_create("Device update", msg,
						"Updating device...", msg_box_update_cb);
				break;
				// ./usb_autorun.sh add {dev+4} (thread)
				// wait for thread finish (create_timer every 1s)
			}
		}
		if (n_devices == 0) {
			tt_obj_info_box_create("Device update", "No USB devices detected", 1);
		}
		fclose(file);
	}
}

static void btn_fact_reset_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		tt_obj_msg_box_create("Factory reset",
				"Are you sure you want to factory reset the system?",
				"Performing factory reset...", msg_box_fact_reset_cb);
	}
}

static void btn_reboot_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		tt_obj_msg_box_create("Reboot system",
				"Are you sure you want to reboot the system?",
				"Rebooting system...", msg_box_reboot_cb);
	}
}

static void btn_diag_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);


	if (code == LV_EVENT_CLICKED) {
		lv_menu_set_page(menu, diagnose_page);
	}
}

static void btn_support_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);


	if (code == LV_EVENT_CLICKED) {
		lv_menu_set_page(menu, support_page);
	}
}

static void msg_box_rot_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_timer_del(timer_rot);
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			config_set_rotation(lv_dropdown_get_selected(dd));
			reset_program();
		} else {
			revert_rot();
		}
		lv_msgbox_close(obj);
	}
}

// TODO: unufy this  callbacks?
static void msg_box_nw_reset_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			char* txt = lv_event_get_user_data(e);
			lv_obj_t* loader_scr = tt_obj_loader_create(txt, NULL);
			lv_obj_add_event_cb(loader_scr, loader_cb,
					LV_EVENT_ALL, lv_scr_act());
			lv_obj_t* scr = lv_scr_act();
			lv_scr_load(loader_scr);
			controller_post_nw_reset();
			lv_scr_load(scr);
			lv_obj_del(loader_scr);

		}
		lv_msgbox_close(obj);
	}
}

static void msg_box_ssh_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			if (lv_obj_get_state(btn_ssh) & LV_STATE_CHECKED) {
				controller_post_start_ssh();
			} else {
				controller_post_stop_ssh();
			}
		} else {
			const models_nw_services_t* nw_services = models_get_nw_services();
			tt_obj_btn_toggle_set_state(btn_ssh, nw_services->ssh);
		}
		lv_msgbox_close(obj);
	}
}

static void msg_box_snmp_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			if (lv_obj_get_state(btn_snmp) & LV_STATE_CHECKED) {
				controller_post_start_snmp();
			} else {
				controller_post_stop_snmp();
			}
		} else {
			const models_nw_services_t* nw_services = models_get_nw_services();
			tt_obj_btn_toggle_set_state(btn_snmp, nw_services->snmp);
		}
		lv_msgbox_close(obj);
	}
}

static void msg_box_modbus_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			if (lv_obj_get_state(btn_modbus) & LV_STATE_CHECKED) {
				controller_post_start_modbus();
			} else {
				controller_post_stop_modbus();
			}
		} else {
			const models_nw_services_t* nw_services = models_get_nw_services();
			tt_obj_btn_toggle_set_state(btn_modbus, nw_services->modbus);
		}
		lv_msgbox_close(obj);
	}
}

static void msg_box_fact_reset_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			char* txt = lv_event_get_user_data(e);
			lv_obj_t* loader_scr = tt_obj_loader_create(txt, NULL);
			lv_obj_add_event_cb(loader_scr, loader_cb,
					LV_EVENT_ALL, lv_scr_act());
			lv_scr_load(loader_scr);
			controller_post_fact_reset();
		}
		lv_msgbox_close(obj);
	}
}

static void msg_box_reboot_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			char* txt = lv_event_get_user_data(e);
			lv_obj_t* loader_scr = tt_obj_loader_create(txt, NULL);
			lv_obj_add_event_cb(loader_scr, loader_cb,
					LV_EVENT_ALL, lv_scr_act());
			lv_scr_load(loader_scr);
			controller_post_reboot();
		}
		lv_msgbox_close(obj);
	}
}

static void msg_box_update_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		if (lv_msgbox_get_active_btn(obj) == 0) { // YES
			update_pid = runbg_run("/usr/bin/usb_autorun.sh", "add", update_dev,
					NULL);
			timer_check_update = lv_timer_create(timer_check_update_cb,
					TIMER_CHECK_UPDATE, NULL);
			char* txt = lv_event_get_user_data(e);
			if (txt != NULL) {
				lv_obj_t* loader_scr = tt_obj_loader_create(txt, NULL);
				lv_obj_add_event_cb(loader_scr, loader_cb,
						LV_EVENT_ALL, lv_scr_act());
				lv_scr_load(loader_scr);
			}
		}
		lv_msgbox_close(obj);
	}
}

static void txt_inactivity_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_NUM);
		lv_scr_load(kb_scr);
	}
	if (code == LV_EVENT_READY) {
		// TODO: ensure txt_splash text is an integer
		int t = atoi(lv_textarea_get_text(txt_splash));
		if (t < 1 || t > 300) {
			tt_obj_info_box_create("ERROR",
					"Inactivity time must be in the interval [1-300] (minutes)", 1);
			return;
		}
		config_set_inactivity_time(t);
	}
}

static void txt_modbus_addr_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_NUM);
		lv_scr_load(kb_scr);
	}
	if (code == LV_EVENT_READY) {
		// TODO: ensure txt_modbus_addr text is an integer
		models_modbus_t modbus;
		modbus.addr = atoi(lv_textarea_get_text(txt_modbus_addr));
		if (modbus.addr < 0 || modbus.addr > 255) {
			tt_obj_info_box_create("ERROR",
					"Modbus address must be in the interval [0-255]", 1);
			return;
		}
		controller_put_modbus(&modbus);
	}
}

/* Function definitions *******************************************************/

static void revert_rot()
{
	int rotation = config_get_rotation();
	screen_set_rotation(rotation);
}

static char* get_last_element(const char* str)
{
	char *last_element = NULL;
	char delimiter = '/';
	char *token = strtok((char*)str, &delimiter);

	while (token != NULL) {
		last_element = token;
		token = strtok(NULL, &delimiter);
	}

	return last_element;
}

/* Public functions ***********************************************************/

void scr_settings_create(lv_obj_t* l_menu, lv_obj_t* btn)
{
	menu = l_menu;

	lv_obj_t* settings_cont = tt_obj_menu_page_create(menu, btn, menu_cb,
			"Settings");

	diagnose_page = scr_diagnose_create(menu);
	support_page = scr_support_create(menu);

	lv_obj_t* settings_cont1 = tt_obj_cont_create(settings_cont);

	tt_obj_label_create(settings_cont1, "Screen rotation");
	char* options = "0 deg\n90 deg\n180 deg\n270 deg";
	dd = tt_obj_dropdown_create(settings_cont1, options, rotate_cb);
	int rotation = config_get_rotation();
	lv_dropdown_set_selected(dd, rotation);

	tt_obj_label_create(settings_cont1, "Screensaver time (min)");
	txt_splash = tt_obj_txt_create(settings_cont1, "Time in minutes",
			txt_inactivity_cb);
	char inactivity_time_str[10];
	sprintf(inactivity_time_str, "%d", config_get_inactivity_time());
	lv_textarea_set_text(txt_splash, inactivity_time_str);

	lv_obj_t* settings_nw_cont = tt_obj_cont_create(settings_cont);
	lv_obj_t* btn_nw = tt_obj_btn_std_create(settings_nw_cont, NULL,
			"NETWORK SETUP");
	scr_settings_nw_create(menu, btn_nw);
	tt_obj_btn_std_create(settings_nw_cont, btn_nw_reset_cb, "RESET NETWORK");
	btn_ssh = tt_obj_btn_toggle_create(settings_nw_cont, btn_ssh_cb, "SSH");
	btn_snmp = tt_obj_btn_toggle_create(settings_nw_cont, btn_snmp_cb, "SNMP");
	btn_modbus = tt_obj_btn_toggle_create(settings_nw_cont,
			btn_modbus_cb, "MODBUS");

	tt_obj_label_create(settings_nw_cont, "Modbus address");
	txt_modbus_addr = tt_obj_txt_create(settings_nw_cont, "Modbus address",
			txt_modbus_addr_cb);

	lv_obj_t* settings_setup_cont = tt_obj_cont_create(settings_cont);
	tt_obj_label_create(settings_setup_cont, "System setup");
	tt_obj_btn_std_create(settings_setup_cont, btn_update_cb,
			"UPDATE DEVICE");
	tt_obj_btn_std_create(settings_setup_cont, btn_fact_reset_cb,
			"FACTORY RESET");
	tt_obj_btn_std_create(settings_setup_cont, btn_reboot_cb, "REBOOT SYSTEM");

	lv_obj_t* settings_diag_cont = tt_obj_cont_create(settings_cont);
	tt_obj_label_create(settings_diag_cont, "System diagnosis");
	tt_obj_btn_std_create(settings_diag_cont, btn_diag_cb, "DIAGNOSE");
	tt_obj_btn_std_create(settings_diag_cont, btn_support_cb, "SUPPORT");
}
