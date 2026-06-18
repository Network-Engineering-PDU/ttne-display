#include "app/app_state.h"

#include <pthread.h>
#include <string.h>

static app_state_snapshot_t state;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

void app_state_init(void)
{
	pthread_mutex_lock(&state_mutex);
	memset(&state, 0, sizeof(state));
	pthread_mutex_unlock(&state_mutex);
}

void app_state_cleanup(void)
{
}

void app_state_set_outlets(const app_state_outlet_t* outlets, int count)
{
	if (count < 0) {
		count = 0;
	}
	if (count > APP_STATE_MAX_OUTLETS) {
		count = APP_STATE_MAX_OUTLETS;
	}

	pthread_mutex_lock(&state_mutex);
	state.outlet_count = count;
	for (int i = 0; i < count; i++) {
		state.outlets[i] = outlets[i];
	}
	for (int i = count; i < APP_STATE_MAX_OUTLETS; i++) {
		state.outlets[i].line_id = i;
		state.outlets[i].status = false;
	}
	state.outlet_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_outlet(int index, bool status)
{
	pthread_mutex_lock(&state_mutex);
	if (index >= 0 && index < state.outlet_count &&
			index < APP_STATE_MAX_OUTLETS) {
		state.outlets[index].status = status;
		state.outlet_revision++;
	}
	pthread_mutex_unlock(&state_mutex);
}

void app_state_get_snapshot(app_state_snapshot_t* snapshot)
{
	if (snapshot == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	*snapshot = state;
	pthread_mutex_unlock(&state_mutex);
}
