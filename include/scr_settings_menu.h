#ifndef SCR_SETTINGS_MENU_H
#define SCR_SETTINGS_MENU_H

#include "lvgl/lvgl.h"

/**
 * Creates the settings menu screen with navigation buttons
 * 
 * @param l_menu The menu object to add the settings page to
 * @param btn The button that triggered the settings menu (for back navigation)
 */
void scr_settings_menu_create(lv_obj_t* l_menu, lv_obj_t* btn);

#endif // SCR_SETTINGS_MENU_H
