#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/
#ifdef SIMULATOR_ENABLED
#include <SDL2/SDL.h>
#include "lv_drivers/sdl/sdl.h"
#else
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#endif
#include "../lvgl/lvgl.h"

#include "ttne_display.h"
#include "config.h"

#ifndef SIMULATOR_ENABLED
#define DISP_BUF_SIZE (128 * 1024)
/*A small buffer for LittlevGL to draw the screen's content*/
static lv_color_t buf[DISP_BUF_SIZE];
static lv_disp_draw_buf_t disp_buf;
#endif

static char* prog_name;

static void hal_init(void);

#ifdef SIMULATOR_ENABLED 

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static void hal_init(void)
{
	/* Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
	sdl_init();

	/* Create a display buffer */
	static lv_disp_draw_buf_t disp_buf1;
	static lv_color_t buf1_1[SDL_HOR_RES * 100];
	lv_disp_draw_buf_init(&disp_buf1, buf1_1, NULL, SDL_HOR_RES * 100);

	/* Create a display*/
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv); /* Basic initialization */
	disp_drv.draw_buf = &disp_buf1;
	disp_drv.flush_cb = sdl_display_flush;
	disp_drv.hor_res = SDL_HOR_RES;
	disp_drv.ver_res = SDL_VER_RES;
	disp_drv.sw_rotate = 1;

	lv_disp_t* disp = lv_disp_drv_register(&disp_drv);
	lv_disp_set_default(disp);
	lv_disp_set_rotation(disp, LV_DISP_ROT_270); 

	lv_theme_t* th = lv_theme_default_init(disp,
			lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
			LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
	lv_disp_set_theme(disp, th);

	lv_group_t* g = lv_group_create();
	lv_group_set_default(g);

	/* Add the mouse as input device
	 * Use the 'mouse' driver which reads the PC's mouse */
	static lv_indev_drv_t indev_drv_1;
	lv_indev_drv_init(&indev_drv_1); /* Basic initialization */
	indev_drv_1.type = LV_INDEV_TYPE_POINTER;

	/* This function will be called periodically (by the library) to get the
	 * mouse position and state */
	indev_drv_1.read_cb = sdl_mouse_read;
	lv_indev_drv_register(&indev_drv_1);

	static lv_indev_drv_t indev_drv_2;
	lv_indev_drv_init(&indev_drv_2); /* Basic initialization */
	indev_drv_2.type = LV_INDEV_TYPE_KEYPAD;
	indev_drv_2.read_cb = sdl_keyboard_read;
	lv_indev_t *kb_indev = lv_indev_drv_register(&indev_drv_2);
	lv_indev_set_group(kb_indev, g);

	static lv_indev_drv_t indev_drv_3;
	lv_indev_drv_init(&indev_drv_3); /* Basic initialization */
	indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
	indev_drv_3.read_cb = sdl_mousewheel_read;
	lv_indev_t * enc_indev = lv_indev_drv_register(&indev_drv_3);
	lv_indev_set_group(enc_indev, g);
}

#else

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static void hal_init(void)
{
	/* Linux frame buffer device init */
	fbdev_init();

	/* Initialize a descriptor for the buffer */
	lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

	/* Initialize and register a display driver */
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.rotated = LV_DISP_ROT_NONE;
	disp_drv.sw_rotate = 1;
	disp_drv.draw_buf = &disp_buf;
	disp_drv.flush_cb = fbdev_flush;
	//disp_drv.hor_res = 320;
	//disp_drv.ver_res = 240;
	disp_drv.hor_res = 240;
	disp_drv.ver_res = 320;
	lv_disp_drv_register(&disp_drv);

	evdev_init();
	static lv_indev_drv_t indev_drv_1;
	lv_indev_drv_init(&indev_drv_1); /* Basic initialization */
	indev_drv_1.type = LV_INDEV_TYPE_POINTER;

	/* This function will be called periodically (by the library) to get the
	 * mouse position and state */
	indev_drv_1.read_cb = evdev_read;
	lv_indev_t* mouse_indev = lv_indev_drv_register(&indev_drv_1);
	(void) mouse_indev;
	//LV_IMG_DECLARE(mouse_cursor_icon);
	//lv_obj_t* cursor_obj = lv_img_create(lv_scr_act());
	//lv_img_set_src(cursor_obj, &mouse_cursor_icon);
	//lv_indev_set_cursor(mouse_indev, cursor_obj);
}

#endif

void reset_program()
{
	LV_LOG_USER("Reset");
	char *args[] = {prog_name, NULL};
	execv(args[0], args);
}


int main(int argc, char **argv)
{
	(void) argc; /* Unused */

	prog_name = malloc(strlen(argv[0]) + 1);
	strcpy(prog_name, argv[0]);

	/* Initialize LVGL */
	lv_init();

	/* Initialize the HAL (display, input devices, tick) for LVGL */
	hal_init();

	ttne_display();

	while (1) {
			/* Periodically call the lv_task handler.
			 * It could be done in a timer interrupt or an OS task too.*/
			lv_timer_handler();
			int inactivity_time = config_get_inactivity_time() * 60 * 1000; // In us
			if ((int)lv_disp_get_inactive_time(NULL) > inactivity_time) {
				ttne_display_idle_cb();
			}
			usleep(5 * 1000);
	}

	return 0;
}
