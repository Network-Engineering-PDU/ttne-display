#ifndef TT_STATUS_BAR_H
#define TT_STATUS_BAR_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Adds the time and the Wi-Fi signal to the end of the menu's top
 * bar, so every page shows them. Loads the date and time settings, keeps
 * the time current and asks for the Wi-Fi signal every few seconds.
 *
 * @param[in] header  The menu's main header (lv_menu_get_main_header()).
 */
void tt_status_bar_create(lv_obj_t* header);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TT_STATUS_BAR_H */
