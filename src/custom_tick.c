typedef int make_iso_compilers_happy; // Avoid Wpedantic warning

#ifndef SIMULATOR_ENABLED

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "lvgl/lvgl.h"

#include "custom_tick.h"

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
	static uint64_t start_ms = 0;
	if(start_ms == 0) {
		struct timespec ts_start;
		clock_gettime(CLOCK_MONOTONIC, &ts_start);
		start_ms = (ts_start.tv_sec * 1000) + (ts_start.tv_nsec / 1000000);
	}

	struct timespec ts_now;
	clock_gettime(CLOCK_MONOTONIC, &ts_now);
	uint64_t now_ms;
	now_ms = (ts_now.tv_sec * 1000) + (ts_now.tv_nsec / 1000000);

	uint32_t time_ms = now_ms - start_ms;
	return time_ms;
}

#endif // SIMULATOR_ENABLED
