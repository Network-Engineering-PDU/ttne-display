#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <cjson/cJSON.h>

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
#include "http_helper.h"

#define TIMER_MSG_BOX_PERIOD 10000 
#define TIMER_ROT 5000 
#define TIMER_CHECK_UPDATE 1000 
#define TIMER_REMOTE_CHECK 30000  // Check for remote updates every 30 seconds (not every 1 second)
#define BASE_URL "http://localhost:8001/"
#define UPDATE_STATUS_FILE "/home/root/.ne/update_status"
#define REMOTE_UPDATE_PENDING_FILE "/home/root/.ne/remote_update_pending"
#define REMOTE_UPDATE_CONFIRMED_FILE "/home/root/.ne/remote_update_confirmed"
#define REMOTE_UPDATE_DECLINED_FILE "/home/root/.ne/remote_update_declined"  // Track user decline with timestamp

#ifdef SIMULATOR_ENABLED
#define MOUNT_DIR "/home/guille"
#else
#define MOUNT_DIR "/run/mount"
#endif

/* Global variables ***********************************************************/
static lv_obj_t* txt_server;
static lv_obj_t* btn_auto; // This is now a dropdown
static lv_timer_t* timer_check_update;
static char update_dev[20];
static pid_t update_pid;

/* Auto-update variables *****/
static bool auto_update_enabled = false;
static lv_timer_t* timer_auto_check = NULL;
static char pending_update_version[20] = {0};
static char current_version[20] = {0};
static lv_timer_t* timer_auto_update_monitor = NULL;
static bool auto_update_in_progress = false;
static lv_obj_t* loader_screen = NULL;  // Track loader screen for updates

/* Remote update variables *****/
static lv_timer_t* timer_remote_update_check = NULL;
static bool remote_update_pending = false;
static char remote_update_filename[256] = {0};
static time_t last_dialog_shown_time = 0;  // Prevent duplicate dialogs
static time_t user_declined_until = 0;     // 24-hour snooze when user clicks NO

/* Function prototypes ********************************************************/
static void timer_check_update_cb(lv_timer_t* timer);
static void loader_cb(lv_event_t* e);
static void txt_server_cb(lv_event_t* e);
static void btn_auto_cb(lv_event_t* e);
static void btn_update_cb(lv_event_t* e);
static void btn_reboot_cb(lv_event_t* e);
static void btn_factory_cb(lv_event_t* e);
static void msg_box_update_cb(lv_event_t* e);
static void msg_box_reboot_cb(lv_event_t* e);
static void msg_box_factory_cb(lv_event_t* e);

/* Auto-update function prototypes */
static void timer_auto_check_cb(lv_timer_t* timer);
static void timer_auto_update_monitor_cb(lv_timer_t* timer);
static void show_auto_update_dialog(const char* current, const char* new_version);
static void auto_update_confirm_cb(lv_event_t* e);
static void load_update_server();

/* Remote update function prototypes */
static void timer_remote_update_check_cb(lv_timer_t* timer);
static void remote_update_confirm_cb(lv_event_t* e);


static char* get_last_element(const char* str) {
    char *last_element = NULL;
    char *token = strtok((char*)str, "/");
    while (token != NULL) {
        last_element = token;
        token = strtok(NULL, "/");
    }
    return last_element;
}

/* Load update server from backend on screen init */
static void load_update_server() {
    char* server = controller_get_update_server();
    if (server && strlen(server) > 0) {
        lv_textarea_set_text(txt_server, server);
        LV_LOG_USER("Update server loaded: %s", server);
    } else {
        lv_textarea_set_text(txt_server, "");
    }
}

/* Load declined timestamp from file on screen init */
static void load_declined_timestamp() {
    FILE* fp = fopen(REMOTE_UPDATE_DECLINED_FILE, "r");
    if (fp) {
        char buffer[32] = {0};
        if (fread(buffer, 1, sizeof(buffer) - 1, fp) > 0) {
            user_declined_until = atol(buffer);
            time_t now = time(NULL);
            if (now < user_declined_until) {
                LV_LOG_USER("Loaded 24-hour snooze: expires in %ld seconds", user_declined_until - now);
            } else {
                user_declined_until = 0;
                unlink(REMOTE_UPDATE_DECLINED_FILE);
            }
        }
        fclose(fp);
    }
}

/* Remote update confirmation dialog callback */
static void remote_update_confirm_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        int btn_id = lv_msgbox_get_active_btn(obj);
        
        if (btn_id == 0) {
            // User clicked YES - signal backend and show updating screen
            LV_LOG_USER("Remote update: User confirmed, calling backend to start update...");
            
            // Clear the pending flag immediately - update is now being processed
            remote_update_pending = false;
            memset(remote_update_filename, 0, sizeof(remote_update_filename));
            unlink(REMOTE_UPDATE_PENDING_FILE);  // Delete pending file
            
            // Call backend to start the actual update
            http_get_req_t req;
            char url[256];
            snprintf(url, sizeof(url), "%ssettings/manual-update-start", BASE_URL);
            
            int err = http_helper_get(&req, url);
            if (err != 0) {
                LV_LOG_WARN("Remote update: Failed to notify backend to start update");
                http_helper_free(&req);
                lv_msgbox_close(obj);
                return;
            }
            
            http_helper_free(&req);
            LV_LOG_USER("Remote update: Backend notified to start update");
            
            // Mark that an update is in progress - prevents duplicate alerts during extraction
            auto_update_in_progress = true;
            loader_screen = tt_obj_loader_create("Updating...", NULL);
            lv_obj_add_event_cb(loader_screen, loader_cb, LV_EVENT_ALL, lv_scr_act());
            lv_scr_load(loader_screen);
            
            // Start monitor timer - checks every 500ms for status updates
            timer_auto_update_monitor = lv_timer_create(timer_auto_update_monitor_cb, 500, NULL);
            
        } else {
            // User clicked NO - clean up and don't proceed
            LV_LOG_USER("Remote update: User rejected update - snoozed for 24 hours");
            
            // Clear the pending flag
            remote_update_pending = false;
            memset(remote_update_filename, 0, sizeof(remote_update_filename));
            
            // Clean up pending file
            unlink(REMOTE_UPDATE_PENDING_FILE);
            
            // Set 24-hour snooze (86400 seconds = 24 hours)
            user_declined_until = time(NULL) + 86400;
            
            // Write declined status with timestamp
            FILE* fp = fopen(REMOTE_UPDATE_DECLINED_FILE, "w");
            if (fp) {
                fprintf(fp, "%ld", user_declined_until);
                fclose(fp);
                LV_LOG_USER("24-hour snooze activated until: %ld", user_declined_until);
            }
        }
        
        lv_msgbox_close(obj);
    }
}

/* Timer callback - checks for pending remote firmware updates */
static void timer_remote_update_check_cb(lv_timer_t* timer) {
    // Check if backend has signaled a pending remote update
    // The pending file will contain the firmware filename
    
    time_t now = time(NULL);
    
    // If auto-update is OFF, don't show any alerts
    if (!auto_update_enabled) {
        // Clean up any pending files
        if (remote_update_pending) {
            remote_update_pending = false;
            memset(remote_update_filename, 0, sizeof(remote_update_filename));
            unlink(REMOTE_UPDATE_PENDING_FILE);
        }
        return;
    }
    
    // If an update is already in progress, don't show duplicate alerts
    if (auto_update_in_progress) {
        LV_LOG_USER("Remote update: Update already in progress, skipping check");
        return;
    }
    
    // Check if user declined within last 24 hours
    if (user_declined_until > 0 && now < user_declined_until) {
        // Still in snooze period - don't show dialog
        LV_LOG_USER("Remote update: Still in 24-hour snooze period");
        return;
    }
    
    // If snooze expired, clear it
    if (user_declined_until > 0 && now >= user_declined_until) {
        user_declined_until = 0;
        unlink(REMOTE_UPDATE_DECLINED_FILE);
        LV_LOG_USER("Remote update: 24-hour snooze period expired");
    }
    
    if (remote_update_pending) {
        // Already showing or processing an update
        return;
    }
    
    FILE* fp = fopen(REMOTE_UPDATE_PENDING_FILE, "r");
    if (fp == NULL) {
        // No pending update
        return;
    }
    
    // Read filename from pending file
    char buffer[256] = {0};
    if (fread(buffer, 1, sizeof(buffer) - 1, fp) > 0) {
        fclose(fp);
        
        // Prevent duplicate dialogs within 5 seconds
        if (last_dialog_shown_time > 0 && (now - last_dialog_shown_time) < 5) {
            LV_LOG_USER("Remote update: Dialog already shown recently, skipping duplicate");
            return;
        }
        
        // Store filename and mark as pending
        strncpy(remote_update_filename, buffer, sizeof(remote_update_filename) - 1);
        remote_update_pending = true;
        last_dialog_shown_time = now;
        
        LV_LOG_USER("Remote update: Pending file detected: %s", remote_update_filename);
        
        // Show confirmation dialog
        char msg[300];
        snprintf(msg, sizeof(msg), 
                 "Device update\n\n"
                 "File: %s\n\n"
                 "Do you want to update\nthe device?",
                 remote_update_filename);
        
        tt_obj_msg_box_create("Device update", msg, "Updating...", remote_update_confirm_cb);
    } else {
        fclose(fp);
    }
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
    
    // Load update server from backend
    load_update_server();
    
    // Load declined timestamp from previous user rejection
    load_declined_timestamp();

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
    lv_dropdown_set_selected(btn_auto, 1); // Default to OFF
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

    lv_obj_t* b_upd = tt_obj_btn_mtx_create(row, btn_update_cb, "  Update\nFrom USB", ASSET("menu.png"));

    lv_obj_t* b_reb = tt_obj_btn_mtx_create(row, btn_reboot_cb, "Reboot", ASSET("menu.png"));

    lv_obj_t* b_fac = tt_obj_btn_mtx_create(row, btn_factory_cb, "Factory\n Reset", ASSET("menu.png"));
    
    /* Start monitoring for remote firmware updates (from web UI) */
    timer_remote_update_check = lv_timer_create(timer_remote_update_check_cb, TIMER_REMOTE_CHECK, NULL);
    LV_LOG_USER("Remote update monitoring started (checking every 30 seconds)");
}

/* Callbacks ******************************************************************/

/* Auto-update timer callback - checks for new firmware every 60 seconds */
static void timer_auto_check_cb(lv_timer_t* timer) {
    if (!auto_update_enabled) {
        return;
    }
    
    LV_LOG_USER("Auto-update: Checking for new firmware...");
    
    // Make HTTP GET request to backend to check for available updates
    http_get_req_t req;
    char url[256];
    snprintf(url, sizeof(url), "%ssettings/auto-update-check", BASE_URL);
    
    int err = http_helper_get(&req, url);
    if (err != 0) {
        LV_LOG_WARN("Auto-update: Failed to check for updates");
        http_helper_free(&req);
        return;
    }
    
    // Parse JSON response
    // Expected format:
    // {
    //   "version_available": true/false,
    //   "current_version": "1.0.0",
    //   "new_version": "1.0.1"
    // }
    
    cJSON* root = cJSON_Parse(req.buffer);
    if (!root) {
        LV_LOG_WARN("Auto-update: Failed to parse response JSON");
        http_helper_free(&req);
        return;
    }
    
    cJSON* available = cJSON_GetObjectItem(root, "version_available");
    cJSON* curr_ver = cJSON_GetObjectItem(root, "current_version");
    cJSON* new_ver = cJSON_GetObjectItem(root, "new_version");
    
    if (available && available->type == cJSON_True) {
        if (curr_ver && new_ver) {
            // Update is available - save versions and show dialog
            strncpy(current_version, curr_ver->valuestring, sizeof(current_version) - 1);
            strncpy(pending_update_version, new_ver->valuestring, sizeof(pending_update_version) - 1);
            
            LV_LOG_USER("Auto-update: New version available %s -> %s", 
                       current_version, pending_update_version);
            
            // Show confirmation dialog to user
            show_auto_update_dialog(current_version, pending_update_version);
        }
    } else if (available && available->type == cJSON_False) {
        LV_LOG_USER("Auto-update: No new version available");
    } else {
        LV_LOG_WARN("Auto-update: Invalid response format");
    }
    
    cJSON_Delete(root);
    http_helper_free(&req);
}

/* Dialog callback for auto-update confirmation */
static void auto_update_confirm_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        int btn_id = lv_msgbox_get_active_btn(obj);
        
        if (btn_id == 0) {
            // User clicked YES - start update
            LV_LOG_USER("Auto-update: User confirmed update to version %s", pending_update_version);
            
            // Mark update as in progress and show "Updating..." screen
            auto_update_in_progress = true;
            loader_screen = tt_obj_loader_create("Updating...", NULL);
            lv_obj_add_event_cb(loader_screen, loader_cb, LV_EVENT_ALL, lv_scr_act());
            lv_scr_load(loader_screen);
            
            // Start monitor timer - checks every 500ms if update status changed
            // This will persist until system restarts
            timer_auto_update_monitor = lv_timer_create(timer_auto_update_monitor_cb, 500, NULL);
            
            // Call backend to start the update
            // Backend will download firmware, apply update, and restart system
            // The monitor timer will poll for status updates and show progress
            controller_post_auto_update_start();
        } else {
            // User clicked NO - just hide dialog and continue checking
            LV_LOG_USER("Auto-update: User skipped update");
        }
        
        lv_msgbox_close(obj);
    }
}

/* Monitor timer callback - polls for status updates and displays progress */
static void timer_auto_update_monitor_cb(lv_timer_t* timer) {
    // This timer runs every 500ms while update is in progress
    // It reads the status file from backend and updates the loader message
    // The system will reboot before this timer can be deleted
    
    // Try to read status file
    FILE* fp = fopen(UPDATE_STATUS_FILE, "r");
    if (fp == NULL) {
        // File not yet created, just keep showing "Updating..."
        LV_LOG_TRACE("Auto-update: Status file not yet available");
        return;
    }
    
    // Read file contents
    char buffer[512] = {0};
    if (fread(buffer, 1, sizeof(buffer) - 1, fp) > 0) {
        fclose(fp);
        
        // Parse JSON
        cJSON* root = cJSON_Parse(buffer);
        if (root) {
            cJSON* status_item = cJSON_GetObjectItem(root, "status");
            cJSON* message_item = cJSON_GetObjectItem(root, "message");
            
            if (status_item && status_item->valuestring && 
                message_item && message_item->valuestring) {
                
                const char* status = status_item->valuestring;
                const char* message = message_item->valuestring;
                
                LV_LOG_USER("Auto-update: Status = %s, Message = %s", status, message);
                
                // Update the loader message if we have a loader screen
                if (loader_screen && lv_obj_is_valid(loader_screen)) {
                    // Find the label inside the loader screen
                    // The label is created by tt_obj_spinner_create and is a child of loader_screen
                    uint32_t child_count = lv_obj_get_child_cnt(loader_screen);
                    for (uint32_t i = 0; i < child_count; i++) {
                        lv_obj_t* child = lv_obj_get_child(loader_screen, i);
                        // Check if this object is a label
                        if (lv_obj_check_type(child, &lv_label_class)) {
                            lv_label_set_text(child, (char*)message);
                            LV_LOG_USER("Auto-update: Updated loader message to: %s", message);
                            break;
                        }
                    }
                }
                
                // Check if error occurred - clear in-progress flag to allow retries
                if (strcmp(status, "error") == 0) {
                    LV_LOG_WARN("Auto-update: Error occurred - %s", message);
                    // Clear the in-progress flag so new updates can be attempted
                    // System will show error message for 10 seconds then allow new alerts
                    auto_update_in_progress = false;
                    LV_LOG_USER("Auto-update: Cleared in-progress flag to allow recovery");
                }
            }
            cJSON_Delete(root);
        } else {
            LV_LOG_WARN("Auto-update: Failed to parse status JSON");
        }
    } else {
        fclose(fp);
    }
}

/* Show auto-update confirmation dialog */
static void show_auto_update_dialog(const char* current, const char* new_version) {
    char msg[256];
    snprintf(msg, sizeof(msg), 
             "Firmware Update Available!\n\n"
             "Current: %s\n"
             "New: %s\n\n"
             "Update now?",
             current, new_version);
    
    strcpy(pending_update_version, new_version);
    
    tt_obj_msg_box_create("Firmware Update", msg, "Updating...", auto_update_confirm_cb);
}

static void btn_auto_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* obj = lv_event_get_target(e);
        uint16_t sel = lv_dropdown_get_selected(obj);
        bool enabled = (sel == 0); // 0 is ON, 1 is OFF
        
        auto_update_enabled = enabled;
        
        if (enabled) {
            // Start checking every 60 seconds (60000 ms)
            timer_auto_check = lv_timer_create(timer_auto_check_cb, 10000, NULL);
            LV_LOG_USER("Auto-update: ENABLED - checking every 60 seconds");
        } else {
            // Stop checking
            if (timer_auto_check != NULL) {
                lv_timer_del(timer_auto_check);
                timer_auto_check = NULL;
            }
            LV_LOG_USER("Auto-update: DISABLED");
            
            // IMPORTANT: Clear any pending firmware update when user disables auto-update
            // This ensures that disabling the feature immediately stops showing update alerts
            if (remote_update_pending) {
                remote_update_pending = false;
                memset(remote_update_filename, 0, sizeof(remote_update_filename));
                LV_LOG_USER("Auto-update disabled: Cleared pending update");
            }
            // Remove the pending file  
            unlink(REMOTE_UPDATE_PENDING_FILE);
            unlink(REMOTE_UPDATE_DECLINED_FILE);
        }
        
        // Persist to backend config
        controller_set_auto_update(enabled);
    }
}

static void txt_server_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* txt = lv_event_get_target(e);
    
    if (code == LV_EVENT_CLICKED) {
        // Open keyboard with IP address type
        lv_obj_t* kb = scr_keyboard_create(lv_scr_act(), txt, KB_IPADDR);
        lv_scr_load(kb);
    } else if (code == LV_EVENT_READY) {
        // Keyboard done - save the new server address
        const char* server = lv_textarea_get_text(txt);
        if (server && strlen(server) > 0) {
            LV_LOG_USER("Update server changed to: %s", server);
            // Send to backend
            controller_set_update_server(server);
            // Show confirmation
            char msg[256];
            snprintf(msg, sizeof(msg), "Server configured: %s", server);
            tt_obj_info_box_create("Update Server", msg, 0);
        }
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