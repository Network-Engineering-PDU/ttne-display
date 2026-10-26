#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "screenshot.h"

/* Global variables ***********************************************************/

static volatile sig_atomic_t requested;

/* Function definitions *******************************************************/

static void sigusr1_handler(int sig)
{
	(void)sig;
	requested = 1;
}

static void rgb565_to_rgb(uint16_t c, uint8_t* rgb)
{
	rgb[0] = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
	rgb[1] = (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
	rgb[2] = (uint8_t)((c & 0x1F) * 255 / 31);
}

/* Captures obj into a malloc'd buffer: a full-screen image is too large to
 * take reliably from LVGL's own fixed memory pool. */
static lv_img_dsc_t* take(lv_obj_t* obj, lv_img_cf_t cf, lv_img_dsc_t* dsc)
{
	uint32_t size = lv_snapshot_buf_size_needed(obj, cf);
	void* buf = size > 0 ? malloc(size) : NULL;
	if (buf == NULL) {
		return NULL;
	}
	if (lv_snapshot_take_to_buf(obj, cf, dsc, buf, size) != LV_RES_OK) {
		free(buf);
		return NULL;
	}
	return dsc;
}

static void save(void)
{
	lv_obj_t* screen = lv_scr_act();
	lv_obj_t* top = lv_layer_top();
	lv_img_dsc_t base_dsc;
	lv_img_dsc_t overlay_dsc;
	lv_img_dsc_t* base = take(screen, LV_IMG_CF_TRUE_COLOR, &base_dsc);
	if (base == NULL) {
		LV_LOG_USER("Screenshot: could not capture the screen");
		return;
	}
	/* Popups live on the top layer, drawn over the screen with alpha. */
	lv_img_dsc_t* overlay = lv_obj_get_child_cnt(top) > 0
			? take(top, LV_IMG_CF_TRUE_COLOR_ALPHA, &overlay_dsc) : NULL;

	uint32_t w = base->header.w;
	uint32_t h = base->header.h;
	FILE* f = fopen(SCREENSHOT_FILE ".tmp", "wb");
	if (f != NULL) {
		fprintf(f, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
		const uint16_t* px = (const uint16_t*)base->data;
		for (uint32_t y = 0; y < h; y++) {
			for (uint32_t x = 0; x < w; x++) {
				uint8_t rgb[3];
				rgb565_to_rgb(px[y * w + x], rgb);
				if (overlay != NULL && x < overlay->header.w && y < overlay->header.h) {
					/* TRUE_COLOR_ALPHA at 16 bits: 2 color bytes, 1 alpha byte. */
					const uint8_t* o = overlay->data + (y * overlay->header.w + x) * 3;
					uint8_t orgb[3];
					rgb565_to_rgb((uint16_t)(o[0] | (o[1] << 8)), orgb);
					for (int i = 0; i < 3; i++) {
						rgb[i] = (uint8_t)((orgb[i] * o[2] + rgb[i] * (255 - o[2])) / 255);
					}
				}
				fwrite(rgb, 1, 3, f);
			}
		}
		fclose(f);
		rename(SCREENSHOT_FILE ".tmp", SCREENSHOT_FILE);
		LV_LOG_USER("Screenshot saved to " SCREENSHOT_FILE);
	}
	free((void*)base->data);
	if (overlay != NULL) {
		free((void*)overlay->data);
	}
}

/* Public functions ***********************************************************/

void screenshot_init(void)
{
	signal(SIGUSR1, sigusr1_handler);
}

void screenshot_process(void)
{
	if (requested) {
		requested = 0;
		save();
	}
}
