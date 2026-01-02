#ifndef SCR_POWER_H
#define SCR_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the power screen.
 *
 * @param[in] menu      Pointer to menu.
 * @param[in] btn       Main menu page button.
 */
void scr_power_create(lv_obj_t* menu, lv_obj_t* btn);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_POWER_H */