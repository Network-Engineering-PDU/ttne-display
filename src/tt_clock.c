/* tm_gmtoff, setenv */
#define _DEFAULT_SOURCE

#include "tt_clock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "i18n.h"

#define ZONEINFO_DIR "/usr/share/zoneinfo/"

static int hour_format = 24;
static char applied_tz[APP_STATE_TZ_LEN + 1];
static uint32_t revision;

/* LVGL's right-to-left reordering puts the digit groups around ':' and '/'
 * in reverse order ("35:3"); a left-to-right mark (U+200E) at each end
 * keeps the number in order. */
static void ltr_number(char* dst, size_t size, const char* number)
{
	if (i18n_is_rtl()) {
		snprintf(dst, size, "\xE2\x80\x8E%s\xE2\x80\x8E", number);
	} else {
		snprintf(dst, size, "%s", number);
	}
}

void tt_clock_apply(const app_state_datetime_t* datetime)
{
	if (datetime == NULL) {
		return;
	}
	/* ":Area/City" reads the zone file; an unknown name would be taken
	 * as UTC with a wrong abbreviation, so check that the file exists. */
	char path[sizeof(ZONEINFO_DIR) + APP_STATE_TZ_LEN];
	snprintf(path, sizeof(path), ZONEINFO_DIR "%s", datetime->timezone);
	char tz[APP_STATE_TZ_LEN + 1] = ":UTC";
	if (datetime->timezone[0] != '\0' && strstr(datetime->timezone, "..") == NULL &&
			access(path, R_OK) == 0) {
		snprintf(tz, sizeof(tz), ":%s", datetime->timezone);
	}
	int format = datetime->hour_format == 12 ? 12 : 24;
	if (strcmp(tz, applied_tz) != 0 || format != hour_format) {
		revision++;
	}
	snprintf(applied_tz, sizeof(applied_tz), "%s", tz);
	hour_format = format;
	setenv("TZ", tz, 1);
	tzset();
}

uint32_t tt_clock_revision(void)
{
	return revision;
}

int tt_clock_hour_format(void)
{
	return hour_format;
}

bool tt_clock_now(struct tm* local)
{
	time_t now = time(NULL);
	if (localtime_r(&now, local) == NULL) {
		memset(local, 0, sizeof(*local));
		return false;
	}
	return local->tm_year + 1900 >= TT_CLOCK_MIN_YEAR;
}

void tt_clock_format_time(char* dst, size_t size, const struct tm* local,
		bool seconds)
{
	char hm[40];
	char number[48];
	int hour = local->tm_hour;
	if (hour_format == 12) {
		hour = hour % 12 == 0 ? 12 : hour % 12;
	}
	if (seconds) {
		snprintf(hm, sizeof(hm), hour_format == 24 ? "%02d:%02d:%02d" :
				"%d:%02d:%02d", hour, local->tm_min, local->tm_sec);
	} else {
		snprintf(hm, sizeof(hm), hour_format == 24 ? "%02d:%02d" : "%d:%02d",
				hour, local->tm_min);
	}
	ltr_number(number, sizeof(number), hm);
	if (hour_format == 24) {
		snprintf(dst, size, "%s", number);
	} else {
		snprintf(dst, size, local->tm_hour < 12 ? tr("%s AM") : tr("%s PM"),
				number);
	}
}

void tt_clock_format_date(char* dst, size_t size, const struct tm* local)
{
	char date[40];
	snprintf(date, sizeof(date), "%04d/%02d/%02d", local->tm_year + 1900,
			local->tm_mon + 1, local->tm_mday);
	ltr_number(dst, size, date);
}

void tt_clock_format_offset(char* dst, size_t size, const struct tm* local)
{
	long offset = local->tm_gmtoff;
	char sign = offset < 0 ? '-' : '+';
	offset = labs(offset);
	snprintf(dst, size, "UTC%c%02ld:%02ld", sign, offset / 3600,
			(offset % 3600) / 60);
}
