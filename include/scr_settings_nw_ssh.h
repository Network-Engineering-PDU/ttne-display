#ifndef SCR_SETTINGS_NW_SSH_H
#define SCR_SETTINGS_NW_SSH_H

#include "lvgl/lvgl.h"

/**
 * @brief Create SSH settings sub-page
 * @param menu LVGL menu object
 * @param btn LVGL button object for back navigation
 */
void scr_settings_nw_ssh_create(lv_obj_t* menu, lv_obj_t* btn);

#endif /* SCR_SETTINGS_NW_SSH_H */
