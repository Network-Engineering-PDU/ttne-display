#ifdef SIMULATOR_ENABLED

#include "backend/backend.h"

#include <stdio.h>

#include "app/app_state.h"

#define SIM_OUTLETS 8

static app_state_outlet_t sim_outlets[SIM_OUTLETS];
static bool sim_initialized;

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

#endif /* SIMULATOR_ENABLED */
