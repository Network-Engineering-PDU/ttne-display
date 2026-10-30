#ifndef TT_KEYBOARD_H
#define TT_KEYBOARD_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	KB_ABC = 0,	/* text in the language's letters */
	KB_NUM = 1,	/* number pad */
	KB_PASS = 2,	/* PIN pad */
	KB_LATIN = 3,	/* Latin letters first: passwords, names, server addresses */
} keyboard_type_t;

/**
 * @brief Creates a keyboard with the layouts of the current language (see
 * tt_keyboard.c). For Chinese it also creates the Pinyin input method, whose
 * candidate bar goes above the keyboard: call tt_keyboard_place_ime() once
 * the keyboard is placed.
 */
lv_obj_t* tt_keyboard_create(lv_obj_t* parent);

/* Shows the layout for a kind of field. */
void tt_keyboard_set_type(lv_obj_t* kb, keyboard_type_t type);

/* Puts the Chinese candidate bar on top of the keyboard. */
void tt_keyboard_place_ime(lv_obj_t* kb);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TT_KEYBOARD_H */
