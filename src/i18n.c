#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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
#ifndef FONT_ARABIC
#define FONT_ARABIC FONT_DIR "DejaVuSans.ttf"
#endif
#ifndef FONT_CHINESE
#define FONT_CHINESE FONT_DIR "wqy-zenhei.ttc"
#endif

#define MAX_FONT_SIZE 48
/* Font files (3) times the sizes the screens use, with room to spare */
#define FREETYPE_MAX_FACES 16
/* Size of the text that uses the theme font: LV_FONT_DEFAULT is Montserrat 14. */
#define DEFAULT_FONT_SIZE 14

/* Default translation file, in the app's assets. */
#define DEFAULT_TRANSLATION_FILE "translations.csv"
/* Limits for the translation file: it is read whole into memory. */
#define MAX_FILE_BYTES (2 * 1024 * 1024)
#define MAX_TEXT_BYTES 1024
#define MAX_COLUMNS 64
#define MAX_LANGS 16
#define MAX_CODE_BYTES 16
#define MAX_NAME_BYTES 64

#define BUILTIN_LANGS 5

typedef struct {
	char code[MAX_CODE_BYTES];
	char name[MAX_NAME_BYTES];
	int builtin;	/* i18n_entry_t column, -1 for a language of the file */
} lang_t;

/* Global variables ***********************************************************/

static const lang_t BUILTIN[BUILTIN_LANGS] = {
	{ "en", "English", 0 },
	{ "es", "Español", 1 },
	{ "de", "Deutsch", 2 },
	{ "zh-hans", "中文 (简体)", 3 },
	{ "ar", "العربية", 4 },
};

/* Built-in languages, then the ones the translation file adds. */
static lang_t langs[MAX_LANGS];
static int lang_count;
static int current_lang;

/* I18N_STRINGS indexes sorted by English text, for bsearch(). */
static unsigned* sorted;
/* Texts of the current language from the translation file, by
 * I18N_STRINGS index; NULL keeps the built-in translation. */
static char** file_texts;

/* I18N_CUSTOM_FILE as i18n_init() found it, to notice changes. */
static struct stat custom_stat;
static bool custom_exists;

/* The fonts by size: plain_fonts draw digits as written, fonts in the
 * language's digits (the same fonts unless the language has its own). */
static const lv_font_t* fonts[MAX_FONT_SIZE + 1];
static const lv_font_t* plain_fonts[MAX_FONT_SIZE + 1];
static lv_font_t native_digit_fonts[MAX_FONT_SIZE + 1];
/* First of the language's own digits, 0 for ASCII digits. */
static uint32_t digit_zero;
/* Digits of IP and MAC addresses in label texts, private use characters
 * the fonts draw as ASCII digits: addresses are typed elsewhere as they
 * are. U+E030 is "\xEE\x80\xB0" in UTF-8. */
#define ADDRESS_DIGIT_ZERO 0xE030
static bool freetype_ready;

static lv_theme_t screen_theme;
static lv_style_t screen_style;

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

/* I18N_STRINGS index of an English text, -1 if it isn't there. */
static int find_entry(const char* text)
{
	if (sorted == NULL) {
		return -1;
	}
	const unsigned* found = bsearch(text, sorted, I18N_STRING_COUNT,
			sizeof(*sorted), compare_key);
	return found != NULL ? (int)*found : -1;
}

static const char* builtin_translation(const i18n_entry_t* entry, int column)
{
	switch (column) {
	case 1: return entry->es;
	case 2: return entry->de;
	case 3: return entry->zh;
	case 4: return entry->ar;
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

/* Translation file checks ****************************************************/

static bool utf8_valid(const char* text)
{
	const unsigned char* p = (const unsigned char*)text;
	while (*p != '\0') {
		unsigned char c = *p;
		int extra;
		uint32_t letter;
		if (c < 0x80) {
			p++;
			continue;
		} else if (c >= 0xC2 && c <= 0xDF) {
			extra = 1;
			letter = c & 0x1F;
		} else if (c >= 0xE0 && c <= 0xEF) {
			extra = 2;
			letter = c & 0x0F;
		} else if (c >= 0xF0 && c <= 0xF4) {
			extra = 3;
			letter = c & 0x07;
		} else {
			return false;
		}
		for (int i = 1; i <= extra; i++) {
			if ((p[i] & 0xC0) != 0x80) {
				return false;
			}
			letter = (letter << 6) | (p[i] & 0x3F);
		}
		/* Overlong forms, surrogates and code points past U+10FFFF */
		if ((extra == 2 && letter < 0x800) || (extra == 3 && letter < 0x10000) ||
				(letter >= 0xD800 && letter <= 0xDFFF) || letter > 0x10FFFF) {
			return false;
		}
		p += extra + 1;
	}
	return true;
}

/* Finds the next printf conversion ("%d", "%.1f", "%%"...) in text: returns
 * its start and sets *len, or NULL when there is none. Sets *bad for a '%'
 * that doesn't start a supported conversion ("%n", "%*d", "%1$s"...). */
static const char* next_conversion(const char* text, size_t* len, bool* bad)
{
	const char* start = strchr(text, '%');
	if (start == NULL) {
		return NULL;
	}
	const char* p = start + 1;
	p += strspn(p, "-+ #0");
	p += strspn(p, "0123456789");
	if (*p == '.') {
		p++;
		p += strspn(p, "0123456789");
	}
	if ((p[0] == 'h' && p[1] == 'h') || (p[0] == 'l' && p[1] == 'l')) {
		p += 2;
	} else if (*p == 'h' || *p == 'l' || *p == 'z') {
		p++;
	}
	if (*p == '\0' || strchr("diouxXeEfFgGcs%", *p) == NULL) {
		*bad = true;
		return NULL;
	}
	*len = (size_t)(p + 1 - start);
	return start;
}

/* The text is passed to printf with the English text's arguments: it must
 * have the same conversions in the same order. */
static bool same_conversions(const char* english, const char* text)
{
	for (;;) {
		size_t len_en = 0;
		size_t len_text = 0;
		bool bad = false;
		const char* conv_en = next_conversion(english, &len_en, &bad);
		const char* conv_text = next_conversion(text, &len_text, &bad);
		if (bad) {
			return false;
		}
		if (conv_en == NULL || conv_text == NULL) {
			return conv_en == conv_text;
		}
		if (len_en != len_text || memcmp(conv_en, conv_text, len_en) != 0) {
			return false;
		}
		english = conv_en + len_en;
		text = conv_text + len_text;
	}
}

static int count_char(const char* text, char c)
{
	int count = 0;
	for (; *text != '\0'; text++) {
		count += *text == c;
	}
	return count;
}

/* True when the fonts have every character of a valid UTF-8 text. */
static bool font_has_text(const char* text)
{
	const lv_font_t* font = i18n_font(DEFAULT_FONT_SIZE);
	uint32_t i = 0;
	uint32_t letter;
	while ((letter = _lv_txt_encoded_next(text, &i)) != 0) {
		/* Line breaks and the direction marks LVGL handles itself */
		if (letter == '\n' || letter == 0x200E || letter == 0x200F) {
			continue;
		}
		/* C0 and C1 control characters */
		if (letter < 0x20 || (letter >= 0x7F && letter < 0xA0)) {
			return false;
		}
		lv_font_glyph_dsc_t dsc;
		/* A character no font has resolves to a placeholder box */
		if (!lv_font_get_glyph_dsc(font, &dsc, letter, 0) ||
				dsc.resolved_font == NULL || dsc.is_placeholder) {
			return false;
		}
	}
	return true;
}

/* Why a text of the translation file can't be used, NULL when it can. */
static const char* text_problem(const char* english, const char* text)
{
	if (strlen(text) > MAX_TEXT_BYTES) {
		return "too long";
	}
	if (!utf8_valid(text)) {
		return "not UTF-8";
	}
	if (!same_conversions(english, text)) {
		return "placeholders differ from English";
	}
	if (count_char(english, '#') != count_char(text, '#')) {
		return "colour marks differ from English";
	}
	if (!font_has_text(text)) {
		return "characters the touchscreen can't show";
	}
	return NULL;
}

/* Translation file parsing ***************************************************/

/* Reads a whole file into a NUL-terminated buffer; NULL if it can't. */
static char* read_file(const char* path, size_t* size)
{
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		return NULL;
	}
	char* data = malloc(MAX_FILE_BYTES + 1);
	size_t n = data != NULL ? fread(data, 1, MAX_FILE_BYTES + 1, file) : 0;
	fclose(file);
	if (data == NULL || n > MAX_FILE_BYTES) {
		LV_LOG_WARN("Translation file %s is too large", path);
		free(data);
		return NULL;
	}
	data[n] = '\0';
	*size = n;
	return data;
}

/* The field separator: whichever of ',' ';' and tab appears most in the
 * header line (outside quotes). Excel uses ';' in some regions. */
static char detect_separator(const char* text)
{
	int commas = 0;
	int semicolons = 0;
	int tabs = 0;
	bool quoted = false;
	for (const char* p = text; *p != '\0' && (quoted || (*p != '\n' && *p != '\r')); p++) {
		if (*p == '"') {
			quoted = !quoted;
		} else if (!quoted) {
			commas += *p == ',';
			semicolons += *p == ';';
			tabs += *p == '\t';
		}
	}
	if (semicolons > commas && semicolons >= tabs) {
		return ';';
	}
	return tabs > commas ? '\t' : ',';
}

/**
 * Splits the next CSV record into fields, in place: each field is unquoted
 * ("" is a quote) and ends with '\0'. Fields past max are dropped.
 *
 * @return The number of fields, 0 at the end of the text.
 */
static int next_record(char** pos, char* end, char separator, char** fields, int max)
{
	char* p = *pos;
	if (p >= end) {
		return 0;
	}
	int count = 0;
	for (;;) {
		char* field = p;
		char* out = p;
		if (*p == '"') {
			p++;
			while (p < end) {
				if (*p == '"') {
					if (p + 1 < end && p[1] == '"') {
						*out++ = '"';
						p += 2;
						continue;
					}
					p++;
					break;
				}
				*out++ = *p++;
			}
		}
		while (p < end && *p != separator && *p != '\n' && *p != '\r') {
			*out++ = *p++;
		}
		char stop = p < end ? *p : '\n';
		*out = '\0';
		if (count < max) {
			fields[count] = field;
		}
		count++;
		/* *p may be the '\0' just written: go by the saved character */
		if (stop == separator) {
			p++;
			continue;
		}
		if (p < end) {
			p++;
			if (stop == '\r' && p < end && *p == '\n') {
				p++;
			}
		}
		break;
	}
	*pos = p;
	return count < max ? count : max;
}

/* Excel writes a line break inside a cell as "\r\n": keeps the '\n'. */
static void drop_carriage_returns(char* text)
{
	char* out = text;
	for (char* p = text; *p != '\0'; p++) {
		if (!(p[0] == '\r' && p[1] == '\n')) {
			*out++ = *p;
		}
	}
	*out = '\0';
}

static void trim(char* text)
{
	size_t len = strlen(text);
	while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t')) {
		text[--len] = '\0';
	}
	size_t start = strspn(text, " \t");
	memmove(text, text + start, len - start + 1);
}

/* Splits a column title "Name (code)" in place. False if it has no valid
 * language code: such columns (ID, Screen, notes) are ignored. */
static bool parse_column_title(char* title, char** name, char** code)
{
	trim(title);
	size_t len = strlen(title);
	char* open = strrchr(title, '(');
	if (len < 4 || title[len - 1] != ')' || open == NULL) {
		return false;
	}
	title[len - 1] = '\0';
	*open = '\0';
	char* c = open + 1;
	trim(c);
	for (char* p = c; *p != '\0'; p++) {
		if (*p >= 'A' && *p <= 'Z') {
			*p = (char)(*p - 'A' + 'a');
		}
	}
	/* ll, lll, ll-xx... (BCP 47 style: "fr", "pt-br", "zh-hant") */
	size_t letters = strspn(c, "abcdefghijklmnopqrstuvwxyz");
	if (letters < 2 || letters > 3) {
		return false;
	}
	if (c[letters] == '-') {
		size_t sub = strspn(c + letters + 1, "abcdefghijklmnopqrstuvwxyz0123456789");
		if (sub < 2 || sub > 8 || c[letters + 1 + sub] != '\0') {
			return false;
		}
	} else if (c[letters] != '\0') {
		return false;
	}
	trim(title);
	*name = title;
	*code = c;
	return true;
}

static int find_lang(const char* code)
{
	for (int i = 0; code != NULL && i < lang_count; i++) {
		if (strcmp(code, langs[i].code) == 0) {
			return i;
		}
	}
	return -1;
}

/* Adds the languages of the file's header: column_lang[i] is the language
 * index of column i, -1 for other columns. */
static void read_header(char** fields, int count, int* column_lang)
{
	for (int i = 0; i < count; i++) {
		char* name;
		char* code;
		column_lang[i] = -1;
		if (!parse_column_title(fields[i], &name, &code) ||
				strlen(code) >= MAX_CODE_BYTES) {
			continue;
		}
		int lang = find_lang(code);
		if (lang < 0 && lang_count < MAX_LANGS) {
			lang = lang_count++;
			snprintf(langs[lang].code, sizeof(langs[lang].code), "%s", code);
			/* The name is shown in the dropdown: the code if it can't be. */
			bool name_ok = name[0] != '\0' && strlen(name) < MAX_NAME_BYTES &&
					utf8_valid(name) && strchr(name, '\n') == NULL &&
					font_has_text(name);
			snprintf(langs[lang].name, sizeof(langs[lang].name), "%s",
					name_ok ? name : code);
			langs[lang].builtin = -1;
		}
		/* The first column of a language wins */
		for (int j = 0; lang >= 0 && j < i; j++) {
			if (column_lang[j] == lang) {
				lang = -1;
			}
		}
		column_lang[i] = lang;
	}
}

/* Keeps the current language's texts from the file's rows. */
static void read_texts(char** pos, char* end, char separator, int english_column,
		int lang_column, const char* path)
{
	char* fields[MAX_COLUMNS];
	int count;
	int used = 0;
	int rejected = 0;
	file_texts = calloc(I18N_STRING_COUNT, sizeof(*file_texts));
	if (file_texts == NULL) {
		return;
	}
	while ((count = next_record(pos, end, separator, fields, MAX_COLUMNS)) > 0) {
		if (count <= english_column || count <= lang_column) {
			continue;
		}
		char* english = fields[english_column];
		char* text = fields[lang_column];
		drop_carriage_returns(english);
		drop_carriage_returns(text);
		int entry = find_entry(english);
		if (entry < 0 || text[0] == '\0' || file_texts[entry] != NULL) {
			continue;
		}
		/* The built-in text (the default file holds them all) is used as it
		 * is: checking every character of it is slow for Chinese. */
		int column = langs[current_lang].builtin;
		const char* builtin = column > 0 ?
				builtin_translation(&I18N_STRINGS[entry], column) : NULL;
		if (builtin != NULL && strcmp(builtin, text) == 0) {
			used++;
			continue;
		}
		const char* problem = text_problem(english, text);
		if (problem != NULL) {
			LV_LOG_WARN("Translation of \"%s\" ignored: %s", english, problem);
			rejected++;
			continue;
		}
		file_texts[entry] = strdup(text);
		used++;
	}
	LV_LOG_USER("Translations: %d texts from %s, %d ignored", used, path, rejected);
}

/* Reads the custom translation file, else the default one. */
static void load_translation_file(const char* code)
{
	char path[160];
	custom_exists = stat(I18N_CUSTOM_FILE, &custom_stat) == 0;
	if (custom_exists) {
		snprintf(path, sizeof(path), "%s", I18N_CUSTOM_FILE);
	} else {
		asset_file(path, sizeof(path), DEFAULT_TRANSLATION_FILE);
	}
	size_t size = 0;
	char* data = read_file(path, &size);
	if (data == NULL) {
		LV_LOG_WARN("No translation file %s; using built-in texts", path);
		return;
	}
	char* pos = data;
	char* end = data + size;
	if (size >= 3 && memcmp(data, "\xEF\xBB\xBF", 3) == 0) {
		pos += 3;	/* UTF-8 byte order mark, as Excel writes it */
	}
	char separator = detect_separator(pos);
	char* fields[MAX_COLUMNS];
	int column_lang[MAX_COLUMNS];
	int count = next_record(&pos, end, separator, fields, MAX_COLUMNS);
	read_header(fields, count, column_lang);

	current_lang = find_lang(code);
	if (current_lang < 0) {
		current_lang = 0;
	}
	int english_column = -1;
	int lang_column = -1;
	for (int i = 0; i < count; i++) {
		if (column_lang[i] == 0 && english_column < 0) {
			english_column = i;
		} else if (column_lang[i] == current_lang && current_lang != 0 && lang_column < 0) {
			lang_column = i;
		}
	}
	if (english_column < 0) {
		LV_LOG_WARN("Translation file %s has no English (en) column", path);
	} else if (lang_column >= 0) {
		read_texts(&pos, end, separator, english_column, lang_column, path);
	}
	free(data);
}

/* Fonts and layout *************************************************************/

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

/* First of the digits a language writes numbers with, 0 for ASCII digits:
 * Arabic-Indic digits for Arabic, the Persian forms for Persian and Urdu. */
static uint32_t native_digit_zero(const char* code)
{
	if (code == NULL) {
		return 0;
	}
	size_t len = strcspn(code, "-");
	if (len == 2 && strncmp(code, "ar", 2) == 0) {
		return 0x0660;
	}
	if (len == 2 && (strncmp(code, "fa", 2) == 0 || strncmp(code, "ur", 2) == 0)) {
		return 0x06F0;
	}
	return 0;
}

/* Public functions ***********************************************************/

void i18n_init(const char* code)
{
	memcpy(langs, BUILTIN, sizeof(BUILTIN));
	lang_count = BUILTIN_LANGS;
	current_lang = find_lang(code);
	if (current_lang < 0) {
		current_lang = 0;
	}

	free(sorted);
	sorted = malloc(I18N_STRING_COUNT * sizeof(*sorted));
	if (sorted != NULL) {
		for (unsigned i = 0; i < I18N_STRING_COUNT; i++) {
			sorted[i] = i;
		}
		qsort(sorted, I18N_STRING_COUNT, sizeof(*sorted), compare_index);
	}

	/* Every font file at every size is a face for FreeType's cache: room
	 * for all of them. With its default (2) the Chinese text, which falls
	 * through the Latin and Arabic fonts first, reopened the 16 MB Chinese
	 * font for most characters and the screens took seconds to build. */
	freetype_ready = lv_freetype_init(FREETYPE_MAX_FACES, FREETYPE_MAX_FACES,
			LV_FREETYPE_CACHE_SIZE) == LV_RES_OK;
	if (!freetype_ready) {
		LV_LOG_WARN("FreeType could not start; using built-in fonts");
	}
	/* Before any font is made: the fonts draw the language's digits */
	digit_zero = native_digit_zero(code);
	/* After the fonts: the texts are checked against them. */
	load_translation_file(code);
	if (native_digit_zero(langs[current_lang].code) != digit_zero) {
		/* The language isn't there after all (English instead) */
		digit_zero = 0;
		for (int size = 1; size <= MAX_FONT_SIZE; size++) {
			fonts[size] = plain_fonts[size];
		}
	}
	LV_LOG_USER("Language: %s", langs[current_lang].code);
}

bool i18n_file_changed(void)
{
	struct stat now;
	bool exists = stat(I18N_CUSTOM_FILE, &now) == 0;
	if (exists != custom_exists) {
		return true;
	}
	/* The API replaces the file by renaming a new one over it. */
	return exists && (now.st_ino != custom_stat.st_ino ||
			now.st_size != custom_stat.st_size ||
			now.st_mtime != custom_stat.st_mtime);
}

const char* tr(const char* text)
{
	if (text == NULL || current_lang == 0) {
		return text;
	}
	int entry = find_entry(text);
	if (entry < 0) {
		return text;
	}
	if (file_texts != NULL && file_texts[entry] != NULL) {
		return file_texts[entry];
	}
	int column = langs[current_lang].builtin;
	const char* translated = column > 0 ?
			builtin_translation(&I18N_STRINGS[entry], column) : NULL;
	return translated != NULL ? translated : text;
}

int i18n_lang_count(void)
{
	return lang_count;
}

int i18n_current_lang(void)
{
	return current_lang;
}

const char* i18n_lang_code(int index)
{
	return index >= 0 && index < lang_count ? langs[index].code : langs[0].code;
}

const char* i18n_native_name(int index)
{
	return index >= 0 && index < lang_count ? langs[index].name : langs[0].name;
}

const char* i18n_dropdown_options(void)
{
	static char options[MAX_LANGS * MAX_NAME_BYTES];
	options[0] = '\0';
	for (int i = 0; i < lang_count; i++) {
		if (i > 0) {
			strncat(options, "\n", sizeof(options) - strlen(options) - 1);
		}
		strncat(options, langs[i].name, sizeof(options) - strlen(options) - 1);
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
	/* Right-to-left scripts: Arabic, Persian, Urdu, Pashto, Sorani,
	 * Sindhi, Uyghur, Hebrew, Yiddish, Dhivehi. */
	static const char* const RTL[] = {
		"ar", "fa", "ur", "ps", "ckb", "sd", "ug", "he", "iw", "yi", "dv",
	};
	const char* code = langs[current_lang].code;
	size_t len = strcspn(code, "-");
	for (size_t i = 0; i < sizeof(RTL) / sizeof(RTL[0]); i++) {
		if (strlen(RTL[i]) == len && strncmp(code, RTL[i], len) == 0) {
			return true;
		}
	}
	return false;
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

static void label_set_text(lv_obj_t* label, const char* text);
static bool is_color_command(const char* p);

static bool is_hex(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/* Length of an IPv4 address ("192.168.1.10") at text, 0 if there is none. */
static size_t ipv4_length(const char* text)
{
	size_t i = 0;
	for (int part = 0; part < 4; part++) {
		size_t digits = 0;
		while (digits < 3 && text[i] >= '0' && text[i] <= '9') {
			i++;
			digits++;
		}
		if (digits == 0 || (text[i] >= '0' && text[i] <= '9')) {
			return 0;
		}
		if (part < 3) {
			if (text[i] != '.') {
				return 0;
			}
			i++;
		}
	}
	return i;
}

/* Length of a MAC address ("C2:03:03:00:19:60") at text, 0 if none. */
static size_t mac_length(const char* text)
{
	for (int i = 0; i < 17; i++) {
		bool separator = i % 3 == 2;
		if (separator ? (text[i] != ':' && text[i] != '-') : !is_hex(text[i])) {
			return 0;
		}
	}
	return is_hex(text[17]) ? 0 : 17;
}

/* Characters of a code such as "VAR-SOM-MX7", "RS-485", "UTC+00", "V2c". */
static bool is_code_char(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			c == '-' || c == '_' || c == '.' || c == ':' || c == '+' || c == '/';
}

/* Length of the address or code at text whose digits stay ASCII: an IPv4
 * or MAC address, or a word mixing Latin letters and digits (serial and
 * part numbers, model names, versions like "1486378-dirty"). 0 if none. */
static size_t protected_length(const char* text)
{
	size_t len = ipv4_length(text);
	if (len == 0) {
		len = mac_length(text);
	}
	if (len != 0) {
		return len;
	}
	bool letter = false;
	bool digit = false;
	for (len = 0; is_code_char(text[len]); len++) {
		char c = text[len];
		letter |= (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
		digit |= c >= '0' && c <= '9';
	}
	return letter && digit ? len : 0;
}

/* A copy of text whose addresses and codes (see protected_length()) keep
 * ASCII digits in languages with their own digits, as they are written or
 * typed elsewhere as they are (see ADDRESS_DIGIT_ZERO). NULL if there is
 * nothing to change. */
static char* protect_addresses(const char* text)
{
	if (digit_zero == 0 || text == NULL) {
		return NULL;
	}
	size_t len = strlen(text);
	char* out = NULL;
	size_t o = 0;
	for (size_t i = 0; i < len; ) {
		/* A color command ("#A1C61A ") is copied as it is */
		size_t keep = is_color_command(text + i) ? 8 : 0;
		bool start = i == 0 || !is_code_char(text[i - 1]);
		size_t code = keep == 0 && start ? protected_length(text + i) : 0;
		if (code == 0) {
			if (keep == 0) {
				/* Skip the rest of the word: codes start at a word */
				keep = 1;
				while (is_code_char(text[i]) && i + keep < len &&
						is_code_char(text[i + keep])) {
					keep++;
				}
			}
			if (out != NULL) {
				memcpy(out + o, text + i, keep);
				o += keep;
			}
			i += keep;
			continue;
		}
		if (out == NULL) {
			out = malloc(len * 3 + 1);
			if (out == NULL) {
				return NULL;
			}
			memcpy(out, text, i);
			o = i;
		}
		for (size_t end = i + code; i < end; i++) {
			if (text[i] >= '0' && text[i] <= '9') {
				out[o++] = '\xEE';
				out[o++] = '\x80';
				out[o++] = (char)(0xB0 + (text[i] - '0'));
			} else {
				out[o++] = text[i];
			}
		}
	}
	if (out != NULL) {
		out[o] = '\0';
	}
	return out;
}

void i18n_label_set_text(lv_obj_t* label, const char* text)
{
	char* protected_text = protect_addresses(text);
	if (protected_text != NULL) {
		label_set_text(label, protected_text);
		free(protected_text);
		return;
	}
	label_set_text(label, text);
}

static void label_set_text(lv_obj_t* label, const char* text)
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

/* The font chain at a size, digits as written: Montserrat for Latin script,
 * then DejaVu Sans (Arabic) and WenQuanYi Zen Hei (Chinese). */
static const lv_font_t* plain_font(int size)
{
	if (plain_fonts[size] != NULL) {
		return plain_fonts[size];
	}
	const lv_font_t* builtin = builtin_font(size);
	plain_fonts[size] = builtin;
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
	plain_fonts[size] = next;
	return next;
}

/* The language's digit for an ASCII digit (and its percent sign for '%');
 * an address digit (see protect_addresses()) back to its ASCII digit;
 * other letters unchanged. */
static uint32_t native_digit(uint32_t letter)
{
	if (digit_zero != 0 && letter >= '0' && letter <= '9') {
		return digit_zero + (letter - '0');
	}
	if (digit_zero != 0 && letter == '%') {
		return 0x066A;	/* ARABIC PERCENT SIGN */
	}
	if (letter >= ADDRESS_DIGIT_ZERO && letter <= ADDRESS_DIGIT_ZERO + 9) {
		return '0' + (letter - ADDRESS_DIGIT_ZERO);
	}
	return letter;
}

/* A font that draws the ASCII digits of a text as the language's digits.
 * Texts keep ASCII digits, so values typed or shown are still parsed as
 * numbers; only their drawing changes. dsc is the plain font chain. */
static bool native_digits_glyph_dsc(const lv_font_t* font, lv_font_glyph_dsc_t* dsc,
		uint32_t letter, uint32_t letter_next)
{
	return lv_font_get_glyph_dsc((const lv_font_t*)font->dsc, dsc,
			native_digit(letter), native_digit(letter_next));
}

static const uint8_t* native_digits_glyph_bitmap(const lv_font_t* font, uint32_t letter)
{
	/* LVGL asks the font that resolved the glyph, this one, for the bitmap.
	 * The glyph is looked up again in the chain: FreeType hands out the
	 * bitmap of the glyph it looked up last. */
	lv_font_glyph_dsc_t dsc;
	uint32_t native = native_digit(letter);
	if (!lv_font_get_glyph_dsc((const lv_font_t*)font->dsc, &dsc, native, 0) ||
			dsc.resolved_font == NULL) {
		return NULL;
	}
	return lv_font_get_glyph_bitmap(dsc.resolved_font, native);
}

const lv_font_t* i18n_font(int size)
{
	if (size < 1 || size > MAX_FONT_SIZE) {
		size = DEFAULT_FONT_SIZE;
	}
	if (fonts[size] != NULL) {
		return fonts[size];
	}
	const lv_font_t* plain = plain_font(size);
	fonts[size] = plain;
	if (digit_zero != 0 && plain != builtin_font(size)) {
		lv_font_t* native = &native_digit_fonts[size];
		*native = *plain;
		native->get_glyph_dsc = native_digits_glyph_dsc;
		native->get_glyph_bitmap = native_digits_glyph_bitmap;
		native->dsc = plain;
		native->fallback = NULL;
		fonts[size] = native;
	}
	return fonts[size];
}

const lv_font_t* i18n_font_western_digits(int size)
{
	if (size < 1 || size > MAX_FONT_SIZE) {
		size = DEFAULT_FONT_SIZE;
	}
	i18n_font(size);
	return plain_font(size);
}

void i18n_format_value(char* dst, size_t size, uint32_t color, const char* value,
		const char* unit)
{
	if (unit == NULL || unit[0] == '\0') {
		snprintf(dst, size, "#%06X %s#", (unsigned)color, value);
	} else if (i18n_is_rtl()) {
		snprintf(dst, size, "#%06X %s# %s", (unsigned)color, value, unit);
	} else {
		snprintf(dst, size, "#%06X %s %s#", (unsigned)color, value, unit);
	}
}

void i18n_textarea_fix_placeholder(lv_obj_t* textarea)
{
	if (i18n_is_rtl()) {
		lv_obj_set_style_text_align(textarea, LV_TEXT_ALIGN_LEFT,
				LV_PART_TEXTAREA_PLACEHOLDER);
	}
}

bool i18n_has_western_digits(lv_obj_t* obj)
{
	const lv_font_t* font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
	for (int size = 1; digit_zero != 0 && size <= MAX_FONT_SIZE; size++) {
		if (plain_fonts[size] == font && fonts[size] != font) {
			return true;
		}
	}
	return false;
}

void i18n_western_digits(lv_obj_t* obj)
{
	if (digit_zero == 0) {
		return;
	}
	const lv_font_t* font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
	for (int size = 1; size <= MAX_FONT_SIZE; size++) {
		if (fonts[size] == font && plain_fonts[size] != font) {
			lv_obj_set_style_text_font(obj, plain_fonts[size], 0);
			return;
		}
	}
}
