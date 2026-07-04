#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "scr_current.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "app/app_state.h"
#include "backend/backend.h"
#include "screen.h"

#ifdef UI_DEBUG_LOGS
#define UI_DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define UI_DEBUG_PRINTF(...) ((void)0)
#endif

/* Global variables ***********************************************************/
static lv_obj_t* menu;

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

static void rated_current_cb(int err, void* userdata)
{
	(void)userdata;
	if (err != 0) {
		tt_obj_info_box_create("Rated Current",
				"Could not save rated current", 1);
	}
}

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

    const int* current_ptr = lv_event_get_user_data(e);
    int current = current_ptr ? *current_ptr : 0;

    UI_DEBUG_PRINTF("[scr_current] Setting rated current to %d A\n", current);
    backend_pdu_set_rated_current(current, rated_current_cb, NULL);

    select_current_button(current);

    char msg[64];
    snprintf(msg, sizeof(msg), "Rated current set to %d A", current);
    tt_obj_info_box_create("Rated Current", msg, 0);
}

void scr_current_create(lv_obj_t* l_menu, lv_obj_t* btn)
{
    menu = l_menu;

    /* Create the Rated Current page directly */
    lv_obj_t* current_page = tt_obj_menu_page_create(menu, btn, NULL,"Rated Current");

    /* Apply Flex layout directly to the page. 
       In LVGL, menu pages usually have a 'scrollable' part or are objects themselves 
       that can act as containers.
    */
    lv_obj_set_flex_flow(current_page, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(
        current_page,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    app_state_snapshot_t snapshot;
    app_state_get_snapshot(&snapshot);
    int selected_current = snapshot.pdu_info.valid ?
            snapshot.pdu_info.rated_current : 0;

    /* Create 2x3 grid buttons with fixed 31% width to force 3 columns */
    for (int i = 0; i < 6; ++i) {
        lv_obj_t* btn = tt_obj_btn_create(current_page, NULL,
                (char*)rated_current_labels[i], NULL,
                LV_PCT(31), 88, LV_ALIGN_CENTER);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_style(btn, &btn_press_style, LV_STATE_CHECKED);
        current_btns[i] = btn;
        lv_obj_add_event_cb(btn, current_btn_cb, LV_EVENT_CLICKED,
                (void*)&rated_currents[i]);

        if (rated_currents[i] == selected_current) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        }
    }
}
