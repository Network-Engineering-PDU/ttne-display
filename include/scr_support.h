#ifndef SCR_SUPPORT_H
#define SCR_SUPPORT_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the support screen.
 *
 * @param[in] menu      Pointer to menu.
 */
// TODO: return support page
lv_obj_t* scr_support_create(lv_obj_t* menu);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_SUPPORT_H */
