#ifndef SIMULATOR_ENABLED

#include "backend/backend.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "controller.h"
#include "models.h"
#include "app/app_state.h"

#define BACKEND_QUEUE_SIZE 32

typedef enum {
	BACKEND_CMD_NONE = 0,
	BACKEND_CMD_OUTLETS_REFRESH,
	BACKEND_CMD_OUTLET_SET,
	BACKEND_CMD_OUTLETS_SET_ALL,
	BACKEND_CMD_OUTLET_DATA_REFRESH,
	BACKEND_CMD_LICENSE_REFRESH,
	BACKEND_CMD_POWER_REFRESH,
} backend_cmd_type_t;

typedef struct {
	backend_cmd_type_t type;
	int line_id;
	bool status;
	backend_callback_t callback;
	void* userdata;
} backend_cmd_t;

typedef struct {
	int err;
	backend_callback_t callback;
	void* userdata;
} backend_result_t;

static backend_cmd_t queue[BACKEND_QUEUE_SIZE];
static backend_result_t results[BACKEND_QUEUE_SIZE];
static int queue_head;
static int queue_tail;
static int queue_count;
static int result_count;
static bool shutdown_requested;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_t worker;
static bool worker_started;

static void publish_outlets_from_models(void)
{
	int len;
	const models_out_sw_t* model_outlets = models_get_out_sw(&len);
	app_state_outlet_t outlets[APP_STATE_MAX_OUTLETS];

	if (model_outlets == NULL || len <= 0) {
		app_state_set_outlets(NULL, 0);
		return;
	}

	if (len > APP_STATE_MAX_OUTLETS) {
		len = APP_STATE_MAX_OUTLETS;
	}

	for (int i = 0; i < len; i++) {
		outlets[i].line_id = model_outlets[i].line_id;
		outlets[i].status = model_outlets[i].status;
	}
	app_state_set_outlets(outlets, len);
}

static void publish_outlet_data_from_models(int outlet_id)
{
	const models_out_data_t* model_data = models_get_out_data();
	app_state_outlet_data_t outlet_data = {
		.voltage = model_data->voltage,
		.current = model_data->current,
		.active_power = model_data->active_power,
		.reactive_power = model_data->reactive_power,
		.apparent_power = model_data->apparent_power,
		.power_factor = model_data->power_factor,
		.phase = model_data->phase,
		.frequency = model_data->frequency,
		.energy = model_data->energy,
		.fuse = model_data->fuse,
		.outlet_id = outlet_id,
		.valid = true,
	};

	snprintf(outlet_data.conn, sizeof(outlet_data.conn), "%s",
			model_data->conn != NULL ? model_data->conn : "N/A");
	app_state_set_outlet_data(&outlet_data);
}

static void publish_license_from_models(void)
{
	const models_license_t* license = models_get_license();
	app_state_set_license_type(license != NULL ? license->type_id : "N/A");
}

static void run_power_refresh(void)
{
	app_state_power_t power;
	memset(&power, 0, sizeof(power));

	controller_get_in_sw();
	const models_in_sw_t* in_sw = models_get_in_sw();
	power.branch = in_sw->branch;
	power.sys_type = in_sw->sys_type;
	power.curr_type = in_sw->curr_type;
	power.input_count = APP_STATE_MAX_POWER_INPUTS;

	for (int i = 0; i < APP_STATE_MAX_POWER_INPUTS; i++) {
		controller_get_in_data(i);
		const models_in_data_t* in_data = models_get_in_data();
		power.inputs[i].voltage = in_data->voltage;
		power.inputs[i].current = in_data->current;
		power.inputs[i].active_power = in_data->active_power;
		power.inputs[i].reactive_power = in_data->reactive_power;
		power.inputs[i].apparent_power = in_data->apparent_power;
		power.inputs[i].power_factor = in_data->power_factor;
		power.inputs[i].phase = in_data->phase;
		power.inputs[i].frequency = in_data->frequency;
		power.inputs[i].energy = in_data->energy;
	}

	app_state_set_power(&power);
}

static void backend_push_result(int err, backend_callback_t callback, void* userdata)
{
	if (callback == NULL) {
		return;
	}

	pthread_mutex_lock(&mutex);
	if (result_count < BACKEND_QUEUE_SIZE) {
		results[result_count].err = err;
		results[result_count].callback = callback;
		results[result_count].userdata = userdata;
		result_count++;
	}
	pthread_mutex_unlock(&mutex);
}

static int backend_submit(backend_cmd_t* cmd)
{
	pthread_mutex_lock(&mutex);
	if (queue_count >= BACKEND_QUEUE_SIZE || shutdown_requested) {
		pthread_mutex_unlock(&mutex);
		return -1;
	}

	queue[queue_tail] = *cmd;
	queue_tail = (queue_tail + 1) % BACKEND_QUEUE_SIZE;
	queue_count++;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
	return 0;
}

static bool backend_pop_command(backend_cmd_t* cmd)
{
	pthread_mutex_lock(&mutex);
	while (queue_count == 0 && !shutdown_requested) {
		pthread_cond_wait(&cond, &mutex);
	}

	if (queue_count == 0 && shutdown_requested) {
		pthread_mutex_unlock(&mutex);
		return false;
	}

	*cmd = queue[queue_head];
	memset(&queue[queue_head], 0, sizeof(queue[queue_head]));
	queue_head = (queue_head + 1) % BACKEND_QUEUE_SIZE;
	queue_count--;
	pthread_mutex_unlock(&mutex);
	return true;
}

static void run_outlets_set_all(bool status)
{
	int len;
	models_get_out_sw(&len);
	if (len <= 0) {
		controller_get_out_sw();
		models_get_out_sw(&len);
	}

	for (int i = 0; i < len; i++) {
		models_out_sw_t out_sw = {
			.line_id = i,
			.status = status,
		};
		controller_put_out_sw(&out_sw, i);
	}
	publish_outlets_from_models();
}

static void* backend_worker(void* arg)
{
	(void)arg;

	while (true) {
		backend_cmd_t cmd;
		int err = 0;

		if (!backend_pop_command(&cmd)) {
			break;
		}

		switch (cmd.type) {
		case BACKEND_CMD_OUTLETS_REFRESH:
			controller_get_out_sw();
			publish_outlets_from_models();
			break;
		case BACKEND_CMD_OUTLET_SET: {
			models_out_sw_t out_sw = {
				.line_id = cmd.line_id,
				.status = cmd.status,
			};
			controller_put_out_sw(&out_sw, cmd.line_id);
			app_state_set_outlet(cmd.line_id, cmd.status);
			break;
		}
		case BACKEND_CMD_OUTLETS_SET_ALL:
			run_outlets_set_all(cmd.status);
			break;
		case BACKEND_CMD_OUTLET_DATA_REFRESH:
			controller_get_out_data(cmd.line_id);
			publish_outlet_data_from_models(cmd.line_id);
			break;
		case BACKEND_CMD_LICENSE_REFRESH:
			controller_get_license();
			publish_license_from_models();
			break;
		case BACKEND_CMD_POWER_REFRESH:
			run_power_refresh();
			break;
		default:
			err = 1;
			break;
		}

		backend_push_result(err, cmd.callback, cmd.userdata);
	}

	return NULL;
}

int backend_init(void)
{
	pthread_mutex_lock(&mutex);
	queue_head = 0;
	queue_tail = 0;
	queue_count = 0;
	result_count = 0;
	shutdown_requested = false;
	pthread_mutex_unlock(&mutex);

	if (pthread_create(&worker, NULL, backend_worker, NULL) != 0) {
		return -1;
	}

	worker_started = true;
	return 0;
}

void backend_cleanup(void)
{
	pthread_mutex_lock(&mutex);
	shutdown_requested = true;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);

	if (worker_started) {
		pthread_join(worker, NULL);
		worker_started = false;
	}
}

void backend_process(void)
{
	backend_result_t pending[BACKEND_QUEUE_SIZE];
	int pending_count;

	pthread_mutex_lock(&mutex);
	pending_count = result_count;
	memcpy(pending, results, sizeof(backend_result_t) * pending_count);
	memset(results, 0, sizeof(results));
	result_count = 0;
	pthread_mutex_unlock(&mutex);

	for (int i = 0; i < pending_count; i++) {
		if (pending[i].callback != NULL) {
			pending[i].callback(pending[i].err, pending[i].userdata);
		}
	}
}

int backend_outlets_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_OUTLETS_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_outlet_set(int line_id, bool status, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_OUTLET_SET,
		.line_id = line_id,
		.status = status,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_outlets_set_all(bool status, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_OUTLETS_SET_ALL,
		.status = status,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_outlet_data_refresh(int outlet_id, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_OUTLET_DATA_REFRESH,
		.line_id = outlet_id,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_license_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_LICENSE_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_power_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_POWER_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

#endif /* SIMULATOR_ENABLED */
