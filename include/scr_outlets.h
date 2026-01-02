#ifndef SCR_OUTLETS_H
#define SCR_OUTLETS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the outlets screen.
 *
 * @param[in] menu      Pointer to menu.
 * @param[in] btn       Main menu page button.
 */
void scr_outlets_create(lv_obj_t* menu, lv_obj_t* btn);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_OUTLETS_H */