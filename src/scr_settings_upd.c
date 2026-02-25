#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "scr_settings_upd.h"
#include "scr_keyboard.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"
#include "models.h"
#include "controller.h"
#include "runbg.h"

#define TIMER_CHECK_UPDATE 1000 // ms

#ifdef SIMULATOR_ENABLED
#define MOUNT_DIR "/home/guille"
#else
#define MOUNT_DIR "/run/mount"
#endif

/* Global variables ***********************************************************/

static lv_obj_t* txt_server;
static lv_obj_t* btn_auto;
static pid_t update_pid;
static char update_dev[20];
static lv_timer_t* timer_check_update;

/* Function prototypes ********************************************************/
static void txt_server_cb(lv_event_t* e);
static void btn_auto_cb(lv_event_t* e);
static void btn_update_cb(lv_event_t* e);
static void btn_reboot_cb(lv_event_t* e);
static void btn_factory_cb(lv_event_t* e);
static void msg_box_update_cb(lv_event_t* e);
static void timer_check_update_cb(lv_timer_t* timer);
static void loader_cb(lv_event_t* e);

/* Layout Helper **************************************************************/

/**
 * Creates a centered, square-ish action button as seen in the mockup
 */
static lv_obj_t* create_action_btn(lv_obj_t* parent, const char* text, lv_event_cb_t cb) {
    lv_obj_t* btn = tt_obj_btn_mtx_create(parent, cb, text, NULL);
    lv_obj_set_size(btn, LV_PCT(30), 120); // Make them large and proportional
    lv_obj_set_style_radius(btn, 15, 0);   // Rounded corners
    return btn;
}

/* Function definitions *******************************************************/

void scr_settings_update_create(lv_obj_t* menu, lv_obj_t* btn) {
    /* Create page */
    lv_obj_t* page = tt_obj_menu_page_create(menu, btn, NULL, "Sys update");

    /* Main Container: Vertical layout */
    lv_obj_t* cont = tt_obj_cont_create(page);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(cont, 15, 0);

    // 1. Label: Server Location
    tt_obj_label_create(cont, "Remote server update file location I.P / DNS");

    // 2. Row: Server Input + Auto Checkbox
    lv_obj_t* row_top = lv_obj_create(cont);
    lv_obj_set_size(row_top, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_top, 0, 0);
    lv_obj_set_style_border_opa(row_top, 0, 0);
    lv_obj_set_flex_flow(row_top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    txt_server = tt_obj_txt_create(row_top, "IP ", txt_server_cb);
    lv_obj_set_flex_grow(txt_server, 1); // Expand to fill space

    btn_auto = tt_obj_btn_toggle_perc_create(row_top, btn_auto_cb, "X", 15);
    lv_obj_set_size(btn_auto, 50, 50); // Small square checkbox/toggle

    // 3. Label: Automatic Updates
    lv_obj_t* lbl_auto = tt_obj_label_create(cont, "Automatic updates (no confirmation)");
    lv_obj_set_style_text_font(lbl_auto, &lv_font_montserrat_14, 0);

    // 4. Action Buttons Row (3-column grid)
    lv_obj_t* row_btns = lv_obj_create(cont);
    lv_obj_set_size(row_btns, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_btns, 0, 0);
    lv_obj_set_style_border_opa(row_btns, 0, 0);
    lv_obj_set_flex_flow(row_btns, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_btns, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    create_action_btn(row_btns, "Update from\nUSB", btn_update_cb);
    create_action_btn(row_btns, "Reboot", btn_reboot_cb);
    create_action_btn(row_btns, "Factory reset", btn_factory_cb);
}

/* Callbacks ******************************************************************/

static void txt_server_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_scr_load(scr_keyboard_create(lv_scr_act(), lv_event_get_target(e), KB_ABC));
    }
    if (lv_event_get_code(e) == LV_EVENT_READY) {
        // controller_set_update_server(lv_textarea_get_text(txt_server));
    }
}

static void btn_auto_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        bool enabled = lv_obj_has_state(btn_auto, LV_STATE_CHECKED);
        // controller_set_auto_update(enabled);
    }
}

static void btn_update_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        FILE* file = fopen("/proc/mounts", "r");
        if (!file) return;

        char line[500], dev[50], path[50];
        bool found = false;
        while (fgets(line, sizeof(line), file)) {
            if (sscanf(line, "%s %s", dev, path) == 2 && strstr(path, MOUNT_DIR)) {
                found = true;
                char msg[200];
                sprintf(msg, "USB device " TT_COLOR_GREEN_NE_STR " %s# detected. Update?", path);
                // Copy device name (e.g., sda1)
                char* last = strrchr(dev, '/');
                strcpy(update_dev, last ? last + 1 : dev);
                
                tt_obj_msg_box_create("Device update", msg, "Updating...", msg_box_update_cb);
                break;
            }
        }
        if (!found) tt_obj_info_box_create("Update", "No USB detected", 1);
        fclose(file);
    }
}

static void btn_reboot_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) controller_post_reboot();
}

static void btn_factory_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) controller_post_fact_reset();
}

/* Background Process Handling ************************************************/

static void msg_box_update_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED && lv_msgbox_get_active_btn(lv_event_get_current_target(e)) == 0) {
        update_pid = runbg_run("/usr/bin/usb_autorun.sh", "add", update_dev, NULL);
        timer_check_update = lv_timer_create(timer_check_update_cb, TIMER_CHECK_UPDATE, NULL);
        
        lv_obj_t* loader = tt_obj_loader_create("Applying Update...", NULL);
        lv_obj_add_event_cb(loader, loader_cb, LV_EVENT_ALL, lv_scr_act());
        lv_scr_load(loader);
    }
    lv_msgbox_close(lv_event_get_current_target(e));
}

static void timer_check_update_cb(lv_timer_t* timer) {
    int is_running;
    if (runbg_check(update_pid, &is_running) != 0 || !is_running) {
        lv_timer_del(timer);
        runbg_check_wait(runbg_run("/usr/bin/usb_autorun.sh", "remove", update_dev, NULL));
    }
}

static void loader_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_scr_load((lv_obj_t*)lv_event_get_user_data(e));
        lv_obj_del(lv_event_get_target(e));
    }
}