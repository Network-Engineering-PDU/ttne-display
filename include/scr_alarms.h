#ifndef SCR_ALARMS_H
#define SCR_ALARMS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the alarms screen.
 *
 * @param[in] menu      Pointer to menu.
 * @param[in] btn       Main menu page button.
 */
void scr_alarms_create(lv_obj_t* menu, lv_obj_t* btn);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_ALARMS_H */