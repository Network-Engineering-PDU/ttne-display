#ifdef SIMULATOR_ENABLED

#include "backend/backend.h"

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

#endif /* SIMULATOR_ENABLED */
