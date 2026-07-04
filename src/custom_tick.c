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
	struct timespec now;
	uint64_t now_ms;

	clock_gettime(CLOCK_MONOTONIC, &now);
	now_ms = (uint64_t)now.tv_sec * 1000U +
			(uint64_t)now.tv_nsec / 1000000U;
	if (start_ms == 0) {
		start_ms = now_ms;
	}

	return (uint32_t)(now_ms - start_ms);
}

#endif // SIMULATOR_ENABLED
