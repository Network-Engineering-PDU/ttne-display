#ifndef BACKEND_H
#define BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef void (*backend_callback_t)(int err, void* userdata);

int backend_init(void);
void backend_cleanup(void);
void backend_process(void);

int backend_outlets_refresh(backend_callback_t callback, void* userdata);
int backend_outlet_set(int line_id, bool status, backend_callback_t callback,
		void* userdata);
int backend_outlets_set_all(bool status, backend_callback_t callback,
		void* userdata);
int backend_outlet_data_refresh(int outlet_id, backend_callback_t callback,
		void* userdata);
int backend_license_refresh(backend_callback_t callback, void* userdata);
int backend_power_refresh(backend_callback_t callback, void* userdata);
int backend_sensor_data_refresh(int sensor_index, backend_callback_t callback,
		void* userdata);
int backend_update_status_refresh(backend_callback_t callback, void* userdata);
int backend_update_confirm(bool confirm, backend_callback_t callback,
		void* userdata);
int backend_update_set_auto(bool enabled, backend_callback_t callback,
		void* userdata);
int backend_update_set_interval(int hours, backend_callback_t callback,
		void* userdata);
int backend_update_set_server(const char* server, backend_callback_t callback,
		void* userdata);
int backend_system_reboot(backend_callback_t callback, void* userdata);
int backend_system_factory_reset(backend_callback_t callback, void* userdata);
int backend_usb_update_detect(backend_callback_t callback, void* userdata);
int backend_usb_update_start(const char* update_dev, backend_callback_t callback,
		void* userdata);
int backend_usb_update_poll(backend_callback_t callback, void* userdata);
int backend_bluetooth_refresh(backend_callback_t callback, void* userdata);
int backend_bluetooth_set(bool powered, bool pairable, bool discoverable,
		backend_callback_t callback, void* userdata);
int backend_bluetooth_scan(bool enable, backend_callback_t callback,
		void* userdata);
int backend_bluetooth_device_action(const char* mac, const char* action,
		backend_callback_t callback, void* userdata);
int backend_bluetooth_pairing_response(bool accept, backend_callback_t callback,
		void* userdata);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* BACKEND_H */
