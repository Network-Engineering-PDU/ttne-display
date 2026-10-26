#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "i18n.h"

#ifndef ASSET_PATH
#define ASSET_PATH "A:assets/"
#endif

/* Fonts installed by the image (ttf-dejavu-sans, ttf-wqy-zenhei) and shipped
 * with this app (assets/fonts). */
#define FONT_DIR "/usr/share/fonts/truetype/"
#define FONT_LATIN "fonts/Montserrat-Regular.ttf"
#define FONT_ARABIC FONT_DIR "DejaVuSans.ttf"
#define FONT_CHINESE FONT_DIR "wqy-zenhei.ttc"

#define MAX_FONT_SIZE 48
/* Size of the text that uses the theme font: LV_FONT_DEFAULT is Montserrat 14. */
#define DEFAULT_FONT_SIZE 14

/* Global variables ***********************************************************/

static i18n_lang_t current_lang = I18N_EN;

/* I18N_STRINGS indexes sorted by English text, for bsearch(). */
static unsigned* sorted;

static const lv_font_t* fonts[MAX_FONT_SIZE + 1];
static bool freetype_ready;

static lv_theme_t screen_theme;
static lv_style_t screen_style;

static const struct {
	const char* code;
	const char* native_name;
} LANGS[I18N_COUNT] = {
	[I18N_EN] = { "en", "English" },
	[I18N_ES] = { "es", "Español" },
	[I18N_DE] = { "de", "Deutsch" },
	[I18N_ZH] = { "zh-hans", "中文 (简体)" },
	[I18N_AR] = { "ar", "العربية" },
};

/* Function definitions *******************************************************/

static int compare_index(const void* a, const void* b)
{
	return strcmp(I18N_STRINGS[*(const unsigned*)a].en,
			I18N_STRINGS[*(const unsigned*)b].en);
}

static int compare_key(const void* key, const void* item)
{
	return strcmp((const char*)key, I18N_STRINGS[*(const unsigned*)item].en);
}

static const char* translation(const i18n_entry_t* entry)
{
	switch (current_lang) {
	case I18N_ES: return entry->es;
	case I18N_DE: return entry->de;
	case I18N_ZH: return entry->zh;
	case I18N_AR: return entry->ar;
	default: return entry->en;
	}
}

/* The asset path is an LVGL drive path ("A:/usr/..."); FreeType needs the
 * plain file system path. */
static void asset_file(char* dst, size_t size, const char* name)
{
	const char* base = ASSET_PATH;
	if (base[0] != '\0' && base[1] == ':') {
		base += 2;
	}
	snprintf(dst, size, "%s%s", base, name);
}

static const lv_font_t* builtin_font(int size)
{
	/* Montserrat sizes enabled in lv_conf.h: 10 to 28 in steps of 2. */
	if (size <= 10) return &lv_font_montserrat_10;
	if (size <= 12) return &lv_font_montserrat_12;
	if (size <= 14) return &lv_font_montserrat_14;
	if (size <= 16) return &lv_font_montserrat_16;
	if (size <= 18) return &lv_font_montserrat_18;
	if (size <= 20) return &lv_font_montserrat_20;
	if (size <= 22) return &lv_font_montserrat_22;
	if (size <= 24) return &lv_font_montserrat_24;
	if (size <= 26) return &lv_font_montserrat_26;
	return &lv_font_montserrat_28;
}

static lv_font_t* load_font(const char* path, int size)
{
	if (access(path, R_OK) != 0) {
		LV_LOG_WARN("Font %s not found", path);
		return NULL;
	}
	lv_font_t* font = lv_freetype_font_create(path, size,
			LV_FREETYPE_FONT_STYLE_NORMAL);
	if (font == NULL) {
		LV_LOG_WARN("Font %s could not be loaded", path);
	}
	return font;
}

/* Gives every screen (an object without a parent) the language's font and,
 * for Arabic, right-to-left layout; the children inherit both. */
static void screen_apply_cb(lv_theme_t* theme, lv_obj_t* obj)
{
	(void)theme;
	if (lv_obj_get_parent(obj) == NULL) {
		lv_obj_add_style(obj, &screen_style, 0);
	}
}

/* Public functions ***********************************************************/

void i18n_init(const char* code)
{
	current_lang = i18n_lang_from_code(code);

	free(sorted);
	sorted = malloc(I18N_STRING_COUNT * sizeof(*sorted));
	if (sorted != NULL) {
		for (unsigned i = 0; i < I18N_STRING_COUNT; i++) {
			sorted[i] = i;
		}
		qsort(sorted, I18N_STRING_COUNT, sizeof(*sorted), compare_index);
	}

	freetype_ready = lv_freetype_init(0, 0, LV_FREETYPE_CACHE_SIZE) == LV_RES_OK;
	if (!freetype_ready) {
		LV_LOG_WARN("FreeType could not start; using built-in fonts");
	}
	LV_LOG_USER("Language: %s", LANGS[current_lang].code);
}

const char* tr(const char* text)
{
	if (text == NULL || current_lang == I18N_EN || sorted == NULL) {
		return text;
	}
	const unsigned* found = bsearch(text, sorted, I18N_STRING_COUNT,
			sizeof(*sorted), compare_key);
	if (found == NULL) {
		return text;
	}
	const char* translated = translation(&I18N_STRINGS[*found]);
	return translated != NULL ? translated : text;
}

i18n_lang_t i18n_get_lang(void)
{
	return current_lang;
}

i18n_lang_t i18n_lang_from_code(const char* code)
{
	if (code != NULL) {
		for (int i = 0; i < I18N_COUNT; i++) {
			if (strcmp(code, LANGS[i].code) == 0) {
				return (i18n_lang_t)i;
			}
		}
	}
	return I18N_EN;
}

const char* i18n_lang_code(i18n_lang_t lang)
{
	return lang < I18N_COUNT ? LANGS[lang].code : LANGS[I18N_EN].code;
}

const char* i18n_native_name(i18n_lang_t lang)
{
	return lang < I18N_COUNT ? LANGS[lang].native_name : LANGS[I18N_EN].native_name;
}

const char* i18n_dropdown_options(void)
{
	static char options[128];
	if (options[0] == '\0') {
		for (int i = 0; i < I18N_COUNT; i++) {
			if (i > 0) {
				strncat(options, "\n", sizeof(options) - strlen(options) - 1);
			}
			strncat(options, LANGS[i].native_name,
					sizeof(options) - strlen(options) - 1);
		}
	}
	return options;
}

void i18n_setup_display(lv_disp_t* disp)
{
	if (disp == NULL) {
		disp = lv_disp_get_default();
	}
	const lv_font_t* font = i18n_font(DEFAULT_FONT_SIZE);
	/* Same palette as LVGL's automatic default theme. This LVGL's default
	 * theme doesn't apply its font to text (text uses LV_FONT_DEFAULT), so
	 * the font is set on every screen below. */
	lv_theme_t* theme = lv_theme_default_init(disp,
			lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
			LV_THEME_DEFAULT_DARK, font);

	lv_style_init(&screen_style);
	lv_style_set_text_font(&screen_style, font);
	if (i18n_is_rtl()) {
		lv_style_set_base_dir(&screen_style, LV_BASE_DIR_RTL);
	}
	screen_theme = *theme;
	lv_theme_set_parent(&screen_theme, theme);
	lv_theme_set_apply_cb(&screen_theme, screen_apply_cb);
	lv_disp_set_theme(disp, &screen_theme);
	/* Screens and layers that already exist. */
	lv_obj_add_style(lv_disp_get_scr_act(disp), &screen_style, 0);
	lv_obj_add_style(lv_disp_get_layer_top(disp), &screen_style, 0);
	lv_obj_add_style(lv_disp_get_layer_sys(disp), &screen_style, 0);
}

bool i18n_is_rtl(void)
{
	return current_lang == I18N_AR;
}

/* "#RRGGBB " starts a color command. */
static bool is_color_command(const char* p)
{
	if (p[0] != '#') {
		return false;
	}
	for (int i = 1; i <= 6; i++) {
		char c = p[i];
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
			return false;
		}
	}
	return p[7] == ' ';
}

/* Arabic, Hebrew and their presentation forms: LVGL's right-to-left set. */
static bool has_rtl_letter(const char* text)
{
	uint32_t i = 0;
	uint32_t letter;
	while ((letter = _lv_txt_encoded_next(text, &i)) != 0) {
		if ((letter >= 0x590 && letter <= 0x8FF) || (letter >= 0xFB1D && letter <= 0xFDFF) ||
				(letter >= 0xFE70 && letter <= 0xFEFF)) {
			return true;
		}
	}
	return false;
}

void i18n_label_set_text(lv_obj_t* label, const char* text)
{
	static const char LRM[] = "\xE2\x80\x8E"; /* U+200E LEFT-TO-RIGHT MARK */
	const size_t lrm_len = sizeof(LRM) - 1;
	if (!i18n_is_rtl() || text == NULL || text[0] == '\0') {
		lv_label_set_text(label, text);
		return;
	}
	/* Text without right-to-left letters (dates, paths, codes, addresses)
	 * would be reordered by LVGL from its first neutral character: a mark
	 * at the start of each line keeps it in its own order. */
	bool ltr_only = !has_rtl_letter(text);
	size_t len = strlen(text);
	size_t lines = 1;
	for (size_t i = 0; i < len; i++) {
		lines += text[i] == '\n';
	}
	/* At most one color command per 9 bytes ("#RRGGBB x#"), two marks each. */
	char* out = malloc(len + ((len / 9 + 1) * 2 + lines) * lrm_len + 1);
	if (out == NULL) {
		lv_label_set_text(label, text);
		return;
	}
	size_t o = 0;
	bool line_start = true;
	bool in_command = false;
	for (size_t i = 0; i < len; i++) {
		if (ltr_only && line_start) {
			memcpy(out + o, LRM, lrm_len);
			o += lrm_len;
		}
		line_start = text[i] == '\n';
		if (!in_command && is_color_command(text + i)) {
			const char* end = strchr(text + i + 8, '#');
			size_t inner = end != NULL ? (size_t)(end - (text + i + 8)) : len - i - 8;
			char* value = malloc(inner + 1);
			bool rtl_value = false;
			if (value != NULL) {
				memcpy(value, text + i + 8, inner);
				value[inner] = '\0';
				rtl_value = has_rtl_letter(value);
				free(value);
			}
			if (rtl_value) {
				/* A right-to-left value can't stay next to its command
				 * after reordering: show it without the color. */
				memcpy(out + o, text + i + 8, inner);
				o += inner;
				i += 8 + inner;
				if (end == NULL) {
					break;
				}
				continue;
			}
			memcpy(out + o, LRM, lrm_len);
			o += lrm_len;
			memcpy(out + o, text + i, 8);
			o += 8;
			i += 7;
			in_command = true;
			continue;
		}
		out[o++] = text[i];
		if (in_command && text[i] == '#') {
			memcpy(out + o, LRM, lrm_len);
			o += lrm_len;
			in_command = false;
		}
	}
	out[o] = '\0';
	lv_label_set_text(label, out);
	free(out);
}

const lv_font_t* i18n_font(int size)
{
	if (size < 1 || size > MAX_FONT_SIZE) {
		size = DEFAULT_FONT_SIZE;
	}
	if (fonts[size] != NULL) {
		return fonts[size];
	}
	const lv_font_t* builtin = builtin_font(size);
	fonts[size] = builtin;
	if (!freetype_ready) {
		return builtin;
	}

	char latin_path[160];
	asset_file(latin_path, sizeof(latin_path), FONT_LATIN);
	lv_font_t* latin = load_font(latin_path, size);
	lv_font_t* arabic = load_font(FONT_ARABIC, size);
	lv_font_t* chinese = load_font(FONT_CHINESE, size);

	/* Each font hands the characters it lacks to the next one. The built-in
	 * Montserrat comes last because it holds the LV_SYMBOL_* icons. */
	lv_font_t* chain[] = { latin, arabic, chinese };
	const lv_font_t* next = builtin;
	for (int i = (int)(sizeof(chain) / sizeof(chain[0])) - 1; i >= 0; i--) {
		if (chain[i] != NULL) {
			chain[i]->fallback = next;
			next = chain[i];
		}
	}
	fonts[size] = next;
	return next;
}
