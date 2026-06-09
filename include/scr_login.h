#ifndef SCR_LOGIN_H
#define SCR_LOGIN_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the login screen.
 *
 * @param[in] menu      Pointer to the menu.
 * @param[in] btn       Main menu page button.
 */
void scr_login_create();

lv_obj_t* scr_login_get_page(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_LOGIN_H */
