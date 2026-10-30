#ifndef PINYIN_DICT_H
#define PINYIN_DICT_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Pinyin dictionary of the Chinese keyboard (src/pinyin_dict.c): the 3755
 * common Simplified Chinese characters by syllable, most frequent first. */
extern lv_pinyin_dict_t pinyin_dict[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PINYIN_DICT_H */
