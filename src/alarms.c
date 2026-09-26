#include <stdio.h>
#include <string.h>

#include "alarms.h"

#define VOLTAGE_MIN_V 20.0f      /* below this an input is considered dead */
#define CURRENT_WARN_RATIO 0.9f  /* warning above 90 % of the rated current */

/* Function prototypes ********************************************************/

static int phases_from_sys_type(int sys_type);
static int branches_from_branch(int branch);
static void add(alarms_t* list, alarms_types_t type, const char* code,
		const char* path, const char* desc);
static void evaluate_power(alarms_t* list, const app_state_snapshot_t* snapshot);
static bool same_alarm(const alarm_desc_t* a, const alarm_desc_t* b);

/* Function definitions *******************************************************/

static int phases_from_sys_type(int sys_type)
{
	switch (sys_type) {
	case 0: return 1;
	case 1: return 2;
	case 2:
	case 3: return 3;
	default: return 0;
	}
}

static int branches_from_branch(int branch)
{
	switch (branch) {
	case 0: return 1;
	case 1: return 2;
	default: return 0;
	}
}

static void add(alarms_t* list, alarms_types_t type, const char* code,
		const char* path, const char* desc)
{
	alarm_desc_t alarm;

	memset(&alarm, 0, sizeof(alarm));
	alarm.type = type;
	snprintf(alarm.code, sizeof(alarm.code), "%s", code);
	snprintf(alarm.path, sizeof(alarm.path), "%s", path);
	snprintf(alarm.desc, sizeof(alarm.desc), "%s", desc);
	alarms_new(list, &alarm);
}

static void evaluate_power(alarms_t* list, const app_state_snapshot_t* snapshot)
{
	const app_state_power_t* power = &snapshot->power;
	int n_phases = phases_from_sys_type(power->sys_type);
	int n_branches = branches_from_branch(power->branch);
	float rated = snapshot->pdu_info.valid ?
			(float)snapshot->pdu_info.rated_current : 0.0f;

	if (!power->valid) {
		return;
	}

	for (int b = 0; b < n_branches; b++) {
		for (int p = 0; p < n_phases; p++) {
			int idx = b * n_phases + p;
			char path[48];

			if (idx >= power->input_count ||
					idx >= APP_STATE_MAX_POWER_INPUTS) {
				continue;
			}
			const app_state_power_input_t* in = &power->inputs[idx];
			snprintf(path, sizeof(path), "/PDU/%s/L%d",
					b == 0 ? "Main" : "Aux", p + 1);

			if (in->voltage < VOLTAGE_MIN_V) {
				add(list, ALARM_ERROR, "#203", path, "No voltage on input");
			}
			if (rated > 0.0f) {
				if (in->current > rated) {
					add(list, ALARM_ERROR, "#201", path,
							"Current above rated current");
				} else if (in->current > rated * CURRENT_WARN_RATIO) {
					add(list, ALARM_WARNING, "#202", path,
							"Current close to rated current");
				}
			}
		}
	}
}

static bool same_alarm(const alarm_desc_t* a, const alarm_desc_t* b)
{
	return strcmp(a->code, b->code) == 0 && strcmp(a->path, b->path) == 0;
}

/* Public functions ***********************************************************/

void alarms_init(alarms_t* self)
{
	memset(self, 0, sizeof(*self));
}

void alarms_new(alarms_t* self, const alarm_desc_t* alarm)
{
	if (self->n >= MAX_ALARMS) {
		return;
	}
	self->alarms[self->n] = *alarm;
	self->n++;
}

bool alarms_update(alarms_t* self, const app_state_snapshot_t* snapshot,
		bool comm_error, time_t now)
{
	static alarms_t next;
	bool changed;
	char stamp[24] = "";
	struct tm tm_now;

	alarms_init(&next);

	if (comm_error) {
		add(&next, ALARM_ERROR, "#101", "/PDU/Power",
				"Power data unavailable");
	}
	evaluate_power(&next, snapshot);
	if (snapshot->nw_info.valid && !snapshot->nw_info.connected) {
		add(&next, ALARM_WARNING, "#301", "/PDU/Network",
				"Network disconnected");
	}

	if (localtime_r(&now, &tm_now) != NULL) {
		strftime(stamp, sizeof(stamp), "%Y/%m/%d %H:%M:%S", &tm_now);
	}

	changed = next.n != self->n;
	for (int i = 0; i < next.n; i++) {
		bool found = false;

		for (int j = 0; j < self->n; j++) {
			if (same_alarm(&next.alarms[i], &self->alarms[j])) {
				snprintf(next.alarms[i].time, sizeof(next.alarms[i].time),
						"%s", self->alarms[j].time);
				next.alarms[i].ack = self->alarms[j].ack;
				if (next.alarms[i].type != self->alarms[j].type ||
						strcmp(next.alarms[i].desc, self->alarms[j].desc)) {
					changed = true;
				}
				found = true;
				break;
			}
		}
		if (!found) {
			snprintf(next.alarms[i].time, sizeof(next.alarms[i].time),
					"%s", stamp);
			changed = true;
		}
	}

	*self = next;
	return changed;
}

void alarms_ack(alarms_t* self, const alarm_desc_t* alarm)
{
	for (int i = 0; i < self->n; i++) {
		if (same_alarm(&self->alarms[i], alarm)) {
			self->alarms[i].ack = true;
		}
	}
}

int alarms_unacked_count(const alarms_t* self)
{
	int count = 0;

	for (int i = 0; i < self->n; i++) {
		if (!self->alarms[i].ack) {
			count++;
		}
	}
	return count;
}
