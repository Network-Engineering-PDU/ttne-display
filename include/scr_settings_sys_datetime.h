#ifndef SCR_SETTINGS_SYS_DATETIME_H
#define SCR_SETTINGS_SYS_DATETIME_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the Date & Time page of System Setup.
 *
 * @param[in] menu  Pointer to the menu.
 * @param[in] btn   System Setup button that opens the page.
 */
void scr_settings_sys_datetime_create(lv_obj_t* menu, lv_obj_t* btn);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_SETTINGS_SYS_DATETIME_H */
