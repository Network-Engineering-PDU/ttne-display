#ifndef I18N_H
#define I18N_H

#include <stdbool.h>

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Languages of the touchscreen, the same set as the web UI. The order is the
 * order of the Language dropdown on the Visualization screen. */
typedef enum {
	I18N_EN,
	I18N_ES,
	I18N_DE,
	I18N_ZH,
	I18N_AR,
	I18N_COUNT,
} i18n_lang_t;

/* One translatable string: the English text is the key, as used in tr(). A
 * NULL translation falls back to English. */
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
 * @brief Selects the language and loads the fonts it needs. Call once, after
 * lv_init() and before any screen is created.
 *
 * @param[in] code  Language code as stored in the display settings and used
 *                  by the web UI ("en", "es", "de", "zh-hans", "ar"). Unknown
 *                  or NULL selects English.
 */
void i18n_init(const char* code);

/**
 * @brief Returns text in the current language.
 *
 * @param[in] text  English text, exactly as listed in i18n_strings.c.
 * @return The translation, or text itself when there is none.
 */
const char* tr(const char* text);

i18n_lang_t i18n_get_lang(void);
i18n_lang_t i18n_lang_from_code(const char* code);
const char* i18n_lang_code(i18n_lang_t lang);

/* Language names as each language writes its own name, one per line, in
 * i18n_lang_t order: the options of the Language dropdown. */
const char* i18n_dropdown_options(void);
const char* i18n_native_name(i18n_lang_t lang);

/* True for right-to-left languages (Arabic). */
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
 * @brief Font for text at the given pixel size, covering every language:
 * Montserrat for Latin script, then DejaVu Sans (Arabic), WenQuanYi Zen Hei
 * (Chinese) and the built-in Montserrat (LVGL symbols) as fallbacks. If the
 * font files are missing the built-in Montserrat is returned.
 */
const lv_font_t* i18n_font(int size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* I18N_H */
