#include "lvgl/lvgl.h"
#include "scr_settings_nw_ntp_sntp.h"
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

static lv_obj_t* ntp_enable_cbx;
static lv_obj_t* ntp_offset_combo;
static lv_obj_t* ntp_server_txt;

void scr_settings_nw_ntp_sntp_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* ntp_page = tt_obj_menu_page_create(menu, btn, NULL, "NTP - SNTP");
    lv_obj_t* cont = tt_obj_cont_create(ntp_page);

    /* NTP/SNTP Enable */
    tt_obj_label_create(cont, "NTP / SNTP enable");
    ntp_enable_cbx = tt_obj_checkbox_create(cont, "", NULL);

    /* Time Offset (Timezone) */
    tt_obj_label_create(cont, "Time offset");
    ntp_offset_combo = tt_obj_dropdown_create(cont, 
        "UTC-12\nUTC-11\nUTC-10\nUTC-09\nUTC-08\nUTC-07\nUTC-06\nUTC-05\nUTC-04\n"
        "UTC-03\nUTC-02\nUTC-01\nUTC+00\nUTC+01\nUTC+02\nUTC+03\nUTC+04\nUTC+05\n"
        "UTC+06\nUTC+07\nUTC+08\nUTC+09\nUTC+10\nUTC+11\nUTC+12", NULL);
    lv_dropdown_set_selected(ntp_offset_combo, 12); /* Default to UTC+00 */

    /* NTP Server */
    tt_obj_label_create(cont, "Server");
    ntp_server_txt = tt_obj_txt_create(cont, "NTP Server", NULL);

    /* OK / Cancel buttons */
    lv_obj_t* btn_row = lv_obj_create(cont);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    tt_obj_btn_std_create(btn_row, NULL, "OK");
    tt_obj_btn_std_create(btn_row, NULL, "Cancel");
}
