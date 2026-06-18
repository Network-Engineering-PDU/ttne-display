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
	state.update_status.check_interval_hours = 24;
	state.nw_if.type = 1;
	state.nw_if.dhcp = true;
	state.nw_if.nw_mode = -1;
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
