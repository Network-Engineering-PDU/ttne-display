#ifndef TT_OBJ_H
#define TT_OBJ_H

#include "lvgl/lvgl.h"
#include "alarms.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a button.
 *
 * @param[in] parent    Pointer to the parent object.
 * @param[in] cb        Callback function to be call when pressed.
 * @param[in] lbl       Button label.
 * @param[in] img_path  Button image path. NULL if no image.
 * @param[in] x         Button width.
 * @param[in] y         Button heigth.
 * @param[in] lbl_align Align of the button label.
 */
lv_obj_t* tt_obj_btn_create(lv_obj_t* parent, lv_event_cb_t cb, const char* lbl,
		char* img_path, int x, int y, int lbl_align);

/**
 * @brief Creates a button of a matrix.
 *
 * @param[in] parent    Pointer to the parent object.
 * @param[in] cb        Callback function to be call when pressed.
 * @param[in] lbl       Button label.
 * @param[in] img_path  Button image path. NULL if no image.
 */
lv_obj_t* tt_obj_btn_mtx_create(lv_obj_t* parent, lv_event_cb_t cb, char* lbl,
		char* img_path);

/**
 * @brief Creates a card with an image and a text.
 *
 * @param[in] parent    Pointer to the parent object.
 * @param[in] lbl       Card label.
 * @param[in] img_path  Card image path. NULL if no image.
 */
lv_obj_t* tt_obj_card_create(lv_obj_t* parent, char* lbl, char* img_path);

/**
 * @brief Change the text of the label of a card.
 *
 * @param[in] card      Pointer to the card object.
 * @param[in] txt       New text.
 */
lv_obj_t* tt_obj_card_set_text(lv_obj_t* card, const char* txt);

/**
 * @brief Change the image of a card.
 *
 * @param[in] card      Pointer to the card object.
 * @param[in] img_path  New image path.
 */
lv_obj_t* tt_obj_card_set_img(lv_obj_t* card, char* img_path);

/**
 * @brief Creates a standard button (centered text, width of the parent).
 *
 * @param[in] parent    Pointer to the parent object.
 * @param[in] cb        Callback function to be call when pressed.
 * @param[in] lbl       Button label.
 */
lv_obj_t* tt_obj_btn_std_create(lv_obj_t* parent, lv_event_cb_t cb, char* lbl);

/**
 * @brief Creates a toggle button.
 *
 * @param[in] parent    Pointer to the parent object.
 * @param[in] cb        Callback function to be call when pressed.
 * @param[in] lbl       Button label.
 */
lv_obj_t* tt_obj_btn_toggle_create(lv_obj_t* parent, lv_event_cb_t cb,
		char* lbl);

/**
 * @brief Sets toggle button state.
 *
 * @param[in] btn       Pointer to the toggle button.
 * @param[in] state     Desired state to set.
 */
void tt_obj_btn_toggle_set_state(lv_obj_t* btn, bool state);

/**
 * @brief Creates a toggle button with certain width in percentage.
 *
 * @param[in] parent     Pointer to the parent object.
 * @param[in] cb         Callback function to be call when pressed.
 * @param[in] lbl        Button label.
 * @param[in] percentage Button width in percentage.
 */
lv_obj_t* tt_obj_btn_toggle_perc_create(lv_obj_t* parent, lv_event_cb_t cb,
		char* lbl, int percentage);

/**
 * @brief Creates a button with certain width in percentage.
 *
 * @param[in] parent     Pointer to the parent object.
 * @param[in] cb         Callback function to be call when pressed.
 * @param[in] lbl        Button label.
 * @param[in] percentage Button width in percentage.
 */
lv_obj_t* tt_obj_btn_perc_create(lv_obj_t* parent, lv_event_cb_t cb,
		char* lbl, int percentage);

/**
 * @brief Change the text of a button.
 *
 * @param[in] btn        Button to change text
 * @param[in] lbl        New label text.
 */
lv_obj_t* tt_obj_btn_set_text(lv_obj_t* btn, const char* txt);

/**
 * @brief Creates a label.
 *
 * @param[in] parent    Pointer to the parent object.
 * @param[in] txt       Label text.
 */
lv_obj_t* tt_obj_label_create(lv_obj_t* parent, const char* txt);

/**
 * @brief Creates a label which color can be set
 *
 * @param[in] parent    Pointer to the parent object.
 * @param[in] txt       Label text.
 */
lv_obj_t* tt_obj_label_color_create(lv_obj_t* parent, char* txt);

/**
 * @brief Creates a menu page.
 * @note Is possible cb is called twice when header is clicked
 *
 * @param[in] menu      Pointer to menu.
 * @param[in] btn       Main menu page button.
 * @param[in] cb        Event callback.
 * @param[in] name      Menu page name.
 */
lv_obj_t* tt_obj_menu_page_create(lv_obj_t* menu, lv_obj_t* btn,
		lv_event_cb_t cb, char* name);
/**
 * @brief Creates container.
 *
 * @param[in] parent    Pointer to the parent object.
 */
lv_obj_t* tt_obj_cont_create(lv_obj_t* parent);

/**
 * @brief Creates an alarm container.
 *
 * @param[in] parent    Pointer to the parent object.
 * @param[in] cb        Callback function to be call when pressed.
 * @param[in] alarm     Pointer to alarm description struct.
 */
lv_obj_t* tt_obj_cont_alarm_create(lv_obj_t* parent, lv_event_cb_t cb,
		alarm_desc_t* alarm);

/**
 * @brief Creates a message box.
 *
 * @param[in] title     Title of the message box.
 * @param[in] msg       Text of the message box.
 * @param[in] txt       Text in spinner passed as user data. //TODO
 * @param[in] cb        Callback function to be call when pressed.
 */
lv_obj_t* tt_obj_msg_box_create(char* title, char* msg, char* txt,
		lv_event_cb_t cb);

/**
 * @brief Creates a info box.
 *
 * @param[in] title     Title of the info box.
 * @param[in] msg       Text of the info box.
 * @param[in] severity  Message severity: 0=INFO; 1=ERROR
 */
lv_obj_t* tt_obj_info_box_create(char* title, char* msg, int severiry);

/**
 * @brief Creates a little spinner with the text on its right.
 *
 * @param[in] scr       Screen in which spinner is shown.
 * @param[in] msg       Message below spinner.
 */
lv_obj_t* tt_obj_spinner_inline_create(lv_obj_t* scr, const char* msg);

/**
 * @brief Creates a spinner.
 *
 * @param[in] scr       Screen in which spinner is shown.
 * @param[in] msg       Message below spinner.
 */
lv_obj_t* tt_obj_spinner_create(lv_obj_t* scr, const char* msg);

/**
 * @brief Creates a text area.
 *
 * @param[in] parent      Pointer to the parent object.
 * @param[in] placeholder Guide text to be desplayed when empty.
 * @param[in] cb          Callback function to be call when selected.
 */
lv_obj_t* tt_obj_txt_create(lv_obj_t* parent, char* placeholder,
		lv_event_cb_t cb);

/**
 * @brief Creates a dropdown.
 *
 * @param[in] parent    Pointer to the parent object.
 * @param[in] options   String with all options splitted with '\n'.
 * @param[in] cb        Callback function to be call when selected.
 */
lv_obj_t* tt_obj_dropdown_create(lv_obj_t* parent, char* options,
		lv_event_cb_t cb);

/**
 * @brief Creates a checkbox.
 *
 * @param[in] parent    Pointer to the parent object.
 * @param[in] txt       String text to show after checkbox.
 * @param[in] cb        Callback function to be call when checked/unchecked.
 */
lv_obj_t* tt_obj_checkbox_create(lv_obj_t* parent, char* txt, lv_event_cb_t cb);

/**
 * @brief Creates a loader screen.
 *
 * @param[in] msg       Message to show in loader screen.
 * @param[in] cancel_cb Cancel button callback.
 */
lv_obj_t* tt_obj_loader_create(const char* msg, lv_event_cb_t cancel_cb);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TT_OBJ_H */
