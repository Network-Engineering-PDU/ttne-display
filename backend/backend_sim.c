#ifdef SIMULATOR_ENABLED

#include "backend/backend.h"

#include <stdio.h>
#include <string.h>

#include "app/app_state.h"

#define SIM_OUTLETS 8

static app_state_outlet_t sim_outlets[SIM_OUTLETS];
static bool sim_initialized;
static app_state_update_status_t sim_update_status = {
	.is_pending = false,
	.auto_update = false,
	.update_server = "https://github.com/Network-Engineering-PDU/firmware-update",
	.check_interval_hours = 24,
	.valid = true,
};

static void ensure_sim_outlets(void)
{
	if (sim_initialized) {
		return;
	}

	for (int i = 0; i < SIM_OUTLETS; i++) {
		sim_outlets[i].line_id = i;
		sim_outlets[i].status = (i % 2) == 0;
	}
	app_state_set_outlets(sim_outlets, SIM_OUTLETS);
	sim_initialized = true;
}

int backend_init(void)
{
	ensure_sim_outlets();
	app_state_set_license_type("B2");
	return 0;
}

void backend_cleanup(void)
{
}

void backend_process(void)
{
}

int backend_outlets_refresh(backend_callback_t callback, void* userdata)
{
	ensure_sim_outlets();
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_outlet_set(int line_id, bool status, backend_callback_t callback,
		void* userdata)
{
	ensure_sim_outlets();
	if (line_id >= 0 && line_id < SIM_OUTLETS) {
		sim_outlets[line_id].status = status;
		app_state_set_outlet(line_id, status);
	}
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_outlets_set_all(bool status, backend_callback_t callback,
		void* userdata)
{
	ensure_sim_outlets();
	for (int i = 0; i < SIM_OUTLETS; i++) {
		sim_outlets[i].line_id = i;
		sim_outlets[i].status = status;
	}
	app_state_set_outlets(sim_outlets, SIM_OUTLETS);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_outlet_data_refresh(int outlet_id, backend_callback_t callback,
		void* userdata)
{
	app_state_outlet_data_t outlet_data = {
		.voltage = 230.0f + (float)(outlet_id % 3),
		.current = 1.2f + (float)(outlet_id % 4) * 0.3f,
		.active_power = 275.0f + (float)outlet_id * 5.0f,
		.reactive_power = 12.0f,
		.apparent_power = 280.0f + (float)outlet_id * 5.0f,
		.power_factor = 0.98f,
		.phase = 0.0f,
		.frequency = 50.0f,
		.energy = 1200.0f + (float)outlet_id * 20.0f,
		.fuse = 1,
		.outlet_id = outlet_id,
		.valid = true,
	};

	snprintf(outlet_data.conn, sizeof(outlet_data.conn), "%s", "IEC C13");
	app_state_set_outlet_data(&outlet_data);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_license_refresh(backend_callback_t callback, void* userdata)
{
	app_state_set_license_type("B2");
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_power_refresh(backend_callback_t callback, void* userdata)
{
	app_state_power_t power;

	memset(&power, 0, sizeof(power));
	power.branch = 1;
	power.sys_type = 2;
	power.curr_type = 0;
	power.input_count = APP_STATE_MAX_POWER_INPUTS;
	power.valid = true;

	for (int i = 0; i < APP_STATE_MAX_POWER_INPUTS; i++) {
		power.inputs[i].voltage = 230.0f + (float)(i % 3);
		power.inputs[i].current = 2.0f + (float)i * 0.2f;
		power.inputs[i].active_power = power.inputs[i].voltage *
				power.inputs[i].current * 0.95f;
		power.inputs[i].reactive_power = 8.0f + (float)i;
		power.inputs[i].apparent_power = power.inputs[i].voltage *
				power.inputs[i].current;
		power.inputs[i].power_factor = 0.95f;
		power.inputs[i].phase = 0.0f;
		power.inputs[i].frequency = 50.0f;
		power.inputs[i].energy = 5000.0f + (float)i * 100.0f;
	}

	app_state_set_power(&power);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_sensor_data_refresh(int sensor_index, backend_callback_t callback,
		void* userdata)
{
	app_state_sensor_data_t sensor_data;

	memset(&sensor_data, 0, sizeof(sensor_data));
	snprintf(sensor_data.mac, sizeof(sensor_data.mac),
			"AA:BB:CC:DD:EE:%02X", sensor_index >= 0 ? sensor_index : 0);
	snprintf(sensor_data.name, sizeof(sensor_data.name), "Sensor %d",
			sensor_index + 1);
	snprintf(sensor_data.kind, sizeof(sensor_data.kind), "%s", "THP");
	sensor_data.temp = 24.5f + (float)sensor_index;
	sensor_data.humd = 45.0f + (float)(sensor_index % 10);
	sensor_data.pres = 1012.0f + (float)(sensor_index % 5);
	sensor_data.rssi = -48 - sensor_index;
	sensor_data.bat_mv = 2980 - (sensor_index * 12);
	sensor_data.bat_pct = 86 - sensor_index;
	sensor_data.sensor_index = sensor_index;
	sensor_data.valid = true;

	app_state_set_sensor_data(&sensor_data);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_update_status_refresh(backend_callback_t callback, void* userdata)
{
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_update_confirm(bool confirm, backend_callback_t callback,
		void* userdata)
{
	if (confirm) {
		sim_update_status.is_pending = false;
	}
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_update_set_auto(bool enabled, backend_callback_t callback,
		void* userdata)
{
	sim_update_status.auto_update = enabled;
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_update_set_interval(int hours, backend_callback_t callback,
		void* userdata)
{
	sim_update_status.check_interval_hours = hours;
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_update_set_server(const char* server, backend_callback_t callback,
		void* userdata)
{
	snprintf(sim_update_status.update_server,
			sizeof(sim_update_status.update_server), "%s",
			server != NULL ? server : "");
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

#endif /* SIMULATOR_ENABLED */
