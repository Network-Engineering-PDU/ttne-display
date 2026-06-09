#ifndef SCR_SPLASH_H
#define SCR_SPLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the splash screen.
 * 
 * @param[in] prev_screen Pointer to revious screen
 * 
 * @return Splash screen object
 */
lv_obj_t* scr_splash_create(lv_obj_t* prev_scr);

/**
 * @brief Show the splash screen.
 *
 * @param force If true, force loading the splash screen even when already active.
 */
void scr_splash_show(bool force);

/**
 * @brief Update the splash screen's previous target screen.
 */
void scr_splash_set_prev(lv_obj_t* prev_scr);

/**
 * @brief Notify the splash screen that login has been completed.
 */
void scr_splash_login_completed(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_SPLASH_H */