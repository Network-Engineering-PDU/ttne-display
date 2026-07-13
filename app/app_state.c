#include "app/app_state.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

static app_state_snapshot_t state;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

void app_state_init(void)
{
	pthread_mutex_lock(&state_mutex);
	memset(&state, 0, sizeof(state));
	snprintf(state.license_type, sizeof(state.license_type), "%s", "N/A");
	snprintf(state.update_status.update_server,
			sizeof(state.update_status.update_server), "%s", "N/A");
	state.update_status.ota_enabled = false;
	state.update_status.check_interval_hours = 168;
	state.nw_if.type = 1;
	state.nw_if.dhcp = true;
	state.nw_if.nw_mode = -1;
	snprintf(state.nw_if.ip, sizeof(state.nw_if.ip), "%s", "192.168.1.100");
	snprintf(state.nw_if.mask, sizeof(state.nw_if.mask), "%s", "255.255.255.0");
	snprintf(state.nw_if.gw, sizeof(state.nw_if.gw), "%s", "192.168.1.1");
	snprintf(state.nw_if.dns, sizeof(state.nw_if.dns), "%s", "8.8.8.8");
	snprintf(state.nw_if.lan1_ip, sizeof(state.nw_if.lan1_ip), "%s", "192.168.1.100");
	snprintf(state.nw_if.lan2_ip, sizeof(state.nw_if.lan2_ip), "%s", "192.168.1.200");
	snprintf(state.system_info.product_name,
			sizeof(state.system_info.product_name), "%s", "N/A");
	snprintf(state.system_info.ip, sizeof(state.system_info.ip), "%s", "N/A");
	pthread_mutex_unlock(&state_mutex);
}

void app_state_cleanup(void)
{
}

void app_state_set_outlets(const app_state_outlet_t* outlets, int count)
{
	if (count < 0) {
		count = 0;
	}
	if (count > APP_STATE_MAX_OUTLETS) {
		count = APP_STATE_MAX_OUTLETS;
	}

	pthread_mutex_lock(&state_mutex);
	state.outlet_count = count;
	for (int i = 0; i < count; i++) {
		state.outlets[i] = outlets[i];
	}
	for (int i = count; i < APP_STATE_MAX_OUTLETS; i++) {
		state.outlets[i].line_id = i;
		state.outlets[i].status = false;
	}
	state.outlet_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_outlet(int index, bool status)
{
	pthread_mutex_lock(&state_mutex);
	if (index >= 0 && index < state.outlet_count &&
			index < APP_STATE_MAX_OUTLETS) {
		state.outlets[index].status = status;
		state.outlet_revision++;
	}
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_outlet_data(const app_state_outlet_data_t* outlet_data)
{
	if (outlet_data == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.outlet_data = *outlet_data;
	state.outlet_data.conn[sizeof(state.outlet_data.conn) - 1] = '\0';
	state.outlet_data.valid = true;
	state.outlet_data_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_power(const app_state_power_t* power)
{
	if (power == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.power = *power;
	if (state.power.input_count < 0) {
		state.power.input_count = 0;
	}
	if (state.power.input_count > APP_STATE_MAX_POWER_INPUTS) {
		state.power.input_count = APP_STATE_MAX_POWER_INPUTS;
	}
	state.power.valid = true;
	state.power_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_sensor_data(const app_state_sensor_data_t* sensor_data)
{
	if (sensor_data == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.sensor_data = *sensor_data;
	state.sensor_data.mac[sizeof(state.sensor_data.mac) - 1] = '\0';
	state.sensor_data.name[sizeof(state.sensor_data.name) - 1] = '\0';
	state.sensor_data.kind[sizeof(state.sensor_data.kind) - 1] = '\0';
	state.sensor_data.valid = true;
	state.sensor_data_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_update_status(const app_state_update_status_t* update_status)
{
	if (update_status == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.update_status = *update_status;
	state.update_status.update_server[
		sizeof(state.update_status.update_server) - 1] = '\0';
	state.update_status.valid = true;
	state.update_status_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_usb_update(const app_state_usb_update_t* usb_update)
{
	if (usb_update == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.usb_update = *usb_update;
	state.usb_update.device_name[
		sizeof(state.usb_update.device_name) - 1] = '\0';
	state.usb_update.update_dev[
		sizeof(state.usb_update.update_dev) - 1] = '\0';
	state.usb_update.valid = true;
	state.usb_update_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_bt_status(const app_state_bt_status_t* bt_status)
{
	if (bt_status == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.bt_status = *bt_status;
	state.bt_status.controller_mac[
		sizeof(state.bt_status.controller_mac) - 1] = '\0';
	state.bt_status.name[sizeof(state.bt_status.name) - 1] = '\0';
	state.bt_status.pairing_mac[
		sizeof(state.bt_status.pairing_mac) - 1] = '\0';
	state.bt_status.pairing_name[
		sizeof(state.bt_status.pairing_name) - 1] = '\0';
	state.bt_status.pairing_passkey[
		sizeof(state.bt_status.pairing_passkey) - 1] = '\0';
	if (state.bt_status.device_count < 0) {
		state.bt_status.device_count = 0;
	}
	if (state.bt_status.device_count > APP_STATE_MAX_BT_DEVICES) {
		state.bt_status.device_count = APP_STATE_MAX_BT_DEVICES;
	}
	for (int i = 0; i < state.bt_status.device_count; i++) {
		state.bt_status.devices[i].mac[
			sizeof(state.bt_status.devices[i].mac) - 1] = '\0';
		state.bt_status.devices[i].name[
			sizeof(state.bt_status.devices[i].name) - 1] = '\0';
	}
	state.bt_status.valid = true;
	state.bt_status_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_nw_if(const app_state_nw_if_t* nw_if)
{
	if (nw_if == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.nw_if = *nw_if;
	state.nw_if.eth_interface[sizeof(state.nw_if.eth_interface) - 1] = '\0';
	state.nw_if.ip[sizeof(state.nw_if.ip) - 1] = '\0';
	state.nw_if.mask[sizeof(state.nw_if.mask) - 1] = '\0';
	state.nw_if.gw[sizeof(state.nw_if.gw) - 1] = '\0';
	state.nw_if.dns[sizeof(state.nw_if.dns) - 1] = '\0';
	state.nw_if.ssid[sizeof(state.nw_if.ssid) - 1] = '\0';
	state.nw_if.pass[sizeof(state.nw_if.pass) - 1] = '\0';
	state.nw_if.lan1_ip[sizeof(state.nw_if.lan1_ip) - 1] = '\0';
	state.nw_if.lan2_ip[sizeof(state.nw_if.lan2_ip) - 1] = '\0';
	state.nw_if.wifi_ip[sizeof(state.nw_if.wifi_ip) - 1] = '\0';
	state.nw_if.valid = true;
	state.nw_if_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_nw_info(const app_state_nw_info_t* nw_info)
{
	if (nw_info == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.nw_info = *nw_info;
	state.nw_info.valid = true;
	state.nw_info_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_nw_services(const app_state_nw_services_t* nw_services)
{
	if (nw_services == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.nw_services = *nw_services;
	state.nw_services.valid = true;
	state.nw_services_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_modbus(const app_state_modbus_t* modbus)
{
	if (modbus == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.modbus = *modbus;
	state.modbus.valid = true;
	state.modbus_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_sensors(const app_state_sensor_t* sensors, int count)
{
	if (count < 0) {
		count = 0;
	}
	if (count > APP_STATE_MAX_SENSORS) {
		count = APP_STATE_MAX_SENSORS;
	}

	pthread_mutex_lock(&state_mutex);
	state.sensor_count = count;
	for (int i = 0; i < count; i++) {
		state.sensors[i] = sensors[i];
		state.sensors[i].mac[sizeof(state.sensors[i].mac) - 1] = '\0';
		state.sensors[i].name[sizeof(state.sensors[i].name) - 1] = '\0';
		state.sensors[i].valid = true;
	}
	for (int i = count; i < APP_STATE_MAX_SENSORS; i++) {
		memset(&state.sensors[i], 0, sizeof(state.sensors[i]));
	}
	state.sensors_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_discovered_sensors(
		const app_state_discovered_sensor_t* sensors, int count)
{
	if (count < 0) {
		count = 0;
	}
	if (count > APP_STATE_MAX_DISCOVERED_SENSORS) {
		count = APP_STATE_MAX_DISCOVERED_SENSORS;
	}

	pthread_mutex_lock(&state_mutex);
	state.discovered_sensor_count = count;
	for (int i = 0; i < count; i++) {
		state.discovered_sensors[i] = sensors[i];
		state.discovered_sensors[i].mac[
			sizeof(state.discovered_sensors[i].mac) - 1] = '\0';
		state.discovered_sensors[i].kind[
			sizeof(state.discovered_sensors[i].kind) - 1] = '\0';
		state.discovered_sensors[i].name[
			sizeof(state.discovered_sensors[i].name) - 1] = '\0';
		state.discovered_sensors[i].valid = true;
	}
	for (int i = count; i < APP_STATE_MAX_DISCOVERED_SENSORS; i++) {
		memset(&state.discovered_sensors[i], 0,
				sizeof(state.discovered_sensors[i]));
	}
	state.discovered_sensors_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_system_info(const app_state_system_info_t* info)
{
	if (info == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.system_info = *info;
	state.system_info.product_name[
		sizeof(state.system_info.product_name) - 1] = '\0';
	state.system_info.product_pn[
		sizeof(state.system_info.product_pn) - 1] = '\0';
	state.system_info.product_sn[
		sizeof(state.system_info.product_sn) - 1] = '\0';
	state.system_info.lan_mac[sizeof(state.system_info.lan_mac) - 1] = '\0';
	state.system_info.ip[sizeof(state.system_info.ip) - 1] = '\0';
	state.system_info.sw_version[
		sizeof(state.system_info.sw_version) - 1] = '\0';
	state.system_info.om_version[
		sizeof(state.system_info.om_version) - 1] = '\0';
	state.system_info.pmb_version[
		sizeof(state.system_info.pmb_version) - 1] = '\0';
	state.system_info.uptime[sizeof(state.system_info.uptime) - 1] = '\0';
	state.system_info.valid = true;
	state.system_info_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_pdu_info(const app_state_pdu_info_t* pdu_info)
{
	if (pdu_info == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.pdu_info = *pdu_info;
	state.pdu_info.controller[sizeof(state.pdu_info.controller) - 1] = '\0';
	state.pdu_info.type[sizeof(state.pdu_info.type) - 1] = '\0';
	state.pdu_info.valid = true;
	state.pdu_info_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_visual_config(const app_state_visual_config_t* visual_config)
{
	if (visual_config == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.visual_config = *visual_config;
	state.visual_config.pdu_company[
		sizeof(state.visual_config.pdu_company) - 1] = '\0';
	state.visual_config.pdu_rack[
		sizeof(state.visual_config.pdu_rack) - 1] = '\0';
	state.visual_config.pdu_system[
		sizeof(state.visual_config.pdu_system) - 1] = '\0';
	state.visual_config.pdu_ups[
		sizeof(state.visual_config.pdu_ups) - 1] = '\0';
	state.visual_config.pdu_elec_board[
		sizeof(state.visual_config.pdu_elec_board) - 1] = '\0';
	state.visual_config.pdu_breaker[
		sizeof(state.visual_config.pdu_breaker) - 1] = '\0';
	state.visual_config.pdu_service[
		sizeof(state.visual_config.pdu_service) - 1] = '\0';
	state.visual_config.valid = true;
	state.visual_config_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_login_config(const app_state_login_config_t* login_config)
{
	if (login_config == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	state.login_config = *login_config;
	state.login_config.valid = true;
	state.login_config_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_set_license_type(const char* license_type)
{
	if (license_type == NULL) {
		license_type = "N/A";
	}

	pthread_mutex_lock(&state_mutex);
	snprintf(state.license_type, sizeof(state.license_type), "%s", license_type);
	state.license_revision++;
	pthread_mutex_unlock(&state_mutex);
}

void app_state_get_snapshot(app_state_snapshot_t* snapshot)
{
	if (snapshot == NULL) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	*snapshot = state;
	pthread_mutex_unlock(&state_mutex);
}
