#include "tt_keyboard.h"

#include "lvgl/lvgl.h"

/* Global variables ***********************************************************/

static const char* kb_map_lc[] = {
	"a", "b", "c", "d", "e", "f", "\n",
	"g", "h", "i", "j", "k", "l", "\n",
	"m", "n", "o", "p", "q", "r", "\n",
	"ABC", "s", "t", "u", "v", LV_SYMBOL_BACKSPACE, "\n",
	"1#", "w", "x", "y", "z", LV_SYMBOL_NEW_LINE, "\n",
	LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, NULL
};

static const char* kb_map_uc[] = {
	"A", "B", "C", "D", "E", "F", "\n",
	"G", "H", "I", "J", "K", "L", "\n",
	"M", "N", "O", "P", "Q", "R", "\n",
	"abc", "S", "T", "U", "V", LV_SYMBOL_BACKSPACE, "\n",
	"1#", "W", "X", "Y", "Z", LV_SYMBOL_NEW_LINE, "\n",
	LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, NULL
};

static const char* kb_map_sym[] = {
	"1", "2", "3", "4", "5", "6", "\n",
	"7", "8", "9", "0", "+", "&", "\n",
	"/", "*", "=", "%", "!", "?", "\n",
	"#", "<", ">", "\\",  "@", "$", "\n",
	"abc", ";", "\"", "(", ")", LV_SYMBOL_BACKSPACE, "\n",
	// "{", "}", "[", "]", "'"
	LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, NULL
};

static const char* num_kb_map[] = {
	"1", "2", "3", LV_SYMBOL_CLOSE, "\n",
	"4", "5", "6", LV_SYMBOL_NEW_LINE, "\n",
	"7", "8", "9", LV_SYMBOL_BACKSPACE, "\n",
	",", "0", ".", LV_SYMBOL_OK, NULL,
};

static const lv_btnmatrix_ctrl_t kb_ctrl[] = {
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1 ,
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1 ,
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1 ,
	LV_KEYBOARD_CTRL_BTN_FLAGS | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
	LV_KEYBOARD_CTRL_BTN_FLAGS | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
	LV_KEYBOARD_CTRL_BTN_FLAGS | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1, 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
};

static const lv_btnmatrix_ctrl_t kb_sym_ctrl[] = {
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1 ,
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1 ,
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1 ,
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1 ,
	LV_KEYBOARD_CTRL_BTN_FLAGS | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
	LV_KEYBOARD_CTRL_BTN_FLAGS | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1, 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
};

static const lv_btnmatrix_ctrl_t num_kb_ctrl[] = {
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
};

/* Function prototypes ********************************************************/
/* Callbacks ******************************************************************/
/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

lv_obj_t* tt_keyboard_create(lv_obj_t* parent)
{
	lv_obj_t* kb = lv_keyboard_create(parent);

	lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, kb_map_lc, kb_ctrl);
	lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_UPPER, kb_map_uc, kb_ctrl);
	lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, kb_map_sym, kb_sym_ctrl);
	lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, num_kb_map, num_kb_ctrl);
	lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
	lv_keyboard_set_popovers(kb, true);

	return kb;
}
