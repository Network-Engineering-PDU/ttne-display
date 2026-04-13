#ifndef SCR_SETTINGS_MENU_H
#define SCR_SETTINGS_MENU_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the settings menu screen.
 *
 * @param[in] menu      Pointer to menu.
 * @param[in] btn       Main menu page button.
 */
void scr_settings_menu_create(lv_obj_t* menu, lv_obj_t* btn);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_SETTINGS_MENU_H */
