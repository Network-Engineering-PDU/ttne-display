#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app/app_state.h"


int json_helper_update_sys_info(const char* json_str);
int json_helper_update_pdu_info(const char* json_str);
int json_helper_update_in_sw(const char* json_str);
int json_helper_update_in_data(const char* json_str);
int json_helper_update_sensors(const char* json_str);
int json_helper_update_discovered(const char* json_str);
int json_helper_update_sensor_live(const char* json_str);
int json_helper_update_out_sw(const char* json_str);
int json_helper_update_out_data(const char* json_str);
int json_helper_update_nw_services(const char* json_str);
int json_helper_update_nw_info(const char* json_str);

/**
 * @brief Parses the GET /alarms response.
 *
 * @param[in]  json_str     JSON text.
 * @param[out] alarms       Parsed alarms (zeroed first).
 * @return 0 on success.
 */
int json_helper_parse_alarms(const char* json_str, app_state_alarms_t* alarms);
int json_helper_update_nw_if(const char* json_str);
int json_helper_update_bt_status(const char* json_str);
int json_helper_update_license(const char* json_str);
int json_helper_update_modbus(const char* json_str);
int json_helper_update_update_status(const char* json_str);
int json_helper_update_ntp(const char* json_str);
int json_helper_update_snmp(const char* json_str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* JSON_HELPER_H */
