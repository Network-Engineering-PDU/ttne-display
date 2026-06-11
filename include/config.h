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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CONFIG_H */
