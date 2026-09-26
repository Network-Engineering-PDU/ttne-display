#ifndef ALARMS_H
#define ALARMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <time.h>

#include "app/app_state.h"

#define MAX_ALARMS 100

typedef enum {
	ALARM_INFO = 0,
	ALARM_WARNING = 1,
	ALARM_ERROR = 2,
	ALARM_CRITICAL_ERROR = 3,
} alarms_types_t;

typedef struct alarm_desc_t {
	alarms_types_t type;
	char time[24];
	char desc[64];
	char path[48];
	char code[8];
	bool ack;
} alarm_desc_t;

typedef struct alarms_t {
	alarm_desc_t alarms[MAX_ALARMS];
	int n;
} alarms_t;

void alarms_init(alarms_t* self);

/**
 * @brief Adds an alarm. Strings are copied. Ignored if the list is full.
 */
void alarms_new(alarms_t* self, const alarm_desc_t* alarm);

/**
 * @brief Re-evaluates the active alarms from the current PDU state.
 *
 * Alarms that stay active keep their first-seen time and acknowledged flag;
 * alarms whose condition cleared are dropped.
 *
 * @param[in] self          Alarm list.
 * @param[in] snapshot      Current application state.
 * @param[in] comm_error    True if the last power data request failed.
 * @param[in] now           Current time.
 * @return True if the list changed (needs redraw).
 */
bool alarms_update(alarms_t* self, const app_state_snapshot_t* snapshot,
		bool comm_error, time_t now);

/** @brief Acknowledges (hides) an alarm until its condition clears. */
void alarms_ack(alarms_t* self, const alarm_desc_t* alarm);

/** @brief Number of active alarms that were not acknowledged. */
int alarms_unacked_count(const alarms_t* self);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ALARMS_H */
