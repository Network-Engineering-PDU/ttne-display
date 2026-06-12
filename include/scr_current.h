#ifndef SCR_CURRENT_H
#define SCR_CURRENT_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the rated current selection screen.
 *
 * @param[in] menu      Pointer to the menu.
 * @param[in] btn       Main menu page button.
 */
void scr_current_create(lv_obj_t* menu, lv_obj_t* btn);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_CURRENT_H */
