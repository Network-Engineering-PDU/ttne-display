#include <string.h>

#include "tt_keyboard.h"

#include "lvgl/lvgl.h"
#include "i18n.h"
#include "pinyin_dict.h"

/* Each language types with its own layout: English alphabetical (the
 * original layout), German QWERTZ with umlauts and ß, Spanish QWERTY with ñ
 * and accents, Arabic letters, Chinese Pinyin (QWERTY letters through the
 * Pinyin input method). Arabic and Chinese have a key to type Latin letters
 * too, for passwords, names and addresses.
 *
 * Keyboard modes hold the layouts:
 *   TEXT_LOWER / TEXT_UPPER  the language's letters
 *   SPECIAL                  digits and symbols
 *   USER_1 / USER_2          number pad / PIN pad
 *   USER_3 / USER_4          Latin letters (Arabic and Chinese only) */

#define MODE_LATIN_LOWER LV_KEYBOARD_MODE_USER_3
#define MODE_LATIN_UPPER LV_KEYBOARD_MODE_USER_4

/* Keys that change the layout */
#define KEY_UPPER "ABC"
#define KEY_LOWER "abc"
#define KEY_SYMBOLS "1#"
#define KEY_ARABIC "أب"		/* Latin or symbols -> Arabic */
#define KEY_LATIN_AR "AB"	/* Arabic -> Latin */
#define KEY_CHINESE "中"		/* Latin or symbols -> Pinyin */
#define KEY_LATIN_ZH "英"	/* Pinyin -> Latin */

#define MAX_KEYS 64

typedef enum {
	FAMILY_ALPHABETICAL,
	FAMILY_GERMAN,
	FAMILY_SPANISH,
	FAMILY_ARABIC,
	FAMILY_CHINESE,
} family_t;

/* Global variables ***********************************************************/

/* English and the languages without a layout of their own */
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

/* German: QWERTZ */
static const char* kb_map_de_lc[] = {
	"q", "w", "e", "r", "t", "z", "u", "i", "o", "p", "ü", "\n",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", "ö", "ä", "\n",
	KEY_UPPER, "y", "x", "c", "v", "b", "n", "m", "ß", LV_SYMBOL_BACKSPACE, "\n",
	LV_SYMBOL_CLOSE, KEY_SYMBOLS, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_NEW_LINE, LV_SYMBOL_OK, NULL
};

static const char* kb_map_de_uc[] = {
	"Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P", "Ü", "\n",
	"A", "S", "D", "F", "G", "H", "J", "K", "L", "Ö", "Ä", "\n",
	KEY_LOWER, "Y", "X", "C", "V", "B", "N", "M", "ß", LV_SYMBOL_BACKSPACE, "\n",
	LV_SYMBOL_CLOSE, KEY_SYMBOLS, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_NEW_LINE, LV_SYMBOL_OK, NULL
};

/* Spanish: QWERTY with ñ, accented vowels and the opening marks */
static const char* kb_map_es_lc[] = {
	"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", "ñ", "\n",
	KEY_UPPER, "z", "x", "c", "v", "b", "n", "m", LV_SYMBOL_BACKSPACE, "\n",
	"á", "é", "í", "ó", "ú", "ü", "¿", "¡", "\n",
	LV_SYMBOL_CLOSE, KEY_SYMBOLS, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_NEW_LINE, LV_SYMBOL_OK, NULL
};

static const char* kb_map_es_uc[] = {
	"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
	"A", "S", "D", "F", "G", "H", "J", "K", "L", "Ñ", "\n",
	KEY_LOWER, "Z", "X", "C", "V", "B", "N", "M", LV_SYMBOL_BACKSPACE, "\n",
	"Á", "É", "Í", "Ó", "Ú", "Ü", "¿", "¡", "\n",
	LV_SYMBOL_CLOSE, KEY_SYMBOLS, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_NEW_LINE, LV_SYMBOL_OK, NULL
};

/* Arabic: the letters in the order of the Arabic PC layout, then the
 * hamza forms */
static const char* kb_map_ar[] = {
	"ض", "ص", "ث", "ق", "ف", "غ", "ع", "ه", "خ", "\n",
	"ح", "ج", "ش", "س", "ي", "ب", "ل", "ا", "ت", "\n",
	"ن", "م", "ك", "ط", "ظ", "د", "ذ", "ر", "ز", "\n",
	"و", "ة", "ى", "ء", "ئ", "ؤ", "أ", "إ", "آ", "\n",
	LV_SYMBOL_CLOSE, KEY_SYMBOLS, KEY_LATIN_AR, " ", "،", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, NULL
};

static const char* kb_map_ar_sym[] = {
	"1", "2", "3", "4", "5", "6", "\n",
	"7", "8", "9", "0", "+", "&", "\n",
	"/", "*", "=", "%", "!", "؟", "\n",
	"#", "<", ">", "\\",  "@", "$", "\n",
	KEY_ARABIC, "؛", "\"", "(", ")", LV_SYMBOL_BACKSPACE, "\n",
	LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, NULL
};

/* Chinese: the Pinyin input method turns the typed letters into
 * characters, chosen in the bar above the keyboard */
static const char* kb_map_zh[] = {
	"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
	KEY_LATIN_ZH, "z", "x", "c", "v", "b", "n", "m", LV_SYMBOL_BACKSPACE, "\n",
	LV_SYMBOL_CLOSE, KEY_SYMBOLS, "，", " ", "。", LV_SYMBOL_NEW_LINE, LV_SYMBOL_OK, NULL
};

static const char* kb_map_zh_sym[] = {
	"1", "2", "3", "4", "5", "6", "\n",
	"7", "8", "9", "0", "+", "&", "\n",
	"/", "*", "=", "%", "！", "？", "\n",
	"#", "<", ">", "\\",  "@", "$", "\n",
	KEY_CHINESE, "；", "：", "（", "）", LV_SYMBOL_BACKSPACE, "\n",
	LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, NULL
};

/* Latin letters from Arabic or Chinese: the English layout with a key
 * back to the language's own letters */
static const char* kb_map_latin_lc[] = {
	"a", "b", "c", "d", "e", "f", "\n",
	"g", "h", "i", "j", "k", "l", "\n",
	"m", "n", "o", "p", "q", "r", "\n",
	KEY_UPPER, "s", "t", "u", "v", LV_SYMBOL_BACKSPACE, "\n",
	KEY_SYMBOLS, "w", "x", "y", "z", LV_SYMBOL_NEW_LINE, "\n",
	LV_SYMBOL_CLOSE, "", LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, NULL
};

static const char* kb_map_latin_uc[] = {
	"A", "B", "C", "D", "E", "F", "\n",
	"G", "H", "I", "J", "K", "L", "\n",
	"M", "N", "O", "P", "Q", "R", "\n",
	KEY_LOWER, "S", "T", "U", "V", LV_SYMBOL_BACKSPACE, "\n",
	KEY_SYMBOLS, "W", "X", "Y", "Z", LV_SYMBOL_NEW_LINE, "\n",
	LV_SYMBOL_CLOSE, "", LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, NULL
};

static const char* num_kb_map[] = {
	"1", "2", "3", LV_SYMBOL_CLOSE, "\n",
	"4", "5", "6", LV_SYMBOL_NEW_LINE, "\n",
	"7", "8", "9", LV_SYMBOL_BACKSPACE, "\n",
	",", "0", ".", LV_SYMBOL_OK, NULL,
};

static const char* pass_kb_map[] = {
	"1", "2", "3", "\n",
	"4", "5", "6", "\n",
	"7", "8", "9", "\n",
	LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, NULL,
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

static const lv_btnmatrix_ctrl_t pass_kb_ctrl[] = {
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1,
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1,
	LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_BTNMATRIX_CTRL_POPOVER | 1,
	LV_KEYBOARD_CTRL_BTN_FLAGS | 1, LV_BTNMATRIX_CTRL_POPOVER | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
};

/* Button controls of the layouts made here, filled by fill_ctrl() */
static lv_btnmatrix_ctrl_t native_lc_ctrl[MAX_KEYS];
static lv_btnmatrix_ctrl_t native_uc_ctrl[MAX_KEYS];
static lv_btnmatrix_ctrl_t native_sym_ctrl[MAX_KEYS];
static lv_btnmatrix_ctrl_t latin_lc_ctrl[MAX_KEYS];
static lv_btnmatrix_ctrl_t latin_uc_ctrl[MAX_KEYS];

/* Function prototypes ********************************************************/

static void kb_event_cb(lv_event_t* e);

/* Function definitions *******************************************************/

static family_t language_family(void)
{
	const char* code = i18n_lang_code(i18n_current_lang());
	if (strcmp(code, "de") == 0) {
		return FAMILY_GERMAN;
	}
	if (strcmp(code, "es") == 0) {
		return FAMILY_SPANISH;
	}
	if (strcmp(code, "ar") == 0) {
		return FAMILY_ARABIC;
	}
	if (strcmp(code, "zh-hans") == 0) {
		return FAMILY_CHINESE;
	}
	return FAMILY_ALPHABETICAL;
}

static bool is_control_key(const char* key)
{
	static const char* const CONTROL[] = {
		LV_SYMBOL_CLOSE, LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_NEW_LINE,
		LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, KEY_UPPER, KEY_LOWER, KEY_SYMBOLS,
		KEY_ARABIC, KEY_LATIN_AR, KEY_CHINESE, KEY_LATIN_ZH,
	};
	for (size_t i = 0; i < sizeof(CONTROL) / sizeof(CONTROL[0]); i++) {
		if (strcmp(key, CONTROL[i]) == 0) {
			return true;
		}
	}
	return false;
}

/* Button controls for a layout: characters show a popover, the space bar
 * is wide, layout and editing keys are twice as wide as a letter. */
static void fill_ctrl(const char* const* map, lv_btnmatrix_ctrl_t* ctrl)
{
	int n = 0;
	for (int i = 0; map[i] != NULL && n < MAX_KEYS; i++) {
		if (strcmp(map[i], "\n") == 0) {
			continue;
		}
		if (strcmp(map[i], " ") == 0) {
			ctrl[n++] = 4;
		} else if (is_control_key(map[i])) {
			ctrl[n++] = LV_KEYBOARD_CTRL_BTN_FLAGS | 2;
		} else {
			ctrl[n++] = LV_BTNMATRIX_CTRL_POPOVER | 1;
		}
	}
}

static void set_layout(lv_obj_t* kb, lv_keyboard_mode_t mode, const char** map,
		lv_btnmatrix_ctrl_t* ctrl)
{
	fill_ctrl(map, ctrl);
	lv_keyboard_set_map(kb, mode, map, ctrl);
}

/* The Pinyin input method of a Chinese keyboard, NULL for other ones */
static lv_obj_t* keyboard_ime(lv_obj_t* kb)
{
	return lv_obj_get_user_data(kb);
}

static void switch_mode(lv_obj_t* kb, lv_keyboard_mode_t mode)
{
	lv_obj_t* ime = keyboard_ime(kb);
	if (ime != NULL) {
		/* Pinyin turns letters into characters only on its own layout. With
		 * 9-key mode compiled out, the number mode just ignores letters. */
		lv_ime_pinyin_set_mode(ime, mode == LV_KEYBOARD_MODE_TEXT_LOWER ?
				LV_IME_PINYIN_MODE_K26 : LV_IME_PINYIN_MODE_K9_NUMBER);
		lv_obj_add_flag(lv_ime_pinyin_get_cand_panel(ime), LV_OBJ_FLAG_HIDDEN);
	}
	lv_keyboard_set_mode(kb, mode);
}

/* Callbacks ******************************************************************/

/* Handles the layout keys; LVGL's handler types the others. */
static void kb_event_cb(lv_event_t* e)
{
	lv_obj_t* kb = lv_event_get_target(e);
	uint16_t btn = lv_btnmatrix_get_selected_btn(kb);
	const char* key = btn != LV_BTNMATRIX_BTN_NONE ?
			lv_btnmatrix_get_btn_text(kb, btn) : NULL;
	if (key == NULL) {
		return;
	}
	lv_keyboard_mode_t mode = lv_keyboard_get_mode(kb);
	bool latin = mode == MODE_LATIN_LOWER || mode == MODE_LATIN_UPPER;

	if (strcmp(key, KEY_UPPER) == 0) {
		switch_mode(kb, latin ? MODE_LATIN_UPPER : LV_KEYBOARD_MODE_TEXT_UPPER);
	} else if (strcmp(key, KEY_LOWER) == 0) {
		switch_mode(kb, latin ? MODE_LATIN_LOWER : LV_KEYBOARD_MODE_TEXT_LOWER);
	} else if (strcmp(key, KEY_SYMBOLS) == 0) {
		switch_mode(kb, LV_KEYBOARD_MODE_SPECIAL);
	} else if (strcmp(key, KEY_ARABIC) == 0 || strcmp(key, KEY_CHINESE) == 0) {
		switch_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
	} else if (strcmp(key, KEY_LATIN_AR) == 0 || strcmp(key, KEY_LATIN_ZH) == 0) {
		switch_mode(kb, MODE_LATIN_LOWER);
	} else {
		lv_keyboard_def_event_cb(e);
	}
}

/* Public functions ***********************************************************/

lv_obj_t* tt_keyboard_create(lv_obj_t* parent)
{
	lv_obj_t* kb = lv_keyboard_create(parent);
	lv_obj_set_user_data(kb, NULL);
	/* kb_event_cb hands LVGL's handler the keys it doesn't handle */
	lv_obj_remove_event_cb(kb, lv_keyboard_def_event_cb);
	lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

	lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, num_kb_map, num_kb_ctrl);
	lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_2, pass_kb_map, pass_kb_ctrl);

	family_t family = language_family();
	switch (family) {
	case FAMILY_GERMAN:
		set_layout(kb, LV_KEYBOARD_MODE_TEXT_LOWER, kb_map_de_lc, native_lc_ctrl);
		set_layout(kb, LV_KEYBOARD_MODE_TEXT_UPPER, kb_map_de_uc, native_uc_ctrl);
		lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, kb_map_sym, kb_sym_ctrl);
		break;
	case FAMILY_SPANISH:
		set_layout(kb, LV_KEYBOARD_MODE_TEXT_LOWER, kb_map_es_lc, native_lc_ctrl);
		set_layout(kb, LV_KEYBOARD_MODE_TEXT_UPPER, kb_map_es_uc, native_uc_ctrl);
		lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, kb_map_sym, kb_sym_ctrl);
		break;
	case FAMILY_ARABIC:
	case FAMILY_CHINESE: {
		bool arabic = family == FAMILY_ARABIC;
		set_layout(kb, LV_KEYBOARD_MODE_TEXT_LOWER, arabic ? kb_map_ar : kb_map_zh,
				native_lc_ctrl);
		set_layout(kb, LV_KEYBOARD_MODE_SPECIAL, arabic ? kb_map_ar_sym : kb_map_zh_sym,
				native_sym_ctrl);
		/* The Latin layouts' key back to the language's letters */
		const char* back = arabic ? KEY_ARABIC : KEY_CHINESE;
		for (int i = 0; kb_map_latin_lc[i] != NULL; i++) {
			if (kb_map_latin_lc[i][0] == '\0') {
				kb_map_latin_lc[i] = back;
				kb_map_latin_uc[i] = back;
			}
		}
		set_layout(kb, MODE_LATIN_LOWER, kb_map_latin_lc, latin_lc_ctrl);
		set_layout(kb, MODE_LATIN_UPPER, kb_map_latin_uc, latin_uc_ctrl);
		if (!arabic) {
			lv_obj_t* ime = lv_ime_pinyin_create(parent);
			lv_ime_pinyin_set_dict(ime, pinyin_dict);
			lv_ime_pinyin_set_keyboard(ime, kb);
			lv_obj_set_user_data(kb, ime);
			lv_obj_t* panel = lv_ime_pinyin_get_cand_panel(ime);
			lv_obj_set_height(panel, 34);
			lv_obj_set_style_pad_all(panel, 2, 0);
			lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
			lv_obj_set_style_bg_color(panel, lv_obj_get_style_bg_color(kb, 0), 0);
			lv_obj_set_style_text_color(panel, lv_color_black(),
					LV_PART_ITEMS | LV_STATE_PRESSED);
		}
		break;
	}
	default:
		lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, kb_map_lc, kb_ctrl);
		lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_UPPER, kb_map_uc, kb_ctrl);
		lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, kb_map_sym, kb_sym_ctrl);
		break;
	}
	lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
	lv_keyboard_set_popovers(kb, true);

	return kb;
}

void tt_keyboard_set_type(lv_obj_t* kb, keyboard_type_t type)
{
	switch (type) {
	case KB_NUM:
		switch_mode(kb, LV_KEYBOARD_MODE_USER_1);
		break;
	case KB_PASS:
		switch_mode(kb, LV_KEYBOARD_MODE_USER_2);
		break;
	case KB_LATIN: {
		/* The Latin layout where the language has other letters */
		family_t family = language_family();
		bool other = family == FAMILY_ARABIC || family == FAMILY_CHINESE;
		switch_mode(kb, other ? MODE_LATIN_LOWER : LV_KEYBOARD_MODE_TEXT_LOWER);
		break;
	}
	case KB_ABC:
	default:
		switch_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
		break;
	}
}

void tt_keyboard_place_ime(lv_obj_t* kb)
{
	lv_obj_t* ime = keyboard_ime(kb);
	if (ime != NULL) {
		lv_obj_align_to(lv_ime_pinyin_get_cand_panel(ime), kb,
				LV_ALIGN_OUT_TOP_MID, 0, 0);
	}
}
