#ifndef APP_STATE_H
#define APP_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define APP_STATE_MAX_OUTLETS 48
#define APP_STATE_MAX_POWER_INPUTS 6
#define APP_STATE_SENSOR_TEXT_LEN 64
#define APP_STATE_MAX_BT_DEVICES 12
#define APP_STATE_BT_TEXT_LEN 64
#define APP_STATE_NW_TEXT_LEN 128
#define APP_STATE_MAX_SENSORS 8
#define APP_STATE_MAX_DISCOVERED_SENSORS 16

typedef struct {
	int line_id;
	bool status;
} app_state_outlet_t;

typedef struct {
	float voltage;
	float current;
	float active_power;
	float reactive_power;
	float apparent_power;
	float power_factor;
	float phase;
	float frequency;
	float energy;
	char conn[32];
	int fuse;
	int outlet_id;
	bool valid;
} app_state_outlet_data_t;

typedef struct {
	float voltage;
	float current;
	float active_power;
	float reactive_power;
	float apparent_power;
	float power_factor;
	float phase;
	float frequency;
	float energy;
} app_state_power_input_t;

typedef struct {
	int branch;
	int sys_type;
	int curr_type;
	app_state_power_input_t inputs[APP_STATE_MAX_POWER_INPUTS];
	int input_count;
	bool valid;
} app_state_power_t;

typedef struct {
	char mac[APP_STATE_SENSOR_TEXT_LEN];
	char name[APP_STATE_SENSOR_TEXT_LEN];
	char kind[APP_STATE_SENSOR_TEXT_LEN];
	float temp;
	float humd;
	float pres;
	int rssi;
	int bat_mv;
	int bat_pct;
	int sensor_index;
	bool valid;
} app_state_sensor_data_t;

typedef struct {
	bool is_pending;
	bool auto_update;
	char update_server[128];
	int check_interval_hours;
	bool valid;
} app_state_update_status_t;

typedef struct {
	bool device_found;
	bool running;
	bool complete;
	int result;
	char device_name[64];
	char update_dev[32];
	bool valid;
} app_state_usb_update_t;

typedef struct {
	char mac[APP_STATE_BT_TEXT_LEN];
	char name[APP_STATE_BT_TEXT_LEN];
	bool paired;
	bool trusted;
	bool connected;
	int rssi;
} app_state_bt_device_t;

typedef struct {
	char controller_mac[APP_STATE_BT_TEXT_LEN];
	char name[APP_STATE_BT_TEXT_LEN];
	bool powered;
	bool pairable;
	bool discoverable;
	bool discovering;
	bool pairing_request;
	char pairing_mac[APP_STATE_BT_TEXT_LEN];
	char pairing_name[APP_STATE_BT_TEXT_LEN];
	char pairing_passkey[APP_STATE_BT_TEXT_LEN];
	app_state_bt_device_t devices[APP_STATE_MAX_BT_DEVICES];
	int device_count;
	bool valid;
} app_state_bt_status_t;

typedef struct {
	int type;
	bool dhcp;
	char eth_interface[APP_STATE_NW_TEXT_LEN];
	char ip[APP_STATE_NW_TEXT_LEN];
	char mask[APP_STATE_NW_TEXT_LEN];
	char gw[APP_STATE_NW_TEXT_LEN];
	char dns[APP_STATE_NW_TEXT_LEN];
	char ssid[APP_STATE_NW_TEXT_LEN];
	char pass[APP_STATE_NW_TEXT_LEN];
	char lan1_ip[APP_STATE_NW_TEXT_LEN];
	char lan2_ip[APP_STATE_NW_TEXT_LEN];
	char wifi_ip[APP_STATE_NW_TEXT_LEN];
	int nw_mode;
	bool valid;
} app_state_nw_if_t;

typedef struct {
	bool connected;
	bool valid;
} app_state_nw_info_t;

typedef struct {
	bool snmp;
	bool modbus;
	bool ssh;
	bool bluetooth;
	bool valid;
} app_state_nw_services_t;

typedef struct {
	int addr;
	bool valid;
} app_state_modbus_t;

typedef struct {
	int id;
	char mac[APP_STATE_SENSOR_TEXT_LEN];
	char name[APP_STATE_SENSOR_TEXT_LEN];
	bool valid;
} app_state_sensor_t;

typedef struct {
	char mac[APP_STATE_SENSOR_TEXT_LEN];
	char kind[APP_STATE_SENSOR_TEXT_LEN];
	char name[APP_STATE_SENSOR_TEXT_LEN];
	int rssi;
	bool valid;
} app_state_discovered_sensor_t;

typedef struct {
	char product_name[APP_STATE_NW_TEXT_LEN];
	char product_pn[APP_STATE_NW_TEXT_LEN];
	char product_sn[APP_STATE_NW_TEXT_LEN];
	char lan_mac[APP_STATE_NW_TEXT_LEN];
	char ip[APP_STATE_NW_TEXT_LEN];
	char sw_version[APP_STATE_NW_TEXT_LEN];
	char om_version[APP_STATE_NW_TEXT_LEN];
	char pmb_version[APP_STATE_NW_TEXT_LEN];
	char uptime[APP_STATE_NW_TEXT_LEN];
	bool valid;
} app_state_system_info_t;

typedef struct {
	int n_outlets;
	int rated_current;
	char controller[APP_STATE_NW_TEXT_LEN];
	char type[APP_STATE_NW_TEXT_LEN];
	bool valid;
} app_state_pdu_info_t;

typedef struct {
	int rotation;
	int inactivity_time;
	char pdu_company[APP_STATE_NW_TEXT_LEN];
	char pdu_rack[APP_STATE_NW_TEXT_LEN];
	char pdu_system[APP_STATE_NW_TEXT_LEN];
	char pdu_ups[APP_STATE_NW_TEXT_LEN];
	char pdu_elec_board[APP_STATE_NW_TEXT_LEN];
	char pdu_breaker[APP_STATE_NW_TEXT_LEN];
	char pdu_service[APP_STATE_NW_TEXT_LEN];
	bool valid;
} app_state_visual_config_t;

typedef struct {
	app_state_outlet_t outlets[APP_STATE_MAX_OUTLETS];
	app_state_outlet_data_t outlet_data;
	app_state_power_t power;
	app_state_sensor_data_t sensor_data;
	app_state_update_status_t update_status;
	app_state_usb_update_t usb_update;
	app_state_bt_status_t bt_status;
	app_state_nw_if_t nw_if;
	app_state_nw_info_t nw_info;
	app_state_nw_services_t nw_services;
	app_state_modbus_t modbus;
	app_state_sensor_t sensors[APP_STATE_MAX_SENSORS];
	app_state_discovered_sensor_t discovered_sensors[
		APP_STATE_MAX_DISCOVERED_SENSORS];
	app_state_system_info_t system_info;
	app_state_pdu_info_t pdu_info;
	app_state_visual_config_t visual_config;
	char license_type[16];
	int outlet_count;
	int sensor_count;
	int discovered_sensor_count;
	uint32_t outlet_revision;
	uint32_t outlet_data_revision;
	uint32_t power_revision;
	uint32_t sensor_data_revision;
	uint32_t update_status_revision;
	uint32_t usb_update_revision;
	uint32_t bt_status_revision;
	uint32_t nw_if_revision;
	uint32_t nw_info_revision;
	uint32_t nw_services_revision;
	uint32_t modbus_revision;
	uint32_t sensors_revision;
	uint32_t discovered_sensors_revision;
	uint32_t system_info_revision;
	uint32_t pdu_info_revision;
	uint32_t visual_config_revision;
	uint32_t license_revision;
} app_state_snapshot_t;

void app_state_init(void);
void app_state_cleanup(void);

void app_state_set_outlets(const app_state_outlet_t* outlets, int count);
void app_state_set_outlet(int index, bool status);
void app_state_set_outlet_data(const app_state_outlet_data_t* outlet_data);
void app_state_set_power(const app_state_power_t* power);
void app_state_set_sensor_data(const app_state_sensor_data_t* sensor_data);
void app_state_set_update_status(const app_state_update_status_t* update_status);
void app_state_set_usb_update(const app_state_usb_update_t* usb_update);
void app_state_set_bt_status(const app_state_bt_status_t* bt_status);
void app_state_set_nw_if(const app_state_nw_if_t* nw_if);
void app_state_set_nw_info(const app_state_nw_info_t* nw_info);
void app_state_set_nw_services(const app_state_nw_services_t* nw_services);
void app_state_set_modbus(const app_state_modbus_t* modbus);
void app_state_set_sensors(const app_state_sensor_t* sensors, int count);
void app_state_set_discovered_sensors(
		const app_state_discovered_sensor_t* sensors, int count);
void app_state_set_system_info(const app_state_system_info_t* info);
void app_state_set_pdu_info(const app_state_pdu_info_t* pdu_info);
void app_state_set_visual_config(const app_state_visual_config_t* visual_config);
void app_state_set_license_type(const char* license_type);
void app_state_get_snapshot(app_state_snapshot_t* snapshot);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* APP_STATE_H */
