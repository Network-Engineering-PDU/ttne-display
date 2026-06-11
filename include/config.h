#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize config module.
 */
void config_init();

/**
 * @brief Set the rotation of the display.
 *
 * @param[in] rotation  Rotation value [0->0; 1->90; 2->180; 3->270].
 */
void config_set_rotation(int rotation);

/**
 * @brief Get the rotation of the display.
 *
 * @return Rotation value [0->0; 1->90; 2->180; 3->270].
 */
int config_get_rotation();

/**
 * @brief Set the inactivity time for the screensaver.
 *
 * @param[in] inactivity_time  inactivity time value in minutes.
 */
void config_set_inactivity_time(int inactivity_time);

/**
 * @brief Get the inactivity time for the screensaver.
 *
 * @return inactivity time value in minutes.
 */
int config_get_inactivity_time();

/**
 * @brief Set whether login should be skipped on future boots.
 *
 * @param[in] skip_login  1 to skip login, 0 to require login.
 */
void config_set_skip_login(int skip_login);

/**
 * @brief Get whether login should be skipped on future boots.
 *
 * @return 1 if login should be skipped, 0 otherwise.
 */
int config_get_skip_login();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CONFIG_H */
