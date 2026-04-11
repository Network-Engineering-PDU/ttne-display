#ifndef SCR_SETTINGS_NW_SNMP_H
#define SCR_SETTINGS_NW_SNMP_H

#include "lvgl/lvgl.h"

/**
 * @brief Create SNMP settings sub-page
 * @param menu LVGL menu object
 * @param btn LVGL button object for back navigation
 * @return Pointer to the created page object
 */
lv_obj_t* scr_settings_nw_snmp_create(lv_obj_t* menu, lv_obj_t* btn);

#endif /* SCR_SETTINGS_NW_SNMP_H */
