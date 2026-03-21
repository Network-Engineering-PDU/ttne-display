#include <stdio.h>
#include <string.h>

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

#define TIMER_MSG_BOX_PERIOD 10000 
#define TIMER_ROT 5000 
#define TIMER_CHECK_UPDATE 1000 
#define TIMER_POLL_UPDATE_STATUS 2000  // Poll every 2 seconds

#ifdef SIMULATOR_ENABLED
#define MOUNT_DIR "/home/guille"
#else
#define MOUNT_DIR "/run/mount"
#endif

/* Global variables ***********************************************************/
static lv_obj_t* txt_server;
static lv_obj_t* btn_auto; // This is now a dropdown
static lv_timer_t* timer_check_update;
static lv_timer_t* timer_poll_update_status;  // Timer for polling update status
static char update_dev[20];
static pid_t update_pid;
static bool update_confirmation_shown = false;  // Track if we've already shown the confirmation

/* Function prototypes ********************************************************/
static void timer_check_update_cb(lv_timer_t* timer);
static void timer_poll_update_status_cb(lv_timer_t* timer);
static void loader_cb(lv_event_t* e);
static void txt_server_cb(lv_event_t* e);
static void btn_auto_cb(lv_event_t* e);
static void btn_update_cb(lv_event_t* e);
static void btn_reboot_cb(lv_event_t* e);
static void btn_factory_cb(lv_event_t* e);
static void msg_box_update_cb(lv_event_t* e);
static void msg_box_update_confirmation_cb(lv_event_t* e);
static void msg_box_reboot_cb(lv_event_t* e);
static void msg_box_factory_cb(lv_event_t* e);

static char* get_last_element(const char* str) {
    char *last_element = NULL;
    char *token = strtok((char*)str, "/");
    while (token != NULL) {
        last_element = token;
        token = strtok(NULL, "/");
    }
    return last_element;
}

/* Main Screen Creation *******************************************************/

void scr_settings_update_create(lv_obj_t* menu, lv_obj_t* btn) {

    /* 1. Create page and main container */
    lv_obj_t* cont = tt_obj_menu_page_create(menu, btn, NULL, "System update");
    lv_obj_t* main = tt_obj_cont_create(cont);
    lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(main, 15, 0);
    lv_obj_set_style_pad_row(main, 12, 0);

    /* 2. Remote update server Label */
    tt_obj_label_create(main, "Remote server update file location I.P / DNS");

    /* 3. Large Text Area (Text VRB) */
    txt_server = tt_obj_txt_create(main, "IP / DNS", txt_server_cb);
    lv_obj_set_width(txt_server, LV_PCT(100));
    lv_obj_set_height(txt_server, 45); // Height to match the boxy look

    /* 4. Automatic update row (Label + Dropdown) */
    lv_obj_t* auto_row = lv_obj_create(main);
    lv_obj_set_size(auto_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(auto_row, 0, 0); 
    lv_obj_set_style_border_side(auto_row, LV_BORDER_SIDE_NONE, 0);
    lv_obj_clear_flag(auto_row, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_set_flex_flow(auto_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(auto_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_auto = lv_label_create(auto_row);
    lv_label_set_text(lbl_auto, "Automatic updates");
    lv_obj_set_flex_grow(lbl_auto, 1);

    // Create Dropdown for ON/OFF
    btn_auto = lv_dropdown_create(auto_row);
    lv_dropdown_set_options(btn_auto, "ON\nOFF");
    lv_dropdown_set_selected(btn_auto, 0); // Default to ON
    lv_obj_set_size(btn_auto, 70, 40);
    lv_obj_add_event_cb(btn_auto, btn_auto_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // UI Styling for dropdown to match the "X" box style
    lv_obj_set_style_bg_color(btn_auto, lv_color_black(), 0);
    lv_obj_set_style_border_color(btn_auto, lv_color_white(), 0);
    lv_obj_set_style_border_width(btn_auto, 2, 0);

    /* 5. Bottom Buttons row (Three Action Buttons) */
    lv_obj_t* row = tt_obj_cont_create(main);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* b_upd = tt_obj_btn_mtx_create(row, btn_update_cb, "Update\nFrom USB", ASSET("menu.png"));

    lv_obj_t* b_reb = tt_obj_btn_mtx_create(row, btn_reboot_cb, "Reboot", ASSET("menu.png"));

    lv_obj_t* b_fac = tt_obj_btn_mtx_create(row, btn_factory_cb, "Factory\nReset", ASSET("menu.png"));
    
    /* Start polling for pending updates from remote server */
    update_confirmation_shown = false;
    timer_poll_update_status = lv_timer_create(timer_poll_update_status_cb, TIMER_POLL_UPDATE_STATUS, NULL);
}

/* Callbacks ******************************************************************/

static void timer_poll_update_status_cb(lv_timer_t* timer) {
    // Poll for pending updates from the remote server
    controller_get_update_status();
    
    const models_update_status_t* update_status = models_get_update_status();
    
    // Only show confirmation once, when an update is pending and auto_update is enabled
    if (update_status->is_pending && update_status->auto_update && !update_confirmation_shown) {
        update_confirmation_shown = true;
        
        // Stop polling while showing confirmation dialog
        lv_timer_pause(timer);
        
        char msg[300];
        sprintf(msg, "Firmware Update Available!\n\nAuto update is enabled.\nDo you want to proceed with the update?");
        tt_obj_msg_box_create("Firmware Update", msg, "Updating device...", msg_box_update_confirmation_cb);
    }
}

static void btn_auto_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_target(e);
        uint16_t sel = lv_dropdown_get_selected(obj);
        bool enabled = (sel == 0); // 0 is ON, 1 is OFF
        // controller_set_auto_update(enabled);
    }
}

static void txt_server_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* kb = scr_keyboard_create(lv_scr_act(), lv_event_get_target(e), KB_NUM);
        lv_scr_load(kb);
    }
}

static void btn_update_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        FILE* file = fopen("/proc/mounts", "r");
        if (file == NULL) return;
        
        char line[500], dev[50], path[50];
        int n_devices = 0;
        while (fgets(line, 500, file) != NULL) {
            sscanf(line, "%s %s", dev, path);
            if (strstr(path, MOUNT_DIR) != NULL) {
                n_devices++;
                char* dev_name = get_last_element(path);
                char msg[200];
                sprintf(msg, "USB device " TT_COLOR_GREEN_NE_STR " %s detected. Update?", dev_name);
                strcpy(update_dev, get_last_element(dev));
                tt_obj_msg_box_create("Device update", msg, "Updating...", msg_box_update_cb);
                break;
            }
        }
        if (n_devices == 0) tt_obj_info_box_create("Device update", "No USB detected", 1);
        fclose(file);
    }
}

static void msg_box_update_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        if (lv_msgbox_get_active_btn(obj) == 0) {
            update_pid = runbg_run("/usr/bin/usb_autorun.sh", "add", update_dev, NULL);
            timer_check_update = lv_timer_create(timer_check_update_cb, TIMER_CHECK_UPDATE, NULL);
            lv_obj_t* loader_scr = tt_obj_loader_create("Updating...", NULL);
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
            controller_post_update_confirm(true);
            lv_obj_t* loader_scr = tt_obj_loader_create("Updating device...", NULL);
            lv_obj_add_event_cb(loader_scr, loader_cb, LV_EVENT_ALL, lv_scr_act());
            lv_scr_load(loader_scr);
        } else {
            // NO - reject update
            controller_post_update_confirm(false);
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
    int running;
    if (runbg_check(update_pid, &running) != 0) return;
    if (!running) {
        lv_timer_del(timer);
        pid_t pid = runbg_run("/usr/bin/usb_autorun.sh", "remove", update_dev, NULL);
        runbg_check_wait(pid);
    }
}

static void msg_box_reboot_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        if (lv_msgbox_get_active_btn(obj) == 0) {
            lv_obj_t* loader_scr = tt_obj_loader_create("Rebooting...", NULL);
            lv_scr_load(loader_scr);
            controller_post_reboot();
        }
        lv_msgbox_close(obj);
    }
}

static void msg_box_factory_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        if (lv_msgbox_get_active_btn(obj) == 0) {
            lv_obj_t* loader_scr = tt_obj_loader_create("Resetting to factory defaults...", NULL);
            lv_scr_load(loader_scr);
            controller_post_fact_reset();
        }
        lv_msgbox_close(obj);
    }
}

static void btn_reboot_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        char msg[200];
        sprintf(msg, "Are you sure you want to reboot the system?");
        tt_obj_msg_box_create("System reboot", msg, "Rebooting...", msg_box_reboot_cb);
    }
}

static void btn_factory_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        char msg[200];
        sprintf(msg, "Are you sure you want to perform a factory reset?\nAll settings will be lost!");
        tt_obj_msg_box_create("Factory reset", msg, "Resetting...", msg_box_factory_cb);
    }
}