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
 */
void scr_splash_show();

/**
 * @brief Update the splash screen's previous target screen.
 */
void scr_splash_set_prev(lv_obj_t* prev_scr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_SPLASH_H */