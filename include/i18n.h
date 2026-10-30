#ifndef I18N_H
#define I18N_H

#include <stdbool.h>

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Custom translation file, written by the PDU API (web UI). It replaces the
 * default file installed with the app (assets/translations.csv). */
#ifndef I18N_CUSTOM_FILE
#define I18N_CUSTOM_FILE "/home/root/.cmdisplay.translations.csv"
#endif

/* One translatable string built into the program (src/i18n_strings.c,
 * generated from assets/translations.csv): the English text is the key, as
 * used in tr(). A NULL translation falls back to English. */
typedef struct {
	const char* en;
	const char* es;
	const char* de;
	const char* zh;
	const char* ar;
} i18n_entry_t;

extern const i18n_entry_t I18N_STRINGS[];
extern const unsigned I18N_STRING_COUNT;

/* Marks a string for translation where tr() can't be called yet, such as a
 * static array initializer; translate it with tr() where it is shown. Lets
 * tools/check_i18n.py find it. */
#define N_(text) (text)

/**
 * @brief Loads the translation file and selects the language. Call once,
 * after lv_init() and before any screen is created.
 *
 * The translation file is a CSV table, one row per text and one column per
 * language, headed "Name (code)", for example "Français (fr)". The English
 * column is the key. A column with a new code adds a language. Each text of
 * the selected language is checked (printf conversions and '#' color marks
 * as in English, valid UTF-8, characters the fonts can draw); a text that
 * fails, or is empty, keeps the built-in translation, else English.
 *
 * @param[in] code  Language code as stored in the display settings and used
 *                  by the web UI ("en", "es", "de", "zh-hans", "ar", or one
 *                  added by the translation file). Unknown or NULL selects
 *                  English.
 */
void i18n_init(const char* code);

/**
 * @brief True when I18N_CUSTOM_FILE was added, changed or removed since
 * i18n_init(): the program has to restart to use it.
 */
bool i18n_file_changed(void);

/**
 * @brief Returns text in the current language.
 *
 * @param[in] text  English text, exactly as listed in i18n_strings.c.
 * @return The translation, or text itself when there is none.
 */
const char* tr(const char* text);

/* Languages: the built-in ones (English first), then the ones the
 * translation file adds, in the order of the Language dropdown. */
int i18n_lang_count(void);
/* Index of the current language. */
int i18n_current_lang(void);
const char* i18n_lang_code(int index);
/* Name of the language as it writes it. */
const char* i18n_native_name(int index);

/* Native language names, one per line, in index order: the options of the
 * Language dropdown. */
const char* i18n_dropdown_options(void);

/* True for right-to-left languages (Arabic, Persian, Hebrew...). */
bool i18n_is_rtl(void);

/**
 * @brief Applies the language to the display: every screen, popup and the
 * top layers use i18n_font() and, for right-to-left languages, lay out right
 * to left. Call after i18n_init() and before any
 * screen is created.
 */
void i18n_setup_display(lv_disp_t* disp);

/**
 * @brief Sets a label's text, keeping LVGL color commands ("#RRGGBB text#")
 * working in right-to-left languages. LVGL parses the commands after
 * reordering a line for right-to-left, which separates the '#' marks from
 * the values they color; a left-to-right mark (U+200E) on each side keeps
 * each colored value in one piece. Other languages get the text unchanged.
 */
void i18n_label_set_text(lv_obj_t* label, const char* text);

/**
 * @brief Font for text at the given pixel size, covering every language
 * and drawing numbers with the language's digits:
 * Montserrat for Latin script, then DejaVu Sans (Arabic), WenQuanYi Zen Hei
 * (Chinese) and the built-in Montserrat (LVGL symbols) as fallbacks. If the
 * font files are missing the built-in Montserrat is returned.
 */
const lv_font_t* i18n_font(int size);

/**
 * @brief The same font with ASCII digits (0-9). i18n_font() draws numbers
 * with the language's own digits (Arabic: ٠١٢٣٤٥٦٧٨٩); addresses that are
 * typed elsewhere as they are (IP and MAC addresses) keep ASCII digits.
 * Texts always hold ASCII digits: only the drawing differs.
 */
const lv_font_t* i18n_font_western_digits(int size);

/**
 * @brief Makes an object (label, text area...) draw ASCII digits: for IP
 * and MAC addresses. Call after its font is set; does nothing in languages
 * that write ASCII digits.
 */
void i18n_western_digits(lv_obj_t* obj);

/**
 * @brief Writes a colored value and its unit for a label ("#A1C61A 1.5 V#").
 * Right-to-left languages close the color before the unit: a value with
 * right-to-left letters would lose its color (see i18n_label_set_text()).
 *
 * @param[in] unit  Unit, already translated (tr("V")); NULL or "" for none.
 */
void i18n_format_value(char* dst, size_t size, uint32_t color, const char* value,
		const char* unit);

/**
 * @brief Keeps a text area's placeholder inside the field in right-to-left
 * languages. LVGL draws the placeholder in the field's area moved right by
 * the left padding, so right-aligned (right-to-left) text loses its first
 * letters past the right edge; left-aligned it is placed right.
 */
void i18n_textarea_fix_placeholder(lv_obj_t* textarea);

/* True if i18n_western_digits() was applied to the object. */
bool i18n_has_western_digits(lv_obj_t* obj);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* I18N_H */
