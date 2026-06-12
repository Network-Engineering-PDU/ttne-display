#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "scr_settings_upd.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"
#include "config.h"
#include "runbg.h"

#define TIMER_CHECK_UPDATE 1000
#define TIMER_POLL_UPDATE_STATUS 2000
#define DEFAULT_UPDATE_SERVER "https://github.com/Network-Engineering-PDU/firmware-update"

#ifdef SIMULATOR_ENABLED
#define MOUNT_DIR "/home/guille"
#else
#define MOUNT_DIR "/run/mount"
#endif

static lv_obj_t* txt_server;
static lv_obj_t* dd_auto;
static lv_obj_t* loader_scr;
static lv_obj_t* active_scr;
static lv_timer_t* timer_check_update;
static lv_timer_t* timer_poll_update_status;
static char update_dev[64];
static pid_t update_pid;
static bool update_confirmation_shown = false;

static void menu_cb(lv_event_t* e);
static void timer_check_update_cb(lv_timer_t* timer);
static void timer_poll_update_status_cb(lv_timer_t* timer);
static void loader_cb(lv_event_t* e);
static void txt_server_cb(lv_event_t* e);
static void dd_auto_cb(lv_event_t* e);
static void btn_update_cb(lv_event_t* e);
static void btn_reboot_cb(lv_event_t* e);
static void btn_factory_cb(lv_event_t* e);
static void msg_box_update_cb(lv_event_t* e);
static void msg_box_update_confirmation_cb(lv_event_t* e);
static void msg_box_reboot_cb(lv_event_t* e);
static void msg_box_factory_cb(lv_event_t* e);
static void update_controls_from_status(
		const models_update_status_t* update_status);
static void get_last_element(char* dest, size_t dest_size, const char* str);
static void save_update_settings();

static void get_last_element(char* dest, size_t dest_size, const char* str)
{
	const char* last = strrchr(str, '/');
	if (last != NULL) {
		last++;
	} else {
		last = str;
	}
	snprintf(dest, dest_size, "%s", last);
}

static void save_update_settings()
{
	bool auto_update = lv_dropdown_get_selected(dd_auto) == 0;
	const char* update_server = lv_textarea_get_text(txt_server);
	controller_put_update_settings(auto_update, update_server);
}

static void menu_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* menu = lv_event_get_current_target(e);
	lv_obj_t* curr_page = lv_event_get_user_data(e);
	lv_obj_t* page = lv_menu_get_cur_main_page(menu);

	if (code == LV_EVENT_VALUE_CHANGED && curr_page == page) {
		if (timer_poll_update_status == NULL) {
			update_confirmation_shown = false;
			timer_poll_update_status = lv_timer_create(
					timer_poll_update_status_cb, TIMER_POLL_UPDATE_STATUS,
					NULL);
		}
		lv_timer_resume(timer_poll_update_status);
		timer_poll_update_status_cb(timer_poll_update_status);
	} else if (code == LV_EVENT_CLICKED) {
		if (timer_poll_update_status != NULL) {
			lv_timer_pause(timer_poll_update_status);
		}
	}
}

static void timer_poll_update_status_cb(lv_timer_t* timer)
{
	controller_get_update_status();

	const models_update_status_t* update_status = models_get_update_status();
	update_controls_from_status(update_status);

	LV_LOG_USER("Poll update: pending=%d, auto_update=%d, shown=%d",
			update_status->is_pending, update_status->auto_update,
			update_confirmation_shown);

	if (update_status->is_pending && update_status->auto_update &&
			!update_confirmation_shown) {
		update_confirmation_shown = true;
		lv_timer_pause(timer);
		tt_obj_msg_box_create("Firmware update",
				"Firmware update available.\nProceed with the update?",
				"Updating device...", msg_box_update_confirmation_cb);
	}
}

static void update_controls_from_status(
		const models_update_status_t* update_status)
{
	if (dd_auto != NULL) {
		uint16_t expected_sel = update_status->auto_update ? 0 : 1;
		if (lv_dropdown_get_selected(dd_auto) != expected_sel) {
			lv_dropdown_set_selected(dd_auto, expected_sel);
		}
	}

	if (txt_server != NULL &&
			update_status->update_server != NULL &&
			strcmp(update_status->update_server, "N/A") != 0 &&
			strlen(update_status->update_server) > 0 &&
			strcmp(lv_textarea_get_text(txt_server),
					update_status->update_server) != 0) {
		lv_textarea_set_text(txt_server, update_status->update_server);
	}
}

static void dd_auto_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		save_update_settings();
	}
}

static void txt_server_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t* kb_scr = scr_keyboard_create(lv_scr_act(), obj, KB_ABC);
		lv_scr_load(kb_scr);
	} else if (code == LV_EVENT_READY || code == LV_EVENT_DEFOCUSED) {
		const char* server_addr = lv_textarea_get_text(obj);
		if (server_addr != NULL && strlen(server_addr) > 0) {
			save_update_settings();
		}
	}
}

static void btn_update_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}

	FILE* file = fopen("/proc/mounts", "r");
	if (file == NULL) {
		tt_obj_info_box_create("Device update", "Cannot read mounted devices", 1);
		return;
	}

	char line[500];
	char dev[128];
	char path[128];
	int n_devices = 0;
	while (fgets(line, sizeof(line), file) != NULL) {
		if (sscanf(line, "%127s %127s %*s %*s %*s %*s", dev, path) != 2) {
			continue;
		}
		if (strstr(path, MOUNT_DIR) != NULL) {
			n_devices++;
			char dev_name[64];
			get_last_element(dev_name, sizeof(dev_name), path);
			get_last_element(update_dev, sizeof(update_dev), dev);
			char msg[200];
			snprintf(msg, sizeof(msg), "USB device " TT_COLOR_GREEN_NE_STR
					" %s# detected. Do you want to update the device?",
					dev_name);
			tt_obj_msg_box_create("Device update", msg,
					"Updating device...", msg_box_update_cb);
			break;
		}
	}
	fclose(file);

	if (n_devices == 0) {
		tt_obj_info_box_create("Device update", "No USB devices detected", 1);
	}
}

static void msg_box_update_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* obj = lv_event_get_current_target(e);
		if (lv_msgbox_get_active_btn(obj) == 0) {
			update_pid = runbg_run("/usr/bin/usb_autorun.sh", "add",
					update_dev, NULL);
			timer_check_update = lv_timer_create(timer_check_update_cb,
					TIMER_CHECK_UPDATE, NULL);
			active_scr = lv_scr_act();
			loader_scr = tt_obj_loader_create("Updating device...", NULL);
			lv_obj_add_event_cb(loader_scr, loader_cb, LV_EVENT_ALL,
					active_scr);
			lv_scr_load(loader_scr);
		}
		lv_msgbox_close(obj);
	}
}

static void msg_box_update_confirmation_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* obj = lv_event_get_current_target(e);
		if (lv_msgbox_get_active_btn(obj) == 0) {
			controller_post_update_confirm(true);
			active_scr = lv_scr_act();
			loader_scr = tt_obj_loader_create("Updating device...", NULL);
			lv_obj_add_event_cb(loader_scr, loader_cb, LV_EVENT_ALL,
					active_scr);
			lv_scr_load(loader_scr);
		} else {
			controller_post_update_confirm(false);
			update_confirmation_shown = false;
			if (timer_poll_update_status != NULL) {
				lv_timer_resume(timer_poll_update_status);
			}
		}
		lv_msgbox_close(obj);
	}
}

static void loader_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		lv_obj_t* scr = lv_event_get_user_data(e);
		lv_scr_load(scr);
		lv_obj_del(lv_event_get_current_target(e));
		loader_scr = NULL;
	}
}

static void timer_check_update_cb(lv_timer_t* timer)
{
	int running;
	int ret_code = runbg_check(update_pid, &running);
	if (running) {
		return;
	}

	lv_timer_del(timer);
	timer_check_update = NULL;
	pid_t pid = runbg_run("/usr/bin/usb_autorun.sh", "remove",
			update_dev, NULL);
	runbg_check_wait(pid);

	if (loader_scr != NULL) {
		lv_scr_load(active_scr);
		lv_obj_del(loader_scr);
		loader_scr = NULL;
	}

	if (ret_code == 0) {
		tt_obj_info_box_create("Device update", "Update finished", 0);
	} else {
		tt_obj_info_box_create("Device update", "Update failed", 1);
	}
}

static void msg_box_reboot_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* obj = lv_event_get_current_target(e);
		if (lv_msgbox_get_active_btn(obj) == 0) {
			lv_obj_t* l_loader_scr =
					tt_obj_loader_create("Rebooting device...", NULL);
			lv_scr_load(l_loader_scr);
			controller_post_reboot();
		}
		lv_msgbox_close(obj);
	}
}

static void msg_box_factory_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* obj = lv_event_get_current_target(e);
		if (lv_msgbox_get_active_btn(obj) == 0) {
			if (txt_server != NULL) {
				lv_textarea_set_text(txt_server, DEFAULT_UPDATE_SERVER);
			}
			config_set_skip_login(0);
			lv_obj_t* l_loader_scr =
					tt_obj_loader_create("Resetting device...", NULL);
			lv_scr_load(l_loader_scr);
			controller_post_fact_reset();
		}
		lv_msgbox_close(obj);
	}
}

static void btn_reboot_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		tt_obj_msg_box_create("System reboot",
				"Are you sure you want to reboot the system?",
				"Rebooting device...", msg_box_reboot_cb);
	}
}

static void btn_factory_cb(lv_event_t* e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		tt_obj_msg_box_create("Factory reset",
				"Are you sure you want to perform a factory reset?\n"
				"All settings will be lost!",
				"Resetting device...", msg_box_factory_cb);
	}
}

void scr_settings_update_create(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_obj_t* cont = tt_obj_menu_page_create(menu, btn, menu_cb,
			"Settings / Sys Update");
	lv_obj_t* main = tt_obj_cont_create(cont);
	lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(main, 15, 0);
	lv_obj_set_style_pad_row(main, 12, 0);

	tt_obj_label_create(main, "Remote server update file location I.P / DNS");

	txt_server = tt_obj_txt_create(main, "IP / DNS", txt_server_cb);
	lv_obj_set_width(txt_server, LV_PCT(100));
	lv_obj_set_height(txt_server, 45);
	lv_textarea_set_text(txt_server, DEFAULT_UPDATE_SERVER);

	lv_obj_t* auto_row = lv_obj_create(main);
	lv_obj_set_size(auto_row, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_style_bg_opa(auto_row, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(auto_row, 0, 0);
	lv_obj_clear_flag(auto_row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(auto_row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(auto_row, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* lbl_auto = lv_label_create(auto_row);
	lv_label_set_text(lbl_auto, "Automatic updates");
	lv_obj_set_flex_grow(lbl_auto, 1);

	dd_auto = lv_dropdown_create(auto_row);
	lv_dropdown_set_options(dd_auto, "ON\nOFF");
	lv_dropdown_set_selected(dd_auto, 1);
	lv_obj_set_size(dd_auto, 70, 40);
	lv_obj_set_style_bg_color(dd_auto, lv_color_black(), 0);
	lv_obj_set_style_border_color(dd_auto, lv_color_white(), 0);
	lv_obj_set_style_border_width(dd_auto, 2, 0);
	lv_obj_add_event_cb(dd_auto, dd_auto_cb, LV_EVENT_VALUE_CHANGED, NULL);

	lv_obj_t* row = tt_obj_cont_create(main);
	lv_obj_set_width(row, LV_PCT(100));
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER,
			LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	tt_obj_btn_mtx_create(row, btn_update_cb, "Update\nFrom USB",
			ASSET("usb.png"));
	tt_obj_btn_mtx_create(row, btn_reboot_cb, "Reboot",
			ASSET("reboot.png"));
	tt_obj_btn_mtx_create(row, btn_factory_cb, "Factory\nReset",
			ASSET("f_reset.png"));
}
