#ifndef SCR_SENSORS_H
#define SCR_SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the sensors screen.
 *
 * @param[in] menu      Pointer to menu.
 * @param[in] btn       Main menu page button.
 */
void scr_sensors_create(lv_obj_t* menu, lv_obj_t* btn);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_SENSORS_H */