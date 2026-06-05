/**
 * @file scr_login.c
 * @brief Password login screen implementation using LVGL.
 */

/*********************
 * INCLUDES
 *********************/
#include "scr_login.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

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

/*********************
 * STATIC VARIABLES
 *********************/
static lv_obj_t * ta_password = NULL;
static lv_obj_t * cb_remember = NULL;

/* Path for storing credentials (per-user in HOME or /tmp fallback) */
#define CRED_FILENAME ".ttne_credentials"
#define CRED_PATH_FALLBACK "/tmp/.ttne_credentials"

/* Default password to prefill when no stored credentials exist. Change as needed. */
#define DEFAULT_PASSWORD "1234"

/* If set to 1, saved credentials will be cleared on every startup (reboot/reset).
 * This forces the UI to fall back to `DEFAULT_PASSWORD` after a system restart.
 * Set to 0 to preserve saved credentials across reboots (default behavior).
 */
#ifndef CLEAR_CREDENTIALS_ON_BOOT
#define CLEAR_CREDENTIALS_ON_BOOT 0
#endif

/* NOTE: This implementation stores the password in plain text.
 * For production use on devices, consider encrypting the file,
 * storing in secure NVS/Flash, or using a hardware-backed keystore.
 */

static void get_credentials_path(char *buf, size_t len) {
    const char *home = getenv("HOME");
    if(home && *home) {
        snprintf(buf, len, "%s/%s", home, CRED_FILENAME);
    } else {
        strncpy(buf, CRED_PATH_FALLBACK, len-1);
        buf[len-1] = '\0';
    }
}

static bool save_credentials_to_file(const char *pwd, bool remember) {
    char path[256];
    get_credentials_path(path, sizeof(path));

    if(!remember) {
        /* Remove existing file if present */
        remove(path);
        return true;
    }

    FILE *f = fopen(path, "w");
    if(!f) {
        LV_LOG_USER("Failed to open credentials file %s: %s", path, strerror(errno));
        return false;
    }
    fprintf(f, "password=%s\nremember=1\n", pwd ? pwd : "");
    fclose(f);
    /* Restrict permissions to user read/write */
    chmod(path, S_IRUSR | S_IWUSR);
    return true;
}

static bool load_credentials_from_file(char *out_pwd, size_t pwd_len, bool *out_remember) {
    char path[256];
    get_credentials_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if(!f) return false;

    char line[256];
    bool have_pwd = false;
    bool remember = false;
    while(fgets(line, sizeof(line), f)) {
        if(strncmp(line, "password=", 9) == 0) {
            char *pv = line + 9;
            size_t ln = strlen(pv);
            if(ln && pv[ln-1] == '\n') pv[ln-1] = '\0';
            strncpy(out_pwd, pv, pwd_len-1);
            out_pwd[pwd_len-1] = '\0';
            have_pwd = true;
        } else if(strncmp(line, "remember=", 9) == 0) {
            remember = (atoi(line + 9) != 0);
        }
    }
    fclose(f);
    if(out_remember) *out_remember = remember;
    return have_pwd || remember;
}

/*********************
 * STATIC FUNCTIONS
 *********************/

/**
 * @brief Event callback function for the password text area.
 * @param e Pointer to the event structure.
 */
static void password_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if(code == LV_EVENT_READY) {
        const char * pwd = lv_textarea_get_text(ta);
        
        /* * TODO: Add your PDU firmware authentication logic here.
         * e.g., if (strcmp(pwd, "1234") == 0) { ... }
         */
        LV_LOG_USER("Password submitted: %s", pwd);
        
        /* Persist credentials if checkbox is checked, otherwise remove stored creds */
        if(scr_login_get_dont_request_again_status()) {
            save_credentials_to_file(pwd, true);
            LV_LOG_USER("Saved credentials to file (remember=1)");
        } else {
            save_credentials_to_file(NULL, false);
            LV_LOG_USER("Cleared saved credentials");
        }
    }
}

static void login_button_event_cb(lv_event_t * e) {
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    if(ta_password) {
        const char * pwd = lv_textarea_get_text(ta_password);
        LV_LOG_USER("Login button clicked. Password: %s", pwd);

        if(scr_login_get_dont_request_again_status()) {
            save_credentials_to_file(pwd, true);
            LV_LOG_USER("Saved credentials to file (remember=1)");
        } else {
            save_credentials_to_file(NULL, false);
            LV_LOG_USER("Cleared saved credentials");
        }
    }
}

/*********************
 * PUBLIC FUNCTIONS
 *********************/

void scr_login_create(lv_obj_t* l_menu, lv_obj_t* btn) {
    /* 1. Create a login page in the menu */
    lv_obj_t * login_page = tt_obj_menu_page_create(l_menu, btn, NULL, "Login");

    /* Optionally clear saved credentials at startup to enforce default password */
    if(CLEAR_CREDENTIALS_ON_BOOT) {
        char path[256];
        get_credentials_path(path, sizeof(path));
        remove(path);
        LV_LOG_USER("CLEAR_CREDENTIALS_ON_BOOT set — removed %s", path);
    }

    /* Set white background and center all elements */
    lv_obj_set_style_bg_color(login_page, lv_color_white(), 0);
    lv_obj_set_flex_flow(login_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(login_page, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(login_page, 18, 0);

    /* 2. "Enter Password" Prompt Label */
    lv_obj_t * lbl_prompt = lv_label_create(login_page);
    lv_label_set_text(lbl_prompt, "Enter Password");
    lv_obj_set_style_text_font(lbl_prompt, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(lbl_prompt, LV_TEXT_ALIGN_CENTER, 0);

    /* 3. Password Input Text Area Box */
    ta_password = lv_textarea_create(login_page);
    lv_textarea_set_password_mode(ta_password, true);  /* Hide password with glyphs '*' */
    lv_textarea_set_one_line(ta_password, true);        /* Keep to a clean single line input */
    lv_textarea_set_placeholder_text(ta_password, "Password");
    
    /* Size optimized for standard landscape PDU micro-displays */
    lv_obj_set_width(ta_password, 240);
    lv_obj_set_height(ta_password, 38);

    /* Style the textarea to be rounded and slightly raised */
    lv_obj_set_style_radius(ta_password, 6, 0);
    lv_obj_set_style_bg_color(ta_password, lv_color_hex(0xF1F7FB), 0);
    lv_obj_set_style_border_width(ta_password, 0, 0);
    lv_obj_set_style_shadow_width(ta_password, 8, 0);
    lv_obj_set_style_shadow_ofs_y(ta_password, 4, 0);
    lv_obj_set_style_shadow_color(ta_password, lv_color_hex(0xA8B4C6), 0);

    /* Assign action event when enter/ready is pressed on keyboard/keypad */
    lv_obj_add_event_cb(ta_password, password_event_cb, LV_EVENT_READY, NULL);

    /* 4. "Don't Request Password again" Checkbox */
    cb_remember = lv_checkbox_create(login_page);
    lv_checkbox_set_text(cb_remember, "Don't request password again");
    lv_obj_set_style_text_font(cb_remember, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(cb_remember, LV_TEXT_ALIGN_CENTER, 0);

    /* Load stored credentials if present */
    char saved_pwd[128] = {0};
    bool saved_remember = false;
    if(load_credentials_from_file(saved_pwd, sizeof(saved_pwd), &saved_remember)) {
        if(saved_remember) {
            /* Pre-fill password and check the box */
            lv_textarea_set_text(ta_password, saved_pwd);
            lv_obj_add_state(cb_remember, LV_STATE_CHECKED);
        } else {
            /* No remembered credential, prefill with default password */
            lv_textarea_set_text(ta_password, DEFAULT_PASSWORD);
        }
    } else {
        /* No credentials file found — prefill default password */
        lv_textarea_set_text(ta_password, DEFAULT_PASSWORD);
    }

    /* 5. Add LOG IN button */
    lv_obj_t * btn_login = lv_btn_create(login_page);
    lv_obj_set_width(btn_login, 100);
    lv_obj_set_height(btn_login, 36);
    lv_obj_add_event_cb(btn_login, login_button_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_btn = lv_label_create(btn_login);
    lv_label_set_text(lbl_btn, "LOG IN");
    lv_obj_center(lbl_btn);
}

bool scr_login_get_dont_request_again_status(void) {
    if(cb_remember != NULL) {
        return lv_obj_has_state(cb_remember, LV_STATE_CHECKED);
    }
    return false;
}