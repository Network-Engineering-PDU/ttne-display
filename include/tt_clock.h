#ifndef TT_CLOCK_H
#define TT_CLOCK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "app/app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Local time on the touchscreen. The system clock is in UTC; the display
 * shows it in the time zone and with the clock (12 or 24-hour) chosen in
 * System Setup > Date & Time. Use from the LVGL thread. */

/* First year of a clock that holds a real time. The board has no backup
 * battery for its clock: after a power loss it starts from the image's
 * build date, until NTP or the user sets it. */
#define TT_CLOCK_MIN_YEAR 2024
/* Last year that can be set: the display's time_t ends in January 2038. */
#define TT_CLOCK_MAX_YEAR 2037

/** @brief Applies the time zone and the 12/24-hour clock. */
void tt_clock_apply(const app_state_datetime_t* datetime);

/** @brief Changes each time the time zone or the clock changes: times
 * already written (alarms) are written again. */
uint32_t tt_clock_revision(void);

/** @brief 12 or 24. */
int tt_clock_hour_format(void);

/** @brief Current local time. Returns false while the clock is not set. */
bool tt_clock_now(struct tm* local);

/**
 * @brief Writes the time of day: "14:30" on the 24-hour clock, "2:30 PM"
 * (AM and PM in the current language) on the 12-hour one.
 *
 * @param[in] seconds  Adds the seconds ("14:30:05").
 */
void tt_clock_format_time(char* dst, size_t size, const struct tm* local,
		bool seconds);

/** @brief Writes the date: "2026/10/30". In right-to-left languages the
 * texts of these functions hold left-to-right marks (U+200E). */
void tt_clock_format_date(char* dst, size_t size, const struct tm* local);

/** @brief Writes the time zone's offset from UTC: "UTC+02:00". */
void tt_clock_format_offset(char* dst, size_t size, const struct tm* local);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TT_CLOCK_H */
