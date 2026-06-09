#ifndef TTNE_DISPLAY_H
#define TTNE_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

void ttne_display_reset();

void ttne_display(void);

void ttne_display_idle_cb();

void ttne_menu_display();

/**
 * @brief Get the current main menu page object.
 */
lv_obj_t* ttne_get_main_page();
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*TTNE_DISPLAY_H*/
