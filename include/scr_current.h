/**
 * @file scr_current.h
 * @brief Header for current/settings network menu screen.
 */

#ifndef SCR_CURRENT_H
#define SCR_CURRENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * @brief Create the network settings menu page inside a parent menu object.
 * @param l_menu Parent `lv_menu` object pointer.
 * @param btn Optional button to attach as header/back control (may be NULL).
 */
void scr_current_create(lv_obj_t* l_menu, lv_obj_t* btn);

#ifdef __cplusplus
}
#endif

#endif /* SCR_CURRENT_H */
