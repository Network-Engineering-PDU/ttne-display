#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "scr_current.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "models.h"

static lv_obj_t* current_btns[6];
static const int rated_currents[6] = {10, 15, 16, 20, 30, 32};
static const char* rated_current_labels[6] = {
    "10 A",
    "15 A",
    "16 A",
    "20 A",
    "30 A",
    "32 A",
};

static void select_current_button(int current)
{
    for (int i = 0; i < 6; ++i) {
        if (current_btns[i] == NULL) {
            continue;
        }

        if (rated_currents[i] == current) {
            lv_obj_add_state(current_btns[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(current_btns[i], LV_STATE_CHECKED);
        }
    }
}

static void current_btn_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    lv_obj_t* btn = lv_event_get_current_target(e);
    const int* current_ptr = lv_event_get_user_data(e);
    int current = current_ptr ? *current_ptr : 0;

    const models_pdu_info_t* pdu_info = models_get_pdu_info();
    models_pdu_info_t new_info = *pdu_info;
    new_info.rated_current = current;
    models_set_pdu_info(&new_info);

    select_current_button(current);

    char msg[64];
    snprintf(msg, sizeof(msg), "Rated current set to %d A", current);
    tt_obj_info_box_create("Rated Current", msg, 0);
}

void scr_current_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* current_page = tt_obj_menu_page_create(menu, btn, NULL,
            "Rated Current");

    lv_obj_set_flex_flow(current_page, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(current_page,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(current_page, 15, 0);
    lv_obj_set_style_pad_column(current_page, 15, 0);
    lv_obj_set_style_pad_all(current_page, 10, 0);

    const models_pdu_info_t* pdu_info = models_get_pdu_info();
    int selected_current = pdu_info ? pdu_info->rated_current : 0;

    for (int i = 0; i < 6; ++i) {
        lv_obj_t* current_btn = tt_obj_btn_mtx_create(current_page,
                NULL, (char*)rated_current_labels[i], NULL);
        lv_obj_add_flag(current_btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_style(current_btn, &btn_press_style, LV_STATE_CHECKED);
        current_btns[i] = current_btn;
        lv_obj_add_event_cb(current_btn, current_btn_cb, LV_EVENT_CLICKED,
                (void*)&rated_currents[i]);

        if (rated_currents[i] == selected_current) {
            lv_obj_add_state(current_btn, LV_STATE_CHECKED);
        }
    }
}
