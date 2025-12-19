#ifndef SCREEN_H
#define SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Initialize screen module.
 */
void screen_init();

/**
 * @brief Set the rotation of the display.
 *
 * @param[in] rotation Rotation value [0->0; 1->90; 2->180; 3->270].
 */
void screen_set_rotation(int rotation);
/**
 * @brief Return true if orientation is landscape.
 */
bool screen_is_landscape();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCREEN_H */
