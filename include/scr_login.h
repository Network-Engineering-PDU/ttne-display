#ifndef SCR_LOGIN_H
#define SCR_LOGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "lvgl/lvgl.h"

/**
 * @brief Creates the login screen.
 *
 * @param[in] main_menu_scr Screen to open after login succeeds.
 *
 * @return Login screen object.
 */
lv_obj_t* scr_login_create(lv_obj_t* main_menu_scr);

/**
 * @brief Returns true when the user must authenticate before opening the menu.
 */
bool scr_login_is_required();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_LOGIN_H */
