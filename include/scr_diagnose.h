#ifndef SCR_DIAGNOSE_H
#define SCR_DIAGNOSE_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the diagnose screen.
 *
 * @param[in] menu      Pointer to menu.
 */
// TODO: return diagnose page
lv_obj_t* scr_diagnose_create(lv_obj_t* menu);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_DIAGNOSE_H */
