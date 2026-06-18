#ifndef APP_STATE_H
#define APP_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define APP_STATE_MAX_OUTLETS 48

typedef struct {
	int line_id;
	bool status;
} app_state_outlet_t;

typedef struct {
	app_state_outlet_t outlets[APP_STATE_MAX_OUTLETS];
	int outlet_count;
	uint32_t outlet_revision;
} app_state_snapshot_t;

void app_state_init(void);
void app_state_cleanup(void);

void app_state_set_outlets(const app_state_outlet_t* outlets, int count);
void app_state_set_outlet(int index, bool status);
void app_state_get_snapshot(app_state_snapshot_t* snapshot);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* APP_STATE_H */
