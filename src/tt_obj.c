#include <stdio.h>

#include "lvgl/lvgl.h"

#include "utils.h"
#include "tt_obj.h"
#include "tt_styles.h"
#include "tt_colors.h"
#include "screen.h"

/* Function prototypes ********************************************************/

static void toggle_cb(lv_event_t* e);

/* Callbacks ******************************************************************/

static void toggle_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t* state_label = lv_obj_get_child(obj, 1);
		if (lv_obj_get_state(obj) & LV_STATE_CHECKED) {
			lv_label_set_text(state_label, "ON");
		} else {
			lv_label_set_text(state_label, "OFF");
		}
	}
}

/* Public functions ***********************************************************/

lv_obj_t* tt_obj_btn_create(lv_obj_t* parent, lv_event_cb_t cb, const char* lbl,
		char* img_path, int x, int y, int lbl_align)
{
	lv_obj_t* btn = lv_obj_create(parent);
	lv_obj_set_size(btn, x, y);
	if (cb != NULL) {
		lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, NULL);
	}
	lv_obj_add_style(btn, &btn_style, LV_STATE_DEFAULT);
	lv_obj_add_style(btn, &btn_hover_style, LV_STATE_PRESSED);

	lv_obj_t* label = lv_label_create(btn);
	lv_label_set_text(label, lbl);
	if (lbl_align == LV_ALIGN_LEFT_MID) {
		lv_obj_align(label, lbl_align, 10, 0);
	} else {
		lv_obj_align(label, lbl_align, 0, 0);
	}

	if (img_path) {
		lv_obj_t* img = lv_img_create(btn);
		lv_img_set_src(img, img_path);
		lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 10);
	}

	return btn;
}

lv_obj_t* tt_obj_btn_mtx_create(lv_obj_t* parent, lv_event_cb_t cb, char* lbl,
		char* img_path)
{
	if (screen_is_landscape()) {
		return tt_obj_btn_create(parent, cb, lbl, img_path, LV_PCT(31), 88,
				LV_ALIGN_BOTTOM_MID);

	} else {
		return tt_obj_btn_create(parent, cb, lbl, img_path, LV_PCT(48), 80,
				LV_ALIGN_BOTTOM_MID);
	}
}

lv_obj_t* tt_obj_card_create(lv_obj_t* parent, char* lbl, char* img_path)
{
	lv_obj_t* card = tt_obj_btn_mtx_create(parent, NULL, lbl, img_path);
	lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
	return card;
}

lv_obj_t* tt_obj_card_set_text(lv_obj_t* card, const char* txt)
{
	lv_obj_t* lbl = lv_obj_get_child(card, 0);
	lv_label_set_text(lbl, txt);
	lv_label_set_recolor(lbl, true);
	return lbl;
}

lv_obj_t* tt_obj_card_set_img(lv_obj_t* card, char* img_path)
{
	lv_obj_t* img = lv_obj_get_child(card, 1);
	lv_img_set_src(img, img_path);
	return img;
}

lv_obj_t* tt_obj_btn_std_create(lv_obj_t* parent, lv_event_cb_t cb, char* lbl)
{
	return tt_obj_btn_create(parent, cb, lbl, NULL, LV_PCT(100), 50,
			LV_ALIGN_LEFT_MID);
}

lv_obj_t* tt_obj_btn_toggle_create(lv_obj_t* parent, lv_event_cb_t cb,
		char* lbl)
{
	lv_obj_t* btn = tt_obj_btn_create(parent, cb, lbl, NULL, LV_PCT(100), 50,
			LV_ALIGN_LEFT_MID);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_add_style(btn, &btn_hover_style, LV_STATE_PRESSED);
	lv_obj_add_style(btn, &btn_press_style, LV_STATE_CHECKED);
	lv_obj_remove_style(btn, &btn_press_style, LV_STATE_PRESSED);

	lv_obj_t* label = lv_label_create(btn);
	lv_label_set_text(label, "OFF");
	lv_obj_align(label, LV_ALIGN_RIGHT_MID, -10, 0);
	lv_obj_add_event_cb(btn, toggle_cb, LV_EVENT_ALL, NULL);

	return btn;
}

void tt_obj_btn_toggle_set_state(lv_obj_t* btn, bool state)
{
	lv_obj_t* child = lv_obj_get_child(btn, 1);
	if (state) {
		lv_obj_add_state(btn, LV_STATE_CHECKED);
		lv_label_set_text(child, "ON");
	} else {
		lv_obj_clear_state(btn, LV_STATE_CHECKED);
		lv_label_set_text(child, "OFF");
	}
}

lv_obj_t* tt_obj_btn_toggle_perc_create(lv_obj_t* parent, lv_event_cb_t cb,
		char* lbl, int percentage)
{
	lv_obj_t* btn = tt_obj_btn_create(parent, cb, lbl, NULL, LV_PCT(percentage),
			50, LV_ALIGN_TOP_MID);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_add_style(btn, &btn_press_style, LV_STATE_CHECKED);

	lv_obj_t* label = lv_label_create(btn);
	lv_label_set_text(label, "OFF");
	lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);

	return btn;
}

lv_obj_t* tt_obj_btn_perc_create(lv_obj_t* parent, lv_event_cb_t cb,
		char* lbl, int percentage)
{
	return tt_obj_btn_create(parent, cb, lbl, NULL, LV_PCT(percentage),
			50, LV_ALIGN_CENTER);
}

// TODO: not used
lv_obj_t* tt_obj_btn_set_text(lv_obj_t* btn, const char* txt)
{
	lv_obj_t* label = lv_obj_get_child(btn, 0);
	lv_label_set_text(label, txt);
	return label;
}

lv_obj_t* tt_obj_label_create(lv_obj_t* parent, const char* txt)
{
	lv_obj_t* label = lv_label_create(parent);
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(label, LV_PCT(100));
	lv_label_set_text(label, txt);
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
	// lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

	return label;
}

lv_obj_t* tt_obj_label_color_create(lv_obj_t* parent, char* txt)
{
	lv_obj_t* label = tt_obj_label_create(parent, txt);
	lv_label_set_recolor(label, true); 

	return label;
}

lv_obj_t* tt_obj_menu_page_create(lv_obj_t* menu, lv_obj_t* btn,
		lv_event_cb_t cb, char* name)
{
	lv_obj_t* page = lv_menu_page_create(menu, NULL);
	lv_menu_set_page_title_static(page, name);
	lv_obj_t* cont = lv_menu_cont_create(page);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN_WRAP);
	if (btn != NULL) {
		lv_menu_set_load_page_event(menu, btn, page);
	}
	lv_obj_add_event_cb(menu, cb, LV_EVENT_ALL, page);
	// Add menu_cb to back buton to allow right close when click on this button
	lv_obj_t* menu_header_btn = lv_menu_get_main_header_back_btn(menu);
	lv_obj_add_event_cb(menu_header_btn, cb, LV_EVENT_ALL, page);

	return cont;
}

lv_obj_t* tt_obj_cont_create(lv_obj_t* parent)
{
	lv_obj_t* cont = lv_obj_create(parent);
	lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN_WRAP);
	lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_style(cont, &cont_style, LV_STATE_DEFAULT);

	return cont;
}

lv_obj_t* tt_obj_cont_alarm_create(lv_obj_t* parent, lv_event_cb_t cb,
		alarm_desc_t* alarm)
{

	lv_obj_t* cont_parent = lv_obj_create(parent);
	lv_obj_set_size(cont_parent, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_align(cont_parent, LV_ALIGN_OUT_TOP_MID, 5, 50);
	lv_obj_add_style(cont_parent, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(cont_parent, LV_SCROLLBAR_MODE_OFF);

	lv_obj_t* cont = lv_obj_create(cont_parent);
	lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_align(cont, LV_ALIGN_OUT_TOP_MID, 0, 0);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN_WRAP);
	lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_style(cont, &alarm_cont_style, LV_STATE_DEFAULT);

	lv_obj_t* cont_img = lv_obj_create(cont_parent);
	lv_obj_add_style(cont_img, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_set_size(cont_img, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_align(cont_img, LV_ALIGN_TOP_RIGHT, -8, 8);
	lv_obj_set_scrollbar_mode(cont_img, LV_SCROLLBAR_MODE_OFF);

	lv_obj_t* img = lv_img_create(cont_img);
	lv_img_set_src(img, ASSET("close.png"));
	lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(img, cb, LV_EVENT_ALL, cont_parent);

	tt_obj_label_create(cont, alarm->time);
	char alarm_type[100];
	switch (alarm->type) {
	case ALARM_INFO: 
		sprintf(alarm_type, "#%06X Info", TT_COLOR_INFO);
		break;
	case ALARM_WARNING:
		sprintf(alarm_type, "#%06X Warning", TT_COLOR_WARNING);
		break;
	case ALARM_ERROR:
		sprintf(alarm_type, "#%06X Error", TT_COLOR_ERROR);
		break;
	case ALARM_CRITICAL_ERROR:
		sprintf(alarm_type, "#%06X Critical error", TT_COLOR_CRITICAL_ERROR);
		break;
	default:
		break;
	}
	tt_obj_label_color_create(cont, alarm_type);
	tt_obj_label_create(cont, alarm->desc);
	tt_obj_label_create(cont, alarm->path);
	tt_obj_label_create(cont, alarm->code);

	return cont_parent;
}

// TODO: fix close button
lv_obj_t* tt_obj_msg_box_create(char* title, char* msg, char* txt,
		lv_event_cb_t cb)
{
	static const char* btns[] = {"YES", "NO", ""};
	lv_obj_t* msgbox = lv_msgbox_create(NULL, title, msg, btns, false);
	lv_obj_add_event_cb(msgbox, cb, LV_EVENT_ALL, txt);
	lv_obj_center(msgbox);
	lv_obj_set_size(msgbox, LV_PCT(90), LV_SIZE_CONTENT);
	
	lv_obj_t* msgbox_txt = lv_msgbox_get_text(msgbox);
	lv_label_set_recolor(msgbox_txt, true);

	lv_obj_t* mtx_btns = lv_msgbox_get_btns(msgbox);
	lv_obj_set_height(mtx_btns, 50);

	return msgbox;
}

// TODO: fix close button; colors allowed??
lv_obj_t* tt_obj_info_box_create(char* title, char* msg, int severiry)
{
	lv_obj_t* msgbox = lv_msgbox_create(NULL, title, msg, NULL, true);
	lv_obj_center(msgbox);
	lv_obj_set_size(msgbox, LV_PCT(90), LV_SIZE_CONTENT);

	lv_obj_t* close_btn = lv_msgbox_get_close_btn(msgbox);
	if (severiry == 1) { // ERROR
		lv_obj_add_style(close_btn, &btn_err_style, LV_STATE_DEFAULT);
	} else { // INFO
		lv_obj_add_style(close_btn, &btn_hover_style, LV_STATE_DEFAULT);
	}

	return msgbox;
}

lv_obj_t* tt_obj_spinner_inline_create(lv_obj_t* scr, const char* msg)
{
	lv_obj_t* cont = lv_obj_create(scr);
	lv_obj_set_size(cont, LV_PCT(100), 30);
	lv_obj_add_style(cont, &invisible_cont_style, LV_STATE_DEFAULT);
	lv_obj_t* spinner = lv_spinner_create(cont, 2000, 100);
	lv_obj_set_size(spinner, 30, 30);
	lv_obj_add_style(spinner, &spinner_inline_style,
			LV_STATE_DEFAULT | LV_PART_INDICATOR);
	lv_obj_add_style(spinner, &spinner_inline_bg_style,
			LV_STATE_DEFAULT | LV_PART_MAIN);
	lv_obj_t* text = lv_label_create(cont);
	lv_label_set_text(text, msg);
	if (screen_is_landscape()) {
		lv_obj_align(cont, LV_ALIGN_CENTER, 0, 10);
		lv_obj_align(spinner, LV_ALIGN_LEFT_MID, 60, 0);
		lv_obj_align(text, LV_ALIGN_LEFT_MID, 100, 0);
	} else {
		lv_obj_align(cont, LV_ALIGN_CENTER, 0, 10);
		lv_obj_align(spinner, LV_ALIGN_LEFT_MID, 20, 0);
		lv_obj_align(text, LV_ALIGN_LEFT_MID, 60, 0);
	}

	return cont;
}

lv_obj_t* tt_obj_spinner_create(lv_obj_t* scr, const char* msg)
{
	lv_obj_t* spinner = lv_spinner_create(scr, 2000, 100);
	lv_obj_set_size(spinner, 80, 80);
	lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -40);
	lv_obj_add_style(spinner, &spinner_style,
			LV_STATE_DEFAULT | LV_PART_INDICATOR);
	lv_obj_t* label = lv_label_create(scr);
	lv_label_set_text(label, msg);
	if (screen_is_landscape()) {
		lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 140);
	} else {
		lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 175);
	}

	return spinner;
}

lv_obj_t* tt_obj_txt_create(lv_obj_t* parent, char* placeholder,
		lv_event_cb_t cb)
{
	lv_obj_t* txt = lv_textarea_create(parent);
	lv_textarea_set_one_line(txt, true);
	lv_obj_align(txt, LV_ALIGN_TOP_MID, 0, 10);
	lv_obj_add_style(txt, &txt_style, LV_STATE_DEFAULT);
	lv_textarea_set_placeholder_text(txt, placeholder);
	lv_obj_set_size(txt, lv_pct(100), 36);
	lv_obj_add_event_cb(txt, cb, LV_EVENT_ALL, NULL);

	return txt;
}

lv_obj_t* tt_obj_dropdown_create(lv_obj_t* parent, char* options,
		lv_event_cb_t cb)
{
	lv_obj_t* dd = lv_dropdown_create(parent);
	lv_dropdown_set_options(dd, options);
	lv_obj_add_style(dd, &txt_style, LV_STATE_DEFAULT);
	lv_obj_set_size(dd, LV_PCT(100), 36);
	lv_obj_align(dd, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_add_event_cb(dd, cb, LV_EVENT_ALL, NULL);

	lv_obj_t* list = lv_dropdown_get_list(dd);
	lv_obj_add_style(list, &cont_style, 0);
	lv_obj_add_style(list, &btn_press_style, LV_PART_SELECTED);
	lv_dropdown_set_selected_highlight(dd, false);

	return dd;
}

lv_obj_t* tt_obj_checkbox_create(lv_obj_t* parent, char* txt, lv_event_cb_t cb)
{
	lv_obj_t* cbx = lv_checkbox_create(parent);
	lv_obj_align(cbx, LV_ALIGN_CENTER, 0, 0);
	lv_checkbox_set_text(cbx, txt);
	lv_obj_add_event_cb(cbx, cb, LV_EVENT_ALL, NULL);

	//TODO: change color
	// lv_obj_add_style(cbx, &btn_press_style, LV_PART_MAIN);

	return cbx;
}

lv_obj_t* tt_obj_loader_create(const char* msg, lv_event_cb_t cancel_cb)
{
	lv_obj_t* loader_scr = lv_obj_create(NULL);
	tt_obj_spinner_create(loader_scr, msg);
	if (cancel_cb != NULL) {
		lv_obj_t* btn = lv_obj_create(loader_scr);
		lv_obj_set_size(btn, 100, 50);
		lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
		lv_obj_add_event_cb(btn, cancel_cb, LV_EVENT_ALL, NULL);
		lv_obj_add_style(btn, &btn_style, LV_STATE_DEFAULT);
		lv_obj_add_style(btn, &btn_press_style, LV_STATE_PRESSED);
		lv_obj_align(btn, LV_ALIGN_CENTER, 0, 80);
		lv_obj_t* label = lv_label_create(btn);
		lv_label_set_text(label, "CANCEL");
		lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
	}

	return loader_scr;
}
