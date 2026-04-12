#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "models.h"

#ifdef __cplusplus
extern "C" {
#endif

void controller_init();
bool controller_check_conn();
void controller_get_sys_info();
void controller_get_pdu_info();
void controller_get_in_sw();
void controller_get_in_data(int line_id);
void controller_get_out_sw();
void controller_put_out_sw(const models_out_sw_t* out_sw, int line_id);
void controller_get_out_data(int line_id);
void controller_get_sensors();
void controller_get_nw_services();
void controller_get_nw_info();
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
void controller_put_modbus(const models_modbus_t* modbus);
void controller_get_modbus();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CONTROLLER_H */