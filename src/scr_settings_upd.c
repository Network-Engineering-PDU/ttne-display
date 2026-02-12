#include <stdio.h>

#include "lvgl/lvgl.h"
#include "scr_settings_upd.h"
#include "scr_settings_nw.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"
#include "runbg.h"

#define TIMER_MSG_BOX_PERIOD 10000 // ms
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

static lv_obj_t* txt_server;
static lv_obj_t* btn_auto;

/* Function prototypes ********************************************************/
static void timer_check_update_cb(lv_timer_t* timer);
static void loader_cb(lv_event_t* e);

static void txt_server_cb(lv_event_t* e);
static void btn_auto_cb(lv_event_t* e);
static void btn_update_cb(lv_event_t* e);
static void btn_reboot_cb(lv_event_t* e);
static void btn_factory_cb(lv_event_t* e);

static void msg_box_update_cb(lv_event_t* e);

/* Function definitions *******************************************************/

void scr_settings_update_create(lv_obj_t* menu, lv_obj_t* btn) {

	/* Create page and bind to main menu button so header/back works */
	lv_obj_t* cont = tt_obj_menu_page_create(menu, btn, NULL, "PDU update");

	lv_obj_t* main = tt_obj_cont_create(cont);

	/* Remote update server */
	tt_obj_label_create(main,
		"Remote server update file location I.P / DNS");

	/* Server input row: text area */
	lv_obj_t* server_row = tt_obj_cont_create(main);
	lv_obj_set_flex_flow(server_row, LV_FLEX_FLOW_ROW);

	txt_server = tt_obj_txt_create(
		server_row,
		"IP or DNS",
		txt_server_cb
	);
	lv_obj_set_width(txt_server, LV_PCT(80));

	/* Small toggle next to server input: acts as "automatic update without confirmation" checkbox */
	btn_auto = tt_obj_btn_toggle_perc_create(server_row, btn_auto_cb, NULL, 12);

	/* Automatic update toggle: label + compact toggle */
	lv_obj_t* auto_row = tt_obj_cont_create(main);
	lv_obj_set_flex_flow(auto_row, LV_FLEX_FLOW_ROW);

	lv_obj_t* lbl_auto = lv_label_create(auto_row);
	lv_label_set_text(lbl_auto, "Automatic updates (no confirmation)");
	lv_obj_set_width(lbl_auto, LV_PCT(100));
	lv_obj_align(lbl_auto, LV_ALIGN_LEFT_MID, 0, 0);

	/* Buttons row: use matrix-style buttons so each becomes a rounded square */
	lv_obj_t* row = tt_obj_cont_create(main);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
			LV_FLEX_ALIGN_CENTER);

	tt_obj_btn_mtx_create(row, btn_update_cb, "Update from USB", NULL);
	tt_obj_btn_mtx_create(row, btn_reboot_cb, "Reboot", NULL);
	tt_obj_btn_mtx_create(row, btn_factory_cb, "Factory reset", NULL);
}

static void txt_server_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* kb = scr_keyboard_create(
            lv_scr_act(),
            lv_event_get_target(e),
            KB_NUM
        );
        lv_scr_load(kb);
    }

    if (code == LV_EVENT_READY) {
        const char* server = lv_textarea_get_text(txt_server);
        //controller_set_update_server(server);
    }
}

static void btn_auto_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        bool enabled = lv_obj_get_state(btn_auto) & LV_STATE_CHECKED;
        //controller_set_auto_update(enabled);
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

static void btn_reboot_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        controller_post_reboot();
    }
}

static void btn_factory_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        controller_post_fact_reset();
    }
}
