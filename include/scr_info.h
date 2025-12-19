#ifndef SCR_INFO_H
#define SCR_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the info screen.
 *
 * @param[in] menu      Pointer to menu.
 * @param[in] btn       Main menu page button.
 */
void scr_info_create(lv_obj_t* menu, lv_obj_t* btn);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_INFO_H */