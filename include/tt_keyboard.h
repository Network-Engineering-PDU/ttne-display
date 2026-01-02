#ifndef TT_KEYBOARD_H
#define TT_KEYBOARD_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	KB_ABC = 0,
	KB_NUM = 1,
} keyboard_type_t;

lv_obj_t* tt_keyboard_create(lv_obj_t* parent);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TT_KEYBOARD_H */
