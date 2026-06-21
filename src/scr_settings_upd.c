#include <stdio.h>
#include <string.h>
#include "lvgl/lvgl.h"
#include "scr_settings_upd.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "app/app_state.h"
#include "backend/backend.h"

#define TIMER_MSG_BOX_PERIOD 10000 
#define TIMER_ROT 5000 
#define TIMER_CHECK_UPDATE 1000 
#define TIMER_POLL_UPDATE_STATUS 2000  // Poll every 2 seconds

#define DEFAULT_UPDATE_SERVER "https://github.com/Network-Engineering-PDU/firmware-update"

#ifdef UI_POLL_DEBUG_LOGS
#define UI_POLL_LOG(...) LV_LOG_USER(__VA_ARGS__)
#else
#define UI_POLL_LOG(...) ((void)0)
#endif

/* Global variables ***********************************************************/
static lv_obj_t* txt_server;
static lv_obj_t* btn_auto; // This is now a dropdown
static lv_obj_t* dd_period;
static lv_timer_t* timer_check_update;
static lv_timer_t* timer_poll_update_status;  // Timer for polling update status
static bool update_confirmation_shown = false;  // Track if we've already shown the confirmation
static bool update_status_refresh_pending;
static bool usb_detect_pending;
static bool usb_update_poll_pending;

/* Function prototypes ********************************************************/
static void timer_check_update_cb(lv_timer_t* timer);
static void timer_poll_update_status_cb(lv_timer_t* timer);
static void update_status_refresh_cb(int err, void* userdata);
static void update_settings_cb(int err, void* userdata);
static void update_confirm_cb(int err, void* userdata);
static void usb_detect_cb(int err, void* userdata);
static void usb_update_start_cb(int err, void* userdata);
static void usb_update_poll_cb(int err, void* userdata);
static void loader_cb(lv_event_t* e);
static void txt_server_cb(lv_event_t* e);
static void btn_auto_cb(lv_event_t* e);
static void dd_period_cb(lv_event_t* e);
static void btn_update_cb(lv_event_t* e);
static void btn_reboot_cb(lv_event_t* e);
static void btn_factory_cb(lv_event_t* e);
static void msg_box_update_cb(lv_event_t* e);
static void msg_box_update_confirmation_cb(lv_event_t* e);
static void msg_box_reboot_cb(lv_event_t* e);
static void msg_box_factory_cb(lv_event_t* e);
static void apply_update_status_snapshot(void);
static void update_controls_from_status(
        const app_state_update_status_t* update_status);
static uint16_t period_sel_from_hours(int hours);
static int period_hours_from_sel(uint16_t sel);

/* Main Screen Creation *******************************************************/

void scr_settings_update_create(lv_obj_t* menu, lv_obj_t* btn) {

    /* 1. Create page and main container */
    lv_obj_t* cont = tt_obj_menu_page_create(menu, btn, NULL, "System Update");
    lv_obj_t* main = tt_obj_cont_create(cont);
    lv_obj_set_flex_flow(main, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(main, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(main, 15, 0);
    lv_obj_set_style_pad_row(main, 12, 0);
    lv_obj_set_style_pad_column(main, 6, 0);

    /* 2. Remote update server Label */
    tt_obj_label_create(main, "Remote server update file location I.P / DNS");

    /* 3. Large Text Area (Text VRB) */
    txt_server = tt_obj_txt_create(main, "IP / DNS", txt_server_cb);
    lv_obj_set_width(txt_server, LV_PCT(100));
    lv_obj_set_height(txt_server, 45); // Height to match the boxy look
    lv_textarea_set_text(txt_server, DEFAULT_UPDATE_SERVER);

    /* 4. Automatic update row (Label + Dropdown) */
    lv_obj_t* auto_row = lv_obj_create(main);
    lv_obj_set_size(auto_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(auto_row, 0, 0); 
    lv_obj_set_style_border_side(auto_row, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_border_width(auto_row, 0, 0);
    lv_obj_set_style_pad_all(auto_row, 0, 0);
    lv_obj_set_style_pad_column(auto_row, 0, 0);
    lv_obj_clear_flag(auto_row, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_set_flex_flow(auto_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(auto_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_auto = lv_label_create(auto_row);
    lv_label_set_text(lbl_auto, "Automatic Updates");
    lv_label_set_long_mode(lbl_auto, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl_auto, LV_PCT(70));
    lv_obj_set_scrollbar_mode(lbl_auto, LV_SCROLLBAR_MODE_OFF);

    // Create Dropdown for ON/OFF
    btn_auto = lv_dropdown_create(auto_row);
    lv_dropdown_set_options(btn_auto, "ON\nOFF");
    lv_dropdown_set_selected(btn_auto, 1);
    lv_obj_set_size(btn_auto, LV_PCT(30), 40);
    lv_obj_add_event_cb(btn_auto, btn_auto_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // UI Styling for dropdown to match the "X" box style
    lv_obj_set_style_bg_color(btn_auto, lv_color_black(), 0);
    lv_obj_set_style_border_color(btn_auto, lv_color_white(), 0);
    lv_obj_set_style_border_width(btn_auto, 2, 0);

    lv_obj_t* period_row = lv_obj_create(main);
    lv_obj_set_size(period_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(period_row, 0, 0);
    lv_obj_set_style_border_side(period_row, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_border_width(period_row, 0, 0);
    lv_obj_set_style_pad_all(period_row, 0, 0);
    lv_obj_set_style_pad_column(period_row, 0, 0);
    lv_obj_clear_flag(period_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(period_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(period_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_period = lv_label_create(period_row);
    lv_label_set_text(lbl_period, "OTA Periodic Checks");
    lv_label_set_long_mode(lbl_period, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl_period, LV_PCT(70));
    lv_obj_set_scrollbar_mode(lbl_period, LV_SCROLLBAR_MODE_OFF);

    dd_period = tt_obj_dropdown_create(period_row, "Daily\nWeekly\nMonthly", dd_period_cb);
    lv_obj_set_size(dd_period, LV_PCT(30), 40);

    /* 5. Bottom buttons */
    tt_obj_btn_mtx_create(main, btn_update_cb, "  Update\nFrom USB", ASSET("usb2.png"));
    tt_obj_btn_mtx_create(main, btn_reboot_cb, "Reboot", ASSET("reboot.png"));
    tt_obj_btn_mtx_create(main, btn_factory_cb, "Factory\n Reset", ASSET("f_reset.png"));
    
    /* Start polling for pending updates from remote server */
    // Delete any existing timer to prevent duplicates
    if (timer_poll_update_status != NULL) {
        lv_timer_del(timer_poll_update_status);
        timer_poll_update_status = NULL;
    }
    update_confirmation_shown = false;
    timer_poll_update_status = lv_timer_create(timer_poll_update_status_cb, TIMER_POLL_UPDATE_STATUS, NULL);
    timer_poll_update_status_cb(timer_poll_update_status);
    LV_LOG_USER("Update polling timer started");
}

/* Callbacks ******************************************************************/

static void timer_poll_update_status_cb(lv_timer_t* timer) {
    (void)timer;
    if (update_status_refresh_pending) {
        return;
    }
    if (backend_update_status_refresh(update_status_refresh_cb, NULL) == 0) {
        update_status_refresh_pending = true;
    }
}

static void update_status_refresh_cb(int err, void* userdata) {
    (void)userdata;
    update_status_refresh_pending = false;
    if (err != 0) {
        return;
    }
    apply_update_status_snapshot();
}

static void update_settings_cb(int err, void* userdata) {
    (void)userdata;
    if (err == 0) {
        apply_update_status_snapshot();
    }
}

static void update_confirm_cb(int err, void* userdata) {
    (void)userdata;
    if (err == 0) {
        apply_update_status_snapshot();
    }
}

static void usb_detect_cb(int err, void* userdata) {
    (void)userdata;
    usb_detect_pending = false;

    app_state_snapshot_t snapshot;
    app_state_get_snapshot(&snapshot);
    const app_state_usb_update_t* usb_update = &snapshot.usb_update;
    if (err != 0 || !usb_update->valid || !usb_update->device_found) {
        tt_obj_info_box_create("Device update", "No USB detected", 1);
        return;
    }

    char msg[200];
    snprintf(msg, sizeof(msg), "USB device " TT_COLOR_GREEN_NE_STR
            " %s detected. Update?", usb_update->device_name);
    tt_obj_msg_box_create("Device update", msg, "Updating Device...",
            msg_box_update_cb);
}

static void usb_update_start_cb(int err, void* userdata) {
    (void)userdata;
    if (err != 0) {
        tt_obj_info_box_create("Device update", "Could not start USB update", 1);
        return;
    }
    if (timer_check_update == NULL) {
        timer_check_update = lv_timer_create(timer_check_update_cb,
                TIMER_CHECK_UPDATE, NULL);
    }
}

static void usb_update_poll_cb(int err, void* userdata) {
    (void)userdata;
    (void)err;
    usb_update_poll_pending = false;

    app_state_snapshot_t snapshot;
    app_state_get_snapshot(&snapshot);
    if (snapshot.usb_update.valid && snapshot.usb_update.complete &&
            timer_check_update != NULL) {
        lv_timer_del(timer_check_update);
        timer_check_update = NULL;
    }
}

static void apply_update_status_snapshot(void) {
    app_state_snapshot_t snapshot;
    app_state_get_snapshot(&snapshot);
    const app_state_update_status_t* update_status = &snapshot.update_status;

    if (!update_status->valid) {
        return;
    }

    update_controls_from_status(update_status);

    UI_POLL_LOG("Poll: pending=%d, auto_update=%d, shown=%d",
        update_status->is_pending, update_status->auto_update, update_confirmation_shown);

    // Only show confirmation once, when an update is pending and auto_update is enabled
    if (update_status->is_pending && update_status->auto_update && !update_confirmation_shown) {
        update_confirmation_shown = true;
        LV_LOG_USER("Firmware update detected! Showing confirmation dialog.");
        
        // Stop polling while showing confirmation dialog
        if (timer_poll_update_status != NULL) {
            lv_timer_pause(timer_poll_update_status);
        }
        
        char msg[300];
        snprintf(msg, sizeof(msg), "Firmware Update Available!\n\nAuto update is enabled.\nDo you want to proceed with the update?");
        tt_obj_msg_box_create("Firmware Update", msg, "Updating device...", msg_box_update_confirmation_cb);
    }
}

static void update_controls_from_status(
        const app_state_update_status_t* update_status) {
    if (btn_auto != NULL) {
        uint16_t expected_sel = update_status->auto_update ? 0 : 1;
        if (lv_dropdown_get_selected(btn_auto) != expected_sel) {
            lv_dropdown_set_selected(btn_auto, expected_sel);
        }
    }

    if (dd_period != NULL) {
        uint16_t expected_sel =
                period_sel_from_hours(update_status->check_interval_hours);
        if (lv_dropdown_get_selected(dd_period) != expected_sel) {
            lv_dropdown_set_selected(dd_period, expected_sel);
        }
    }

    if (txt_server != NULL &&
        strcmp(update_status->update_server, "N/A") != 0 &&
        strlen(update_status->update_server) > 0 &&
        strcmp(lv_textarea_get_text(txt_server), update_status->update_server) != 0) {
        lv_textarea_set_text(txt_server, update_status->update_server);
    }
}

static void btn_auto_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_target(e);
        uint16_t sel = lv_dropdown_get_selected(obj);
        bool enabled = (sel == 0); // 0 is ON, 1 is OFF
        LV_LOG_USER("Auto-update changed to: %s", enabled ? "ON" : "OFF");
        backend_update_set_auto(enabled, update_settings_cb, NULL);
    }
}

static void dd_period_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_target(e);
        int hours = period_hours_from_sel(lv_dropdown_get_selected(obj));
        LV_LOG_USER("OTA periodic check interval changed to: %d hours", hours);
        backend_update_set_interval(hours, update_settings_cb, NULL);
    }
}

static uint16_t period_sel_from_hours(int hours) {
    switch (hours) {
        case 1:
            return 0;
        case 168:
            return 2;
        case 720:
            return 3;
        case 24:
        default:
            return 1;
    }
}

static int period_hours_from_sel(uint16_t sel) {
    switch (sel) {
        case 0:
            return 1;
        case 2:
            return 168;
        case 3:
            return 720;
        case 1:
        default:
            return 24;
    }
}

static void txt_server_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* kb = scr_keyboard_create(lv_scr_act(), lv_event_get_target(e), KB_NUM);
        lv_scr_load(kb);
    } else if (code == LV_EVENT_DEFOCUSED) {
        // After keyboard closes, save the server address
        lv_obj_t* txt_obj = lv_event_get_target(e);
        const char* server_addr = lv_textarea_get_text(txt_obj);
        if (server_addr && strlen(server_addr) > 0) {
            backend_update_set_server(server_addr, update_settings_cb, NULL);
        }
    }
}

static void btn_update_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (usb_detect_pending) {
            return;
        }
        if (backend_usb_update_detect(usb_detect_cb, NULL) == 0) {
            usb_detect_pending = true;
        }
    }
}

static void msg_box_update_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        if (lv_msgbox_get_active_btn(obj) == 0) {
            app_state_snapshot_t snapshot;
            app_state_get_snapshot(&snapshot);
            backend_usb_update_start(snapshot.usb_update.update_dev,
                    usb_update_start_cb, NULL);
            lv_obj_t* loader_scr = tt_obj_loader_create("Updating Device...", NULL);
            lv_obj_add_event_cb(loader_scr, loader_cb, LV_EVENT_ALL, lv_scr_act());
            lv_scr_load(loader_scr);
        }
        lv_msgbox_close(obj);
    }
}

static void msg_box_update_confirmation_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        if (lv_msgbox_get_active_btn(obj) == 0) {
            // YES - confirm update
            backend_update_confirm(true, update_confirm_cb, NULL);
            lv_obj_t* loader_scr = tt_obj_loader_create("Updating Device...", NULL);
            lv_obj_add_event_cb(loader_scr, loader_cb, LV_EVENT_ALL, lv_scr_act());
            lv_scr_load(loader_scr);
        } else {
            // NO - reject update
            backend_update_confirm(false, update_confirm_cb, NULL);
            update_confirmation_shown = false;
        }
        lv_msgbox_close(obj);
        
        // Resume polling
        if (timer_poll_update_status != NULL) {
            lv_timer_resume(timer_poll_update_status);
        }
    }
}

static void loader_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_scr_load(lv_event_get_user_data(e));
        lv_obj_del(lv_event_get_current_target(e));
    }
}

static void timer_check_update_cb(lv_timer_t* timer) {
    (void)timer;
    if (usb_update_poll_pending) {
        return;
    }
    if (backend_usb_update_poll(usb_update_poll_cb, NULL) == 0) {
        usb_update_poll_pending = true;
    }
}

static void msg_box_reboot_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        if (lv_msgbox_get_active_btn(obj) == 0) {
            lv_obj_t* loader_scr = tt_obj_loader_create("Rebooting Device...", NULL);
            lv_scr_load(loader_scr);
            backend_system_reboot(NULL, NULL);
        }
        lv_msgbox_close(obj);
    }
}

static void msg_box_factory_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        if (lv_msgbox_get_active_btn(obj) == 0) {
            if (txt_server != NULL) {
                lv_textarea_set_text(txt_server, DEFAULT_UPDATE_SERVER);
            }
            lv_obj_t* loader_scr = tt_obj_loader_create("Resetting to factory defaults...", NULL);
            lv_scr_load(loader_scr);
            backend_system_factory_reset(NULL, NULL);
        }
        lv_msgbox_close(obj);
    }
}

static void btn_reboot_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        char msg[200];
        sprintf(msg, "Are you sure you want to reboot the system?");
        tt_obj_msg_box_create("System reboot", msg, "Rebooting Device...", msg_box_reboot_cb);
    }
}

static void btn_factory_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        char msg[200];
        sprintf(msg, "Are you sure you want to perform a factory reset?\nAll settings will be lost!");
        tt_obj_msg_box_create("Factory reset", msg, "Resetting Device...", msg_box_factory_cb);
    }
}
