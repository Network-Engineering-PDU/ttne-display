#ifndef SCR_LOGIN_H
#define SCR_LOGIN_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the login screen.
 *
 * @param[in] main_menu_scr  Screen to navigate to after successful login.
 *
 * @return Login screen object.
 */
lv_obj_t* scr_login_create(lv_obj_t* main_menu_scr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_LOGIN_H */
