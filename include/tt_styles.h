#ifndef SCR_TT_STYLES_H
#define SCR_TT_STYLES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

extern lv_style_t btn_style;
extern lv_style_t btn_hover_style;
extern lv_style_t btn_press_style;
extern lv_style_t btn_err_style;
extern lv_style_t cont_style;
extern lv_style_t txt_style;
extern lv_style_t spinner_style;
extern lv_style_t spinner_inline_style;
extern lv_style_t spinner_inline_bg_style;
extern lv_style_t header_style;
extern lv_style_t alarm_cont_style;
extern lv_style_t invisible_cont_style;

void tt_styles_init();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_TT_STYLES_H */
