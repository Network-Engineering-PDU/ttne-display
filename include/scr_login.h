/**
 * @file scr_login.h
 * @brief Password login screen implementation using LVGL.
 * Includes password entry text area and a "Don't Request Again" checkbox.
 */

#ifndef SCR_LOGIN_H
#define SCR_LOGIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 * INCLUDES
 *********************/
#include "lvgl/lvgl.h"

/*********************
 * PUBLIC FUNCTIONS
 *********************/

/**
 * @brief Create and initialize the PDU login screen.
 * Clears the current active screen and sets up the password prompt UI.
 */
void scr_login_create(void);

/**
 * @brief Check if the "Don't Request Password again" checkbox is checked.
 * @return true if checked, false otherwise.
 */
bool scr_login_get_dont_request_again_status(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SCR_LOGIN_H*/