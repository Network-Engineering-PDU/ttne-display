#ifndef MODELS_H
#define MODELS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define MAX_ALARMS 100

// INFO

typedef struct models_info_t {
	const char* product_name;
	const char* product_pn;
	const char* product_sn;
	const char* lan_mac;
	const char* ip;
	const char* sw_version;
	const char* om_version;
	const char* pmb_version;
	const char* uptime;
} models_info_t;

typedef struct models_pdu_info_t {
	int n_outlets;
	int rated_current;
	const char* controller;
	const char* type;
} models_pdu_info_t;

typedef enum {
	BRANCH_MAIN = 0,
	BRANCH_ALL  = 1,
} models_in_sw_branch_t;

typedef enum {
	TYPE_SINGLE = 0,
	TYPE_BI     = 1,
	TYPE_TRI    = 2,
	TYPE_TRI_N  = 3,
} models_in_sw_type_t;

typedef enum {
	CURR_MLX = 0,
	CURR_TRA = 1,
} models_in_sw_curr_t;

typedef struct models_in_sw_t {
	int branch;
	int sys_type;
	int curr_type;
} models_in_sw_t;

typedef struct models_info_snmp_t {
	const char* name;
	const char* contact;
	const char* location;
} models_info_snmp_t;

// ALARMS

// INPUTS

typedef struct models_in_data_t {
	float voltage;
	float current;
	float active_power;
	float reactive_power;
	float apparent_power;
	float power_factor;
	float phase;
	float frequency;
	float energy;
} models_in_data_t;

// OUTLETS

typedef struct models_out_sw_t {
	int line_id;
	bool status;
} models_out_sw_t;

typedef struct models_out_data_t {
	float voltage;
	float current;
	float active_power;
	float reactive_power;
	float apparent_power;
	float power_factor;
	float phase;
	float frequency;
	float energy;
	const char* conn;
	int fuse;
} models_out_data_t;


// SENSORS

typedef struct sensor_last_data_t {
	const char* datetime;
	float temp;
	float humd;
	float pres;
	int rssi;
	int bat;
} sensor_last_data_t;

typedef struct models_sensor_t {
	int id;
	const char* mac;
	const char* name;
	sensor_last_data_t last_data;
} models_sensor_t;


// SETTINGS

typedef enum {
    UNCONF      = 1,
    ETH_DHCP    = 2,
    ETH_STATIC  = 3,
    WIFI_DHCP   = 4,
    WIFI_STATIC = 5,
} nw_type_t;

typedef struct nw_if_params_t {
	const char* ip;
	const char* mask;
	const char* gw;
	const char* dns;
	const char* ssid;
	const char* pass;
} nw_if_params_t;

typedef struct models_nw_info_t {
	bool connected;
} models_nw_info_t;

typedef struct models_nw_if_t {
	nw_type_t type;
	bool dhcp;
	nw_if_params_t params;
} models_nw_if_t;

typedef struct models_nw_services_t {
	bool snmp;
	bool modbus;
	bool ssh;
} models_nw_services_t;

typedef struct models_license_t {
	const char* type_id;
} models_license_t;

typedef struct models_modbus_t {
	int addr;
} models_modbus_t;

const models_info_t* models_get_info();
void models_set_info(const models_info_t* info);

const models_pdu_info_t* models_get_pdu_info();
void models_set_pdu_info(const models_pdu_info_t* pdu_info);

const models_in_sw_t* models_get_in_sw();
void models_set_in_sw(const models_in_sw_t* l_in_sw);

const models_in_data_t* models_get_in_data();
void models_set_in_data(const models_in_data_t* in_data);

const models_out_sw_t* models_get_out_sw(int* len);
const models_out_sw_t* models_get_out_sw_id(int id);
void models_set_out_sw(const models_out_sw_t* out_sw, int len);
void models_set_out_sw_idx(const models_out_sw_t* l_out_sw, int line_id);

const models_out_data_t* models_get_out_data();
void models_set_out_data(const models_out_data_t* out_data);

const models_sensor_t* models_get_sensor(int* len);
const models_sensor_t* models_get_sensor_id(int id);
void models_set_sensor(const models_sensor_t* l_sensor, int len);

const models_nw_services_t* models_get_nw_services();
void models_set_nw_services(const models_nw_services_t* nw_services);

const models_nw_info_t* models_get_nw_info();
void models_set_nw_info(const models_nw_info_t* nw_info);

const models_nw_if_t* models_get_nw_if();
void models_set_nw_if(const models_nw_if_t* nw_if);

const models_license_t* models_get_license();
void models_set_license(const models_license_t* license);

const models_modbus_t* models_get_modbus();
void models_set_modbus(const models_modbus_t* modbus);

// const models_settings_t* models_get_settings();
// void models_set_settings(models_settings_t* settings);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MODELS_H */
