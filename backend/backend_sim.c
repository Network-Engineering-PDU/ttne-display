#ifdef SIMULATOR_ENABLED

#include "backend/backend.h"

#include "models.h"

#define SIM_OUTLETS 8

static models_out_sw_t sim_outlets[SIM_OUTLETS];

static void ensure_sim_outlets(void)
{
	int len;
	models_get_out_sw(&len);
	if (len > 0) {
		return;
	}

	for (int i = 0; i < SIM_OUTLETS; i++) {
		sim_outlets[i].line_id = i;
		sim_outlets[i].status = (i % 2) == 0;
	}
	models_set_out_sw(sim_outlets, SIM_OUTLETS);
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
		models_out_sw_t out_sw = {
			.line_id = line_id,
			.status = status,
		};
		models_set_out_sw_idx(&out_sw, line_id);
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
	models_set_out_sw(sim_outlets, SIM_OUTLETS);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

#endif /* SIMULATOR_ENABLED */
