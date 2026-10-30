#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "app/app_state.h"
#include "models.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Callback typedef for async operations */
typedef void (*controller_callback_t)(int err, void* userdata);

void controller_init();
bool controller_check_conn();
void controller_get_sys_info();
void controller_get_pdu_info();

/* Async versions of critical functions */
void controller_get_sys_info_async(controller_callback_t callback, void* userdata);
void controller_get_pdu_info_async(controller_callback_t callback, void* userdata);
void controller_get_nw_if_async(controller_callback_t callback, void* userdata);

void controller_put_pdu_info(const models_pdu_info_t* pdu_info);
void controller_get_in_sw();
void controller_get_in_data(int line_id);
void controller_get_out_sw();
void controller_put_out_sw(const models_out_sw_t* out_sw, int line_id);
void controller_get_out_data(int line_id);
void controller_get_sensors();
bool controller_get_sensor_live(const char* mac);
bool controller_post_ble_scan_start();
void controller_post_ble_scan_stop();
void controller_get_ble_discovered();
bool controller_post_ble_confirm_mac(const char* mac);
bool controller_post_ble_confirm_all();
void controller_get_nw_services();
void controller_get_nw_info();

/** @brief GET /alarms. Returns 0 on success. */
int controller_get_alarms(app_state_alarms_t* alarms);
/** @brief POST /alarms/ack for one alarm id. Returns 0 on success. */
int controller_post_alarm_ack(const char* id);
/** @brief GET /settings/datetime. Returns 0 on success. */
int controller_get_datetime(app_state_datetime_t* datetime);
/** @brief PUT /settings/datetime: time zone and 12/24-hour clock. */
int controller_put_datetime(const char* timezone, int hour_format,
		app_state_datetime_t* datetime);
/** @brief PUT /settings/datetime/clock: sets the clock to local_time
 * ("YYYY-MM-DD HH:MM" in the saved time zone). Fails while NTP is on. */
int controller_put_clock(const char* local_time, app_state_datetime_t* datetime);
/** @brief GET /network/wifi-status. Returns 0 on success. */
int controller_get_wifi_status(app_state_wifi_t* wifi);
void controller_get_nw_if();
void controller_put_nw_if(const models_nw_if_t* nw_if);
void controller_post_nw_reset();
void controller_post_fact_reset();
void controller_post_reboot();
void controller_post_start_scan();
void controller_post_stop_scan();
void controller_put_license(const models_license_t* license);
void controller_get_license();
void controller_post_start_ssh();
void controller_post_stop_ssh();
void controller_post_start_snmp();
void controller_post_stop_snmp();
void controller_post_start_modbus();
void controller_post_stop_modbus();
int controller_put_modbus(const models_modbus_t* modbus);
void controller_get_modbus();
int controller_get_ntp();
int controller_put_ntp(const models_ntp_t* ntp);
int controller_get_snmp();
int controller_put_snmp(const models_snmp_t* snmp);
void controller_post_start_bluetooth();
void controller_post_stop_bluetooth();
void controller_get_bluetooth();
void controller_put_bluetooth(bool powered, bool pairable, bool discoverable);
void controller_post_bluetooth_scan(bool enable);
void controller_post_bluetooth_device_action(const char* mac, const char* action);
void controller_post_bluetooth_pairing_response(bool accept);
void controller_get_update_status();
void controller_post_update_confirm(bool confirm);
/* The update settings setters return 0 when the API saved them; the local
 * copy only changes then. */
int controller_put_update_settings(bool auto_update, const char* update_server,
		int check_interval_hours);
int controller_set_update_server(const char* server);
int controller_set_auto_update(bool enabled);
int controller_set_update_check_interval(int check_interval_hours);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CONTROLLER_H */
