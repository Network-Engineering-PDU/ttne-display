#include "lvgl/lvgl.h"

#include "scr_keyboard.h"
#include "tt_keyboard.h"
#include "tt_obj.h"
#include "screen.h"

/* Global variables ***********************************************************/

/* Function prototypes ********************************************************/

static void txt_event_cb(lv_event_t* e);
static void scr_keyboard_constructor(const lv_obj_class_t* class_p,
lv_obj_t* obj);

const lv_obj_class_t scr_keyboard_class = {
	.constructor_cb = scr_keyboard_constructor,
	.width_def = LV_PCT(100),
	.height_def = LV_PCT(50),
	.instance_size = sizeof(scr_keyboard_t),
	.editable = 1,
	.base_class = &lv_obj_class
};


/* Callbacks ******************************************************************/

static void txt_event_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* txt = lv_event_get_target(e);
	scr_keyboard_t* keyboard = (scr_keyboard_t*)lv_obj_get_parent(txt);

	if (code == LV_EVENT_READY) {
		lv_textarea_set_text(keyboard->prev_txt,
				lv_textarea_get_text(txt));
	}
	if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
		lv_scr_load(keyboard->prev_scr);
		lv_obj_del(lv_obj_get_parent(txt));
		lv_event_send(keyboard->prev_txt, code, NULL);
	}
}

/* Function definitions *******************************************************/

static void scr_keyboard_constructor(const lv_obj_class_t* class_p,
		lv_obj_t* obj)
{
	LV_UNUSED(class_p);

	scr_keyboard_t* keyboard = (scr_keyboard_t*)obj;
	keyboard->prev_txt = NULL;
	keyboard->prev_scr = NULL;
}

/* Public functions ***********************************************************/

lv_obj_t* scr_keyboard_create(lv_obj_t* prev_scr, lv_obj_t* prev_txt,
		keyboard_type_t type)
{
	lv_obj_t* obj = lv_obj_class_create_obj(&scr_keyboard_class, NULL);
	lv_obj_class_init_obj(obj);
	scr_keyboard_t* keyboard = (scr_keyboard_t*)obj;
	keyboard->prev_txt = prev_txt;
	keyboard->prev_scr = prev_scr;

	lv_obj_t* kb = tt_keyboard_create(obj);

	if (type == KB_NUM) {
		lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
	}
	lv_obj_t* txt_kb = lv_textarea_create(obj);
	lv_obj_align(txt_kb, LV_ALIGN_TOP_MID, 0, 10);
	lv_textarea_set_placeholder_text(txt_kb,
			lv_textarea_get_placeholder_text(prev_txt));
	lv_textarea_set_text(txt_kb, lv_textarea_get_text(prev_txt));
	lv_textarea_set_password_mode(txt_kb,
			lv_textarea_get_password_mode(prev_txt));
	lv_textarea_set_one_line(txt_kb, lv_textarea_get_one_line(prev_txt));
	if (screen_is_landscape()) {
		lv_obj_set_style_height(kb, 180, 0);
		lv_obj_set_size(txt_kb, lv_pct(95), 40);
	} else {
		lv_obj_set_size(txt_kb, lv_pct(95), 100);
		lv_obj_set_style_height(kb, 200, 0);
	}
	lv_obj_add_event_cb(txt_kb, txt_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_move_foreground(kb);
	lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

	lv_keyboard_set_textarea(kb, txt_kb);
	lv_indev_reset(NULL, txt_kb);

	return obj;
}
