#ifndef SCR_KEYBOARD_H
#define SCR_KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include "tt_keyboard.h"

typedef struct {
	lv_obj_t scr;
	lv_obj_t* prev_txt;
	lv_obj_t* prev_scr;
} scr_keyboard_t;

/**
 * @brief Creates the keyboard screen.
 *
 * @param[in] menu      Pointer to menu.
 * @param[in] btn       Main menu page button.
 * @param[in] type      Keyboard type (ABC or NUM).
 */
lv_obj_t* scr_keyboard_create(lv_obj_t* prev_scr, lv_obj_t* prev_txt,
		keyboard_type_t type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_KEYBOARD_H */
