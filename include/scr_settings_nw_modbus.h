#ifndef SCR_SETTINGS_NW_MODBUS_H
#define SCR_SETTINGS_NW_MODBUS_H

#include "lvgl/lvgl.h"

/**
 * @brief Create Modbus settings sub-page
 * @param menu LVGL menu object
 * @param btn LVGL button object for back navigation
 */
void scr_settings_nw_modbus_create(lv_obj_t* menu, lv_obj_t* btn);

#endif /* SCR_SETTINGS_NW_MODBUS_H */
