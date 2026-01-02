#include "tt_styles.h"
#include "tt_colors.h"
#include "utils.h"

lv_style_t btn_style;
lv_style_t btn_hover_style;
lv_style_t btn_press_style;
lv_style_t btn_err_style;
lv_style_t cont_style;
lv_style_t txt_style;
lv_style_t spinner_style;
lv_style_t spinner_inline_style;
lv_style_t spinner_inline_bg_style;
lv_style_t alarm_cont_style;
lv_style_t header_style;
lv_style_t invisible_cont_style;

void tt_styles_init()
{
	lv_style_init(&btn_style);
	lv_style_set_pad_all(&btn_style, 5);
	lv_style_set_border_width(&btn_style, 2);
	lv_style_set_border_color(&btn_style, lv_color_white());
	lv_style_set_bg_color(&btn_style, lv_color_hex(TT_COLOR_BG3));

	lv_style_init(&btn_hover_style);
	lv_style_set_bg_color(&btn_hover_style, lv_color_hex(TT_COLOR_DARK_GREEN_NE));

	lv_style_init(&btn_press_style);
	lv_style_set_bg_color(&btn_press_style, lv_color_hex(TT_COLOR_GREEN_NE));

	lv_style_init(&btn_err_style);
	lv_style_set_bg_color(&btn_err_style, lv_color_hex(TT_COLOR_ERROR));

	lv_style_init(&cont_style);
	lv_style_set_pad_all(&cont_style, 5);
	lv_style_set_border_width(&cont_style, 2);
	lv_style_set_border_color(&cont_style, lv_color_hex(TT_COLOR_DARK_GREEN_NE));
	lv_style_set_bg_color(&cont_style, lv_color_hex(TT_COLOR_BG2));

	lv_style_init(&txt_style);
	lv_style_set_border_width(&txt_style, 2);
	lv_style_set_border_color(&txt_style, lv_color_hex3(0xFFF));
	lv_style_set_bg_color(&txt_style, lv_color_hex(TT_COLOR_BG3));

	lv_style_init(&spinner_style);
	lv_style_set_arc_color(&spinner_style, lv_color_hex(TT_COLOR_GREEN_NE));

	lv_style_init(&spinner_inline_style);
	lv_style_set_arc_color(&spinner_inline_style,
			lv_color_hex(TT_COLOR_GREEN_NE));
	lv_style_set_arc_width(&spinner_inline_style, 3);

	lv_style_init(&spinner_inline_bg_style);
	lv_style_set_arc_width(&spinner_inline_bg_style, 3);

	lv_style_init(&alarm_cont_style);
	lv_style_set_pad_all(&alarm_cont_style, 5);
	lv_style_set_bg_color(&alarm_cont_style, lv_color_hex(TT_COLOR_BG3));
	lv_style_set_border_color(&alarm_cont_style, lv_color_hex(TT_COLOR_ERROR));

	lv_style_init(&header_style);
	lv_style_set_bg_opa(&header_style, LV_OPA_100);
	lv_style_set_bg_color(&header_style, lv_color_hex(TT_COLOR_HEADER));

	lv_style_init(&alarm_cont_style);
	lv_style_set_pad_all(&alarm_cont_style, 5);
	//lv_style_set_bg_color(&alarm_cont_style, lv_color_hex3(0x333));
	lv_style_set_bg_opa(&alarm_cont_style, LV_OPA_0);
	lv_style_set_border_color(&alarm_cont_style, lv_color_hex(0xff2943));

	lv_style_init(&invisible_cont_style);
	lv_style_set_pad_all(&invisible_cont_style, 0);
	lv_style_set_bg_opa(&invisible_cont_style, LV_OPA_0);
	lv_style_set_border_width(&invisible_cont_style, 0);
}
