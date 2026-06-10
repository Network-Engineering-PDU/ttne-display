#ifndef SCR_SPLASH_H
#define SCR_SPLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the splash screen.
 *
 * @param[in] menu_scr   Main menu screen.
 * @param[in] login_scr  Login screen shown when password is required.
 *
 * @return Splash screen object.
 */
lv_obj_t* scr_splash_create(lv_obj_t* menu_scr, lv_obj_t* login_scr);

/**
 * @brief Called after a successful login to update splash navigation.
 */
void scr_splash_on_login_success(void);

/**
 * @brief Show the splash screen.
 */
void scr_splash_show();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_SPLASH_H */