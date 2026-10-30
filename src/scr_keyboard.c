#include <string.h>

#include "lvgl/lvgl.h"

#include "i18n.h"
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
	tt_keyboard_set_type(kb, type);

	lv_obj_t* txt_kb = lv_textarea_create(obj);
	lv_obj_align(txt_kb, LV_ALIGN_TOP_MID, 0, 10);
	lv_textarea_set_placeholder_text(txt_kb,
			lv_textarea_get_placeholder_text(prev_txt));
	i18n_textarea_fix_placeholder(txt_kb);
	lv_textarea_set_text(txt_kb, lv_textarea_get_text(prev_txt));
	lv_textarea_set_password_mode(txt_kb,
			lv_textarea_get_password_mode(prev_txt));
	lv_textarea_set_one_line(txt_kb, lv_textarea_get_one_line(prev_txt));
	if (type == KB_PASS) {
		lv_textarea_set_accepted_chars(txt_kb, "0123456789");
		lv_textarea_set_max_length(txt_kb, 6);
	}
	/* Addresses (IP fields) keep ASCII digits while typed too */
	if (i18n_has_western_digits(prev_txt)) {
		i18n_western_digits(txt_kb);
		i18n_western_digits(kb);
	}
	/* Chinese: room for the candidate bar between field and keyboard */
	int ime_room = strcmp(i18n_lang_code(i18n_current_lang()), "zh-hans") == 0 &&
			type != KB_NUM && type != KB_PASS ? 34 : 0;
	if (screen_is_landscape()) {
		lv_obj_set_style_height(kb, (type == KB_PASS ? 170 : 180) - ime_room, 0);
		lv_obj_set_size(txt_kb, lv_pct(95), type == KB_PASS ? 58 : 40);
	} else {
		lv_obj_set_size(txt_kb, lv_pct(95), 100);
		lv_obj_set_style_height(kb, (type == KB_PASS ? 240 : 200) - ime_room, 0);
	}
	lv_obj_add_event_cb(txt_kb, txt_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_move_foreground(kb);
	lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_update_layout(obj);
	tt_keyboard_place_ime(kb);

	lv_keyboard_set_textarea(kb, txt_kb);
	lv_indev_reset(NULL, txt_kb);

	return obj;
}
