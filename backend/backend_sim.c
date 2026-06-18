#ifdef SIMULATOR_ENABLED

#include "backend/backend.h"

#include <stdio.h>
#include <string.h>

#include "app/app_state.h"

#define SIM_OUTLETS 8

static app_state_outlet_t sim_outlets[SIM_OUTLETS];
static bool sim_initialized;
static bool sim_usb_running;
static app_state_nw_services_t sim_nw_services = {
	.snmp = false,
	.modbus = false,
	.ssh = false,
	.bluetooth = true,
	.valid = true,
};
static app_state_modbus_t sim_modbus = {
	.addr = 1,
	.valid = true,
};
static app_state_sensor_t sim_sensors[APP_STATE_MAX_SENSORS];
static int sim_sensor_count;
static app_state_discovered_sensor_t sim_discovered[2] = {
	{
		.mac = "AA:BB:CC:DD:EE:10",
		.kind = "THP",
		.name = "Rack Sensor",
		.rssi = -44,
		.valid = true,
	},
	{
		.mac = "AA:BB:CC:DD:EE:11",
		.kind = "TH",
		.name = "Door Sensor",
		.rssi = -51,
		.valid = true,
	},
};
static app_state_bt_status_t sim_bt_status;
static app_state_nw_if_t sim_nw_if = {
	.type = 2,
	.dhcp = true,
	.eth_interface = "eth0",
	.ip = "192.168.1.120",
	.mask = "255.255.255.0",
	.gw = "192.168.1.1",
	.dns = "8.8.8.8",
	.ssid = "PDU-Lab",
	.pass = "",
	.lan1_ip = "192.168.1.120",
	.lan2_ip = "",
	.wifi_ip = "",
	.nw_mode = 0,
	.valid = true,
};
static app_state_update_status_t sim_update_status = {
	.is_pending = false,
	.auto_update = false,
	.update_server = "https://github.com/Network-Engineering-PDU/firmware-update",
	.check_interval_hours = 24,
	.valid = true,
};

static void ensure_sim_outlets(void)
{
	if (sim_initialized) {
		return;
	}

	for (int i = 0; i < SIM_OUTLETS; i++) {
		sim_outlets[i].line_id = i;
		sim_outlets[i].status = (i % 2) == 0;
	}
	app_state_set_outlets(sim_outlets, SIM_OUTLETS);

	memset(&sim_bt_status, 0, sizeof(sim_bt_status));
	snprintf(sim_bt_status.controller_mac, sizeof(sim_bt_status.controller_mac),
			"%s", "00:11:22:33:44:55");
	snprintf(sim_bt_status.name, sizeof(sim_bt_status.name), "%s", "PDU-SIM");
	sim_bt_status.powered = true;
	sim_bt_status.pairable = false;
	sim_bt_status.discoverable = false;
	sim_bt_status.discovering = false;
	sim_bt_status.device_count = 2;
	snprintf(sim_bt_status.devices[0].mac, sizeof(sim_bt_status.devices[0].mac),
			"%s", "10:20:30:40:50:60");
	snprintf(sim_bt_status.devices[0].name, sizeof(sim_bt_status.devices[0].name),
			"%s", "Service Tablet");
	sim_bt_status.devices[0].paired = true;
	sim_bt_status.devices[0].trusted = true;
	sim_bt_status.devices[0].connected = false;
	sim_bt_status.devices[0].rssi = -42;
	snprintf(sim_bt_status.devices[1].mac, sizeof(sim_bt_status.devices[1].mac),
			"%s", "AA:BB:CC:DD:EE:01");
	snprintf(sim_bt_status.devices[1].name, sizeof(sim_bt_status.devices[1].name),
			"%s", "BLE Sensor Bridge");
	sim_bt_status.devices[1].paired = false;
	sim_bt_status.devices[1].trusted = false;
	sim_bt_status.devices[1].connected = false;
	sim_bt_status.devices[1].rssi = -58;
	app_state_set_bt_status(&sim_bt_status);
	app_state_set_nw_if(&sim_nw_if);
	app_state_set_nw_services(&sim_nw_services);
	app_state_set_modbus(&sim_modbus);
	if (sim_sensor_count == 0) {
		sim_sensor_count = 1;
		sim_sensors[0].id = 1;
		snprintf(sim_sensors[0].mac, sizeof(sim_sensors[0].mac), "%s",
				"AA:BB:CC:DD:EE:01");
		snprintf(sim_sensors[0].name, sizeof(sim_sensors[0].name), "%s",
				"Rack Ambient");
		sim_sensors[0].valid = true;
	}
	app_state_set_sensors(sim_sensors, sim_sensor_count);
	sim_initialized = true;
}

int backend_init(void)
{
	ensure_sim_outlets();
	app_state_set_license_type("B2");
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
		sim_outlets[line_id].status = status;
		app_state_set_outlet(line_id, status);
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
	app_state_set_outlets(sim_outlets, SIM_OUTLETS);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_outlet_data_refresh(int outlet_id, backend_callback_t callback,
		void* userdata)
{
	app_state_outlet_data_t outlet_data = {
		.voltage = 230.0f + (float)(outlet_id % 3),
		.current = 1.2f + (float)(outlet_id % 4) * 0.3f,
		.active_power = 275.0f + (float)outlet_id * 5.0f,
		.reactive_power = 12.0f,
		.apparent_power = 280.0f + (float)outlet_id * 5.0f,
		.power_factor = 0.98f,
		.phase = 0.0f,
		.frequency = 50.0f,
		.energy = 1200.0f + (float)outlet_id * 20.0f,
		.fuse = 1,
		.outlet_id = outlet_id,
		.valid = true,
	};

	snprintf(outlet_data.conn, sizeof(outlet_data.conn), "%s", "IEC C13");
	app_state_set_outlet_data(&outlet_data);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_license_refresh(backend_callback_t callback, void* userdata)
{
	app_state_set_license_type("B2");
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_power_refresh(backend_callback_t callback, void* userdata)
{
	app_state_power_t power;

	memset(&power, 0, sizeof(power));
	power.branch = 1;
	power.sys_type = 2;
	power.curr_type = 0;
	power.input_count = APP_STATE_MAX_POWER_INPUTS;
	power.valid = true;

	for (int i = 0; i < APP_STATE_MAX_POWER_INPUTS; i++) {
		power.inputs[i].voltage = 230.0f + (float)(i % 3);
		power.inputs[i].current = 2.0f + (float)i * 0.2f;
		power.inputs[i].active_power = power.inputs[i].voltage *
				power.inputs[i].current * 0.95f;
		power.inputs[i].reactive_power = 8.0f + (float)i;
		power.inputs[i].apparent_power = power.inputs[i].voltage *
				power.inputs[i].current;
		power.inputs[i].power_factor = 0.95f;
		power.inputs[i].phase = 0.0f;
		power.inputs[i].frequency = 50.0f;
		power.inputs[i].energy = 5000.0f + (float)i * 100.0f;
	}

	app_state_set_power(&power);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_sensor_data_refresh(int sensor_index, backend_callback_t callback,
		void* userdata)
{
	app_state_sensor_data_t sensor_data;

	memset(&sensor_data, 0, sizeof(sensor_data));
	snprintf(sensor_data.mac, sizeof(sensor_data.mac),
			"AA:BB:CC:DD:EE:%02X", sensor_index >= 0 ? sensor_index : 0);
	snprintf(sensor_data.name, sizeof(sensor_data.name), "Sensor %d",
			sensor_index + 1);
	snprintf(sensor_data.kind, sizeof(sensor_data.kind), "%s", "THP");
	sensor_data.temp = 24.5f + (float)sensor_index;
	sensor_data.humd = 45.0f + (float)(sensor_index % 10);
	sensor_data.pres = 1012.0f + (float)(sensor_index % 5);
	sensor_data.rssi = -48 - sensor_index;
	sensor_data.bat_mv = 2980 - (sensor_index * 12);
	sensor_data.bat_pct = 86 - sensor_index;
	sensor_data.sensor_index = sensor_index;
	sensor_data.valid = true;

	app_state_set_sensor_data(&sensor_data);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_update_status_refresh(backend_callback_t callback, void* userdata)
{
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_update_confirm(bool confirm, backend_callback_t callback,
		void* userdata)
{
	if (confirm) {
		sim_update_status.is_pending = false;
	}
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_update_set_auto(bool enabled, backend_callback_t callback,
		void* userdata)
{
	sim_update_status.auto_update = enabled;
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_update_set_interval(int hours, backend_callback_t callback,
		void* userdata)
{
	sim_update_status.check_interval_hours = hours;
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_update_set_server(const char* server, backend_callback_t callback,
		void* userdata)
{
	snprintf(sim_update_status.update_server,
			sizeof(sim_update_status.update_server), "%s",
			server != NULL ? server : "");
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_system_reboot(backend_callback_t callback, void* userdata)
{
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_system_factory_reset(backend_callback_t callback, void* userdata)
{
	snprintf(sim_update_status.update_server,
			sizeof(sim_update_status.update_server), "%s",
			"https://github.com/Network-Engineering-PDU/firmware-update");
	sim_update_status.auto_update = false;
	sim_update_status.check_interval_hours = 24;
	sim_update_status.is_pending = false;
	app_state_set_update_status(&sim_update_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_usb_update_detect(backend_callback_t callback, void* userdata)
{
	app_state_usb_update_t usb_update;

	memset(&usb_update, 0, sizeof(usb_update));
	usb_update.device_found = true;
	snprintf(usb_update.device_name, sizeof(usb_update.device_name), "%s",
			"SIM_USB");
	snprintf(usb_update.update_dev, sizeof(usb_update.update_dev), "%s",
			"sdb1");
	app_state_set_usb_update(&usb_update);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_usb_update_start(const char* update_dev, backend_callback_t callback,
		void* userdata)
{
	app_state_usb_update_t usb_update;

	memset(&usb_update, 0, sizeof(usb_update));
	snprintf(usb_update.update_dev, sizeof(usb_update.update_dev), "%s",
			update_dev != NULL ? update_dev : "sdb1");
	usb_update.running = true;
	sim_usb_running = true;
	app_state_set_usb_update(&usb_update);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_usb_update_poll(backend_callback_t callback, void* userdata)
{
	app_state_usb_update_t usb_update;

	memset(&usb_update, 0, sizeof(usb_update));
	snprintf(usb_update.update_dev, sizeof(usb_update.update_dev), "%s",
			"sdb1");
	usb_update.running = false;
	usb_update.complete = true;
	usb_update.result = sim_usb_running ? 0 : 1;
	sim_usb_running = false;
	app_state_set_usb_update(&usb_update);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_bluetooth_refresh(backend_callback_t callback, void* userdata)
{
	ensure_sim_outlets();
	app_state_set_bt_status(&sim_bt_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_bluetooth_set(bool powered, bool pairable, bool discoverable,
		backend_callback_t callback, void* userdata)
{
	ensure_sim_outlets();
	sim_bt_status.powered = powered;
	sim_bt_status.pairable = pairable;
	sim_bt_status.discoverable = discoverable;
	app_state_set_bt_status(&sim_bt_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_bluetooth_scan(bool enable, backend_callback_t callback,
		void* userdata)
{
	ensure_sim_outlets();
	sim_bt_status.discovering = enable;
	app_state_set_bt_status(&sim_bt_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_bluetooth_device_action(const char* mac, const char* action,
		backend_callback_t callback, void* userdata)
{
	ensure_sim_outlets();
	for (int i = 0; i < sim_bt_status.device_count; i++) {
		app_state_bt_device_t* device = &sim_bt_status.devices[i];
		if (strcmp(device->mac, mac != NULL ? mac : "") == 0) {
			device->connected = action != NULL && strcmp(action, "connect") == 0;
			if (device->connected) {
				device->paired = true;
			}
			break;
		}
	}
	app_state_set_bt_status(&sim_bt_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_bluetooth_pairing_response(bool accept, backend_callback_t callback,
		void* userdata)
{
	ensure_sim_outlets();
	(void)accept;
	sim_bt_status.pairing_request = false;
	app_state_set_bt_status(&sim_bt_status);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_network_if_refresh(backend_callback_t callback, void* userdata)
{
	ensure_sim_outlets();
	app_state_set_nw_if(&sim_nw_if);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_network_if_save(const app_state_nw_if_t* nw_if,
		backend_callback_t callback, void* userdata)
{
	if (nw_if != NULL) {
		sim_nw_if = *nw_if;
		sim_nw_if.valid = true;
		app_state_set_nw_if(&sim_nw_if);
	}
	if (callback != NULL) {
		callback(nw_if != NULL ? 0 : 1, userdata);
	}
	return nw_if != NULL ? 0 : -1;
}

int backend_network_info_refresh(backend_callback_t callback, void* userdata)
{
	app_state_nw_info_t nw_info = {
		.connected = true,
		.valid = true,
	};
	app_state_set_nw_info(&nw_info);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_network_services_refresh(backend_callback_t callback, void* userdata)
{
	ensure_sim_outlets();
	app_state_set_nw_services(&sim_nw_services);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_network_service_set(const char* service, bool enable,
		backend_callback_t callback, void* userdata)
{
	ensure_sim_outlets();
	if (strcmp(service != NULL ? service : "", "ssh") == 0) {
		sim_nw_services.ssh = enable;
	} else if (strcmp(service != NULL ? service : "", "snmp") == 0) {
		sim_nw_services.snmp = enable;
	} else if (strcmp(service != NULL ? service : "", "modbus") == 0) {
		sim_nw_services.modbus = enable;
	}
	app_state_set_nw_services(&sim_nw_services);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_modbus_refresh(backend_callback_t callback, void* userdata)
{
	ensure_sim_outlets();
	app_state_set_modbus(&sim_modbus);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_modbus_set_addr(int addr, backend_callback_t callback,
		void* userdata)
{
	sim_modbus.addr = addr;
	app_state_set_modbus(&sim_modbus);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_sensors_refresh(backend_callback_t callback, void* userdata)
{
	ensure_sim_outlets();
	app_state_set_sensors(sim_sensors, sim_sensor_count);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_ble_scan_start(backend_callback_t callback, void* userdata)
{
	ensure_sim_outlets();
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_ble_scan_stop(backend_callback_t callback, void* userdata)
{
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_ble_discovered_refresh(backend_callback_t callback, void* userdata)
{
	app_state_set_discovered_sensors(sim_discovered, 2);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_ble_confirm_mac(const char* mac, backend_callback_t callback,
		void* userdata)
{
	ensure_sim_outlets();
	if (sim_sensor_count < APP_STATE_MAX_SENSORS) {
		app_state_sensor_t* sensor = &sim_sensors[sim_sensor_count];
		memset(sensor, 0, sizeof(*sensor));
		sensor->id = sim_sensor_count + 1;
		snprintf(sensor->mac, sizeof(sensor->mac), "%s",
				mac != NULL ? mac : "");
		snprintf(sensor->name, sizeof(sensor->name), "Sensor %d",
				sim_sensor_count + 1);
		sensor->valid = true;
		sim_sensor_count++;
	}
	app_state_set_sensors(sim_sensors, sim_sensor_count);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

int backend_ble_confirm_all(backend_callback_t callback, void* userdata)
{
	for (int i = 0; i < 2 && sim_sensor_count < APP_STATE_MAX_SENSORS; i++) {
		backend_ble_confirm_mac(sim_discovered[i].mac, NULL, NULL);
	}
	app_state_set_sensors(sim_sensors, sim_sensor_count);
	if (callback != NULL) {
		callback(0, userdata);
	}
	return 0;
}

#endif /* SIMULATOR_ENABLED */
