#ifndef ALARMS_H
#define ALARMS_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ALARMS 100

typedef enum {
	ALARM_INFO = 0,
	ALARM_WARNING = 1,
	ALARM_ERROR = 2,
	ALARM_CRITICAL_ERROR = 3,
} alarms_types_t;

typedef struct alarm_desc_t {
	alarms_types_t type;
	const char* time;
	const char* desc;
	const char* path;
	const char* code;
} alarm_desc_t;

typedef struct alarms_t {
	alarm_desc_t alarms[MAX_ALARMS];
	int n;
} alarms_t;

void alarms_new(alarms_t* self, alarm_desc_t* alarm);
void alarms_del(alarms_t* self, alarm_desc_t* alarm);
void alarms_get(alarms_t* self);
void alarms_init(alarms_t* self);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ALARMS_H */
