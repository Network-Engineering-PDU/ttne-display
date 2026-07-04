#ifndef SIMULATOR_ENABLED

#include "backend/backend.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "controller.h"
#include "models.h"
#include "runbg.h"
#include "config.h"
#include "app/app_state.h"

extern void reset_program(void);

#define BACKEND_QUEUE_SIZE 32

#ifdef SIMULATOR_ENABLED
#define USB_MOUNT_DIR "/home/guille"
#else
#define USB_MOUNT_DIR "/run/mount"
#endif

typedef enum {
	BACKEND_CMD_NONE = 0,
	BACKEND_CMD_OUTLETS_REFRESH,
	BACKEND_CMD_OUTLET_SET,
	BACKEND_CMD_OUTLETS_SET_ALL,
	BACKEND_CMD_OUTLET_DATA_REFRESH,
	BACKEND_CMD_LICENSE_REFRESH,
	BACKEND_CMD_POWER_REFRESH,
	BACKEND_CMD_SENSOR_DATA_REFRESH,
	BACKEND_CMD_UPDATE_STATUS_REFRESH,
	BACKEND_CMD_UPDATE_CONFIRM,
	BACKEND_CMD_UPDATE_SET_AUTO,
	BACKEND_CMD_UPDATE_SET_INTERVAL,
	BACKEND_CMD_UPDATE_SET_SERVER,
	BACKEND_CMD_SYSTEM_REBOOT,
	BACKEND_CMD_SYSTEM_FACTORY_RESET,
	BACKEND_CMD_USB_UPDATE_DETECT,
	BACKEND_CMD_USB_UPDATE_START,
	BACKEND_CMD_USB_UPDATE_POLL,
	BACKEND_CMD_BLUETOOTH_REFRESH,
	BACKEND_CMD_BLUETOOTH_SET,
	BACKEND_CMD_BLUETOOTH_SCAN,
	BACKEND_CMD_BLUETOOTH_DEVICE_ACTION,
	BACKEND_CMD_BLUETOOTH_PAIRING_RESPONSE,
	BACKEND_CMD_NETWORK_IF_REFRESH,
	BACKEND_CMD_NETWORK_IF_SAVE,
	BACKEND_CMD_NETWORK_INFO_REFRESH,
	BACKEND_CMD_NETWORK_SERVICES_REFRESH,
	BACKEND_CMD_NETWORK_SERVICE_SET,
	BACKEND_CMD_MODBUS_REFRESH,
	BACKEND_CMD_MODBUS_SET_ADDR,
	BACKEND_CMD_SENSORS_REFRESH,
	BACKEND_CMD_BLE_SCAN_START,
	BACKEND_CMD_BLE_SCAN_STOP,
	BACKEND_CMD_BLE_DISCOVERED_REFRESH,
	BACKEND_CMD_BLE_CONFIRM_MAC,
	BACKEND_CMD_BLE_CONFIRM_ALL,
	BACKEND_CMD_SYSTEM_INFO_REFRESH,
	BACKEND_CMD_PDU_INFO_REFRESH,
	BACKEND_CMD_PDU_SET_RATED_CURRENT,
	BACKEND_CMD_VISUAL_CONFIG_REFRESH,
	BACKEND_CMD_VISUAL_SET_INACTIVITY,
	BACKEND_CMD_VISUAL_SET_PDU_FIELD,
	BACKEND_CMD_VISUAL_SAVE_ROTATION_RESTART,
	BACKEND_CMD_LOGIN_CONFIG_REFRESH,
	BACKEND_CMD_LOGIN_SET_SKIP,
} backend_cmd_type_t;

typedef struct {
	backend_cmd_type_t type;
	int line_id;
	int value;
	bool status;
	bool pairable;
	bool discoverable;
	char text[128];
	char action[32];
	app_state_nw_if_t nw_if;
	backend_callback_t callback;
	void* userdata;
} backend_cmd_t;

typedef struct {
	int err;
	backend_callback_t callback;
	void* userdata;
} backend_result_t;

static backend_cmd_t queue[BACKEND_QUEUE_SIZE];
static backend_result_t results[BACKEND_QUEUE_SIZE];
static int queue_head;
static int queue_tail;
static int queue_count;
static int result_count;
static bool shutdown_requested;
static pid_t usb_update_pid = -1;
static char usb_update_dev[32];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_t worker;
static bool worker_started;

static void publish_outlets_from_models(void)
{
	int len;
	const models_out_sw_t* model_outlets = models_get_out_sw(&len);
	app_state_outlet_t outlets[APP_STATE_MAX_OUTLETS];

	if (model_outlets == NULL || len <= 0) {
		app_state_set_outlets(NULL, 0);
		return;
	}

	if (len > APP_STATE_MAX_OUTLETS) {
		len = APP_STATE_MAX_OUTLETS;
	}

	for (int i = 0; i < len; i++) {
		outlets[i].line_id = model_outlets[i].line_id;
		outlets[i].status = model_outlets[i].status;
	}
	app_state_set_outlets(outlets, len);
}

static void publish_outlet_data_from_models(int outlet_id)
{
	const models_out_data_t* model_data = models_get_out_data();
	app_state_outlet_data_t outlet_data = {
		.voltage = model_data->voltage,
		.current = model_data->current,
		.active_power = model_data->active_power,
		.reactive_power = model_data->reactive_power,
		.apparent_power = model_data->apparent_power,
		.power_factor = model_data->power_factor,
		.phase = model_data->phase,
		.frequency = model_data->frequency,
		.energy = model_data->energy,
		.fuse = model_data->fuse,
		.outlet_id = outlet_id,
		.valid = true,
	};

	snprintf(outlet_data.conn, sizeof(outlet_data.conn), "%s",
			model_data->conn != NULL ? model_data->conn : "N/A");
	app_state_set_outlet_data(&outlet_data);
}

static void publish_license_from_models(void)
{
	const models_license_t* license = models_get_license();
	app_state_set_license_type(license != NULL ? license->type_id : "N/A");
}

static void publish_update_status_from_models(void)
{
	const models_update_status_t* model_status = models_get_update_status();
	app_state_update_status_t update_status;

	memset(&update_status, 0, sizeof(update_status));
	if (model_status != NULL) {
		update_status.is_pending = model_status->is_pending;
		update_status.auto_update = model_status->auto_update;
		update_status.check_interval_hours = model_status->check_interval_hours;
		snprintf(update_status.update_server,
				sizeof(update_status.update_server), "%s",
				model_status->update_server != NULL
						? model_status->update_server : "N/A");
	} else {
		update_status.check_interval_hours = 168;
		snprintf(update_status.update_server,
				sizeof(update_status.update_server), "%s", "N/A");
	}

	app_state_set_update_status(&update_status);
}

static void publish_bluetooth_from_models(void)
{
	const models_bt_status_t* model_status = models_get_bt_status();
	app_state_bt_status_t bt_status;

	memset(&bt_status, 0, sizeof(bt_status));
	if (model_status == NULL) {
		app_state_set_bt_status(&bt_status);
		return;
	}

	snprintf(bt_status.controller_mac, sizeof(bt_status.controller_mac), "%s",
			model_status->controller_mac != NULL ?
					model_status->controller_mac : "");
	snprintf(bt_status.name, sizeof(bt_status.name), "%s",
			model_status->name != NULL ? model_status->name : "");
	bt_status.powered = model_status->powered;
	bt_status.pairable = model_status->pairable;
	bt_status.discoverable = model_status->discoverable;
	bt_status.discovering = model_status->discovering;
	bt_status.pairing_request = model_status->pairing_request;
	snprintf(bt_status.pairing_mac, sizeof(bt_status.pairing_mac), "%s",
			model_status->pairing_mac != NULL ? model_status->pairing_mac : "");
	snprintf(bt_status.pairing_name, sizeof(bt_status.pairing_name), "%s",
			model_status->pairing_name != NULL ? model_status->pairing_name : "");
	snprintf(bt_status.pairing_passkey, sizeof(bt_status.pairing_passkey), "%s",
			model_status->pairing_passkey != NULL ?
					model_status->pairing_passkey : "");

	bt_status.device_count = model_status->device_count;
	if (bt_status.device_count > APP_STATE_MAX_BT_DEVICES) {
		bt_status.device_count = APP_STATE_MAX_BT_DEVICES;
	}
	if (bt_status.device_count < 0) {
		bt_status.device_count = 0;
	}
	for (int i = 0; i < bt_status.device_count; i++) {
		const models_bt_device_t* model_device = &model_status->devices[i];
		app_state_bt_device_t* device = &bt_status.devices[i];
		snprintf(device->mac, sizeof(device->mac), "%s",
				model_device->mac != NULL ? model_device->mac : "");
		snprintf(device->name, sizeof(device->name), "%s",
				model_device->name != NULL ? model_device->name : "");
		device->paired = model_device->paired;
		device->trusted = model_device->trusted;
		device->connected = model_device->connected;
		device->rssi = model_device->rssi;
	}

	app_state_set_bt_status(&bt_status);
}

static void publish_network_if_from_models(void)
{
	const models_nw_if_t* model = models_get_nw_if();
	app_state_nw_if_t nw_if;

	memset(&nw_if, 0, sizeof(nw_if));
	if (model == NULL) {
		nw_if.type = UNCONF;
		nw_if.dhcp = true;
		nw_if.nw_mode = -1;
		app_state_set_nw_if(&nw_if);
		return;
	}

	nw_if.type = model->type;
	nw_if.dhcp = model->dhcp;
	nw_if.nw_mode = model->nw_mode;
	snprintf(nw_if.eth_interface, sizeof(nw_if.eth_interface), "%s",
			model->eth_interface != NULL ? model->eth_interface : "");
	snprintf(nw_if.ip, sizeof(nw_if.ip), "%s",
			model->params.ip != NULL ? model->params.ip : "");
	snprintf(nw_if.mask, sizeof(nw_if.mask), "%s",
			model->params.mask != NULL ? model->params.mask : "");
	snprintf(nw_if.gw, sizeof(nw_if.gw), "%s",
			model->params.gw != NULL ? model->params.gw : "");
	snprintf(nw_if.dns, sizeof(nw_if.dns), "%s",
			model->params.dns != NULL ? model->params.dns : "");
	snprintf(nw_if.ssid, sizeof(nw_if.ssid), "%s",
			model->params.ssid != NULL ? model->params.ssid : "");
	snprintf(nw_if.pass, sizeof(nw_if.pass), "%s",
			model->params.pass != NULL ? model->params.pass : "");
	snprintf(nw_if.lan1_ip, sizeof(nw_if.lan1_ip), "%s",
			model->lan1_ip != NULL ? model->lan1_ip : "");
	snprintf(nw_if.lan2_ip, sizeof(nw_if.lan2_ip), "%s",
			model->lan2_ip != NULL ? model->lan2_ip : "");
	snprintf(nw_if.wifi_ip, sizeof(nw_if.wifi_ip), "%s",
			model->wifi_ip != NULL ? model->wifi_ip : "");

	app_state_set_nw_if(&nw_if);
}

static void publish_network_info_from_models(void)
{
	const models_nw_info_t* model = models_get_nw_info();
	app_state_nw_info_t nw_info = {
		.connected = model != NULL && model->connected,
		.valid = true,
	};
	app_state_set_nw_info(&nw_info);
}

static void publish_network_services_from_models(void)
{
	const models_nw_services_t* model = models_get_nw_services();
	app_state_nw_services_t services = {
		.snmp = model != NULL && model->snmp,
		.modbus = model != NULL && model->modbus,
		.ssh = model != NULL && model->ssh,
		.bluetooth = model != NULL && model->bluetooth,
		.valid = true,
	};
	app_state_set_nw_services(&services);
}

static void publish_modbus_from_models(void)
{
	const models_modbus_t* model = models_get_modbus();
	app_state_modbus_t modbus = {
		.addr = model != NULL ? model->addr : 0,
		.valid = true,
	};
	app_state_set_modbus(&modbus);
}

static void publish_sensors_from_models(void)
{
	int len;
	const models_sensor_t* model_sensors = models_get_sensor(&len);
	app_state_sensor_t sensors[APP_STATE_MAX_SENSORS];

	memset(sensors, 0, sizeof(sensors));
	if (model_sensors == NULL || len <= 0) {
		app_state_set_sensors(NULL, 0);
		return;
	}
	if (len > APP_STATE_MAX_SENSORS) {
		len = APP_STATE_MAX_SENSORS;
	}
	for (int i = 0; i < len; i++) {
		sensors[i].id = model_sensors[i].id;
		snprintf(sensors[i].mac, sizeof(sensors[i].mac), "%s",
				model_sensors[i].mac != NULL ? model_sensors[i].mac : "");
		snprintf(sensors[i].name, sizeof(sensors[i].name), "%s",
				model_sensors[i].name != NULL ? model_sensors[i].name : "");
		sensors[i].valid = true;
	}
	app_state_set_sensors(sensors, len);
}

static void publish_discovered_from_models(void)
{
	int len;
	const models_discovered_sensor_t* model_sensors =
			models_get_discovered(&len);
	app_state_discovered_sensor_t sensors[APP_STATE_MAX_DISCOVERED_SENSORS];

	memset(sensors, 0, sizeof(sensors));
	if (model_sensors == NULL || len <= 0) {
		app_state_set_discovered_sensors(NULL, 0);
		return;
	}
	if (len > APP_STATE_MAX_DISCOVERED_SENSORS) {
		len = APP_STATE_MAX_DISCOVERED_SENSORS;
	}
	for (int i = 0; i < len; i++) {
		snprintf(sensors[i].mac, sizeof(sensors[i].mac), "%s",
				model_sensors[i].mac != NULL ? model_sensors[i].mac : "");
		snprintf(sensors[i].kind, sizeof(sensors[i].kind), "%s",
				model_sensors[i].kind != NULL ? model_sensors[i].kind : "");
		snprintf(sensors[i].name, sizeof(sensors[i].name), "%s",
				model_sensors[i].name != NULL ? model_sensors[i].name : "");
		sensors[i].rssi = model_sensors[i].rssi;
		sensors[i].valid = true;
	}
	app_state_set_discovered_sensors(sensors, len);
}

static void publish_system_info_from_models(void)
{
	const models_info_t* model = models_get_info();
	app_state_system_info_t info;

	memset(&info, 0, sizeof(info));
	if (model != NULL) {
		snprintf(info.product_name, sizeof(info.product_name), "%s",
				model->product_name != NULL ? model->product_name : "N/A");
		snprintf(info.product_pn, sizeof(info.product_pn), "%s",
				model->product_pn != NULL ? model->product_pn : "N/A");
		snprintf(info.product_sn, sizeof(info.product_sn), "%s",
				model->product_sn != NULL ? model->product_sn : "N/A");
		snprintf(info.lan_mac, sizeof(info.lan_mac), "%s",
				model->lan_mac != NULL ? model->lan_mac : "N/A");
		snprintf(info.ip, sizeof(info.ip), "%s",
				model->ip != NULL ? model->ip : "N/A");
		snprintf(info.sw_version, sizeof(info.sw_version), "%s",
				model->sw_version != NULL ? model->sw_version : "N/A");
		snprintf(info.om_version, sizeof(info.om_version), "%s",
				model->om_version != NULL ? model->om_version : "N/A");
		snprintf(info.pmb_version, sizeof(info.pmb_version), "%s",
				model->pmb_version != NULL ? model->pmb_version : "N/A");
		snprintf(info.uptime, sizeof(info.uptime), "%s",
				model->uptime != NULL ? model->uptime : "N/A");
	} else {
		snprintf(info.product_name, sizeof(info.product_name), "%s", "N/A");
		snprintf(info.ip, sizeof(info.ip), "%s", "N/A");
	}
	app_state_set_system_info(&info);
}

static void publish_pdu_info_from_models(void)
{
	const models_pdu_info_t* model = models_get_pdu_info();
	app_state_pdu_info_t pdu_info;

	memset(&pdu_info, 0, sizeof(pdu_info));
	pdu_info.rated_current = 32;
	if (model != NULL) {
		pdu_info.n_outlets = model->n_outlets;
		pdu_info.rated_current = model->rated_current;
		snprintf(pdu_info.controller, sizeof(pdu_info.controller), "%s",
				model->controller != NULL ? model->controller : "N/A");
		snprintf(pdu_info.type, sizeof(pdu_info.type), "%s",
				model->type != NULL ? model->type : "N/A");
	}
	app_state_set_pdu_info(&pdu_info);
}

static void publish_visual_config_from_config(void)
{
	app_state_visual_config_t visual_config;

	memset(&visual_config, 0, sizeof(visual_config));
	visual_config.rotation = config_get_rotation();
	visual_config.inactivity_time = config_get_inactivity_time();
	snprintf(visual_config.pdu_company, sizeof(visual_config.pdu_company),
			"%s", config_get_pdu_company());
	snprintf(visual_config.pdu_rack, sizeof(visual_config.pdu_rack),
			"%s", config_get_pdu_rack());
	snprintf(visual_config.pdu_system, sizeof(visual_config.pdu_system),
			"%s", config_get_pdu_system());
	snprintf(visual_config.pdu_ups, sizeof(visual_config.pdu_ups),
			"%s", config_get_pdu_ups());
	snprintf(visual_config.pdu_elec_board,
			sizeof(visual_config.pdu_elec_board), "%s",
			config_get_pdu_elec_board());
	snprintf(visual_config.pdu_breaker, sizeof(visual_config.pdu_breaker),
			"%s", config_get_pdu_breaker());
	snprintf(visual_config.pdu_service, sizeof(visual_config.pdu_service),
			"%s", config_get_pdu_service());
	app_state_set_visual_config(&visual_config);
}

static void publish_login_config_from_config(void)
{
	app_state_login_config_t login_config;

	memset(&login_config, 0, sizeof(login_config));
	login_config.skip_login = config_get_skip_login() != 0;
	app_state_set_login_config(&login_config);
}

static void set_visual_pdu_field(int field_id, const char* value)
{
	switch (field_id) {
	case 0:
		config_set_pdu_company(value);
		break;
	case 1:
		config_set_pdu_rack(value);
		break;
	case 2:
		config_set_pdu_system(value);
		break;
	case 3:
		config_set_pdu_ups(value);
		break;
	case 4:
		config_set_pdu_elec_board(value);
		break;
	case 5:
		config_set_pdu_breaker(value);
		break;
	case 6:
		config_set_pdu_service(value);
		break;
	default:
		break;
	}
}

static void model_from_app_network_if(const app_state_nw_if_t* nw_if,
		models_nw_if_t* model)
{
	memset(model, 0, sizeof(*model));
	model->type = nw_if->type;
	model->dhcp = nw_if->dhcp;
	model->eth_interface = nw_if->eth_interface;
	model->params.ip = nw_if->ip;
	model->params.mask = nw_if->mask;
	model->params.gw = nw_if->gw;
	model->params.dns = nw_if->dns;
	model->params.ssid = nw_if->ssid;
	model->params.pass = nw_if->pass;
	model->lan1_ip = nw_if->lan1_ip;
	model->lan2_ip = nw_if->lan2_ip;
	model->wifi_ip = nw_if->wifi_ip;
	model->nw_mode = nw_if->nw_mode;
}

static int run_power_refresh(void)
{
	app_state_power_t power;
	int phase_count;
	int branch_count;
	memset(&power, 0, sizeof(power));

	if (controller_get_in_sw() != 0) {
		return 1;
	}
	const models_in_sw_t* in_sw = models_get_in_sw();
	power.branch = in_sw->branch;
	power.sys_type = in_sw->sys_type;
	power.curr_type = in_sw->curr_type;
	branch_count = power.branch == 1 ? 2 : 1;
	phase_count = power.sys_type == 0 ? 1 :
			(power.sys_type == 1 ? 2 : 3);
	power.input_count = branch_count * phase_count;
	if (power.input_count > APP_STATE_MAX_POWER_INPUTS) {
		power.input_count = APP_STATE_MAX_POWER_INPUTS;
	}

	for (int i = 0; i < power.input_count; i++) {
		if (controller_get_in_data(i) != 0) {
			return 1;
		}
		const models_in_data_t* in_data = models_get_in_data();
		power.inputs[i].voltage = in_data->voltage;
		power.inputs[i].current = in_data->current;
		power.inputs[i].active_power = in_data->active_power;
		power.inputs[i].reactive_power = in_data->reactive_power;
		power.inputs[i].apparent_power = in_data->apparent_power;
		power.inputs[i].power_factor = in_data->power_factor;
		power.inputs[i].phase = in_data->phase;
		power.inputs[i].frequency = in_data->frequency;
		power.inputs[i].energy = in_data->energy;
	}

	app_state_set_power(&power);
	return 0;
}

static void publish_sensor_data_from_live(int sensor_index,
		const models_sensor_live_t* live, const models_sensor_t* stored)
{
	app_state_sensor_data_t sensor_data;
	const char* name;
	const char* kind;
	int bat_mv;

	if (live == NULL) {
		return;
	}

	memset(&sensor_data, 0, sizeof(sensor_data));
	sensor_data.temp = live->temp;
	sensor_data.humd = live->humd;
	sensor_data.pres = live->pres;
	sensor_data.rssi = live->rssi;
	sensor_data.bat_mv = live->bat_mv;
	sensor_data.bat_pct = live->bat_pct;
	sensor_data.sensor_index = sensor_index;

	name = live->name;
	kind = live->kind;
	bat_mv = sensor_data.bat_mv;
	if (stored != NULL) {
		if (name == NULL || name[0] == '\0') {
			name = stored->name;
		}
		if (sensor_data.temp != sensor_data.temp) {
			sensor_data.temp = stored->last_data.temp;
		}
		if (sensor_data.humd != sensor_data.humd) {
			sensor_data.humd = stored->last_data.humd;
		}
		if (sensor_data.pres != sensor_data.pres) {
			sensor_data.pres = stored->last_data.pres;
		}
		if (sensor_data.rssi <= -200 && stored->last_data.rssi > -200) {
			sensor_data.rssi = stored->last_data.rssi;
		}
		if (bat_mv <= 0 && stored->last_data.bat > 0) {
			bat_mv = (stored->last_data.bat < 20.0f)
					? (int)(stored->last_data.bat * 1000.0f)
					: stored->last_data.bat;
		}
	}

	sensor_data.bat_mv = bat_mv;
	snprintf(sensor_data.mac, sizeof(sensor_data.mac), "%s",
			live->mac != NULL ? live->mac : "");
	snprintf(sensor_data.name, sizeof(sensor_data.name), "%s",
			name != NULL ? name : "");
	snprintf(sensor_data.kind, sizeof(sensor_data.kind), "%s",
			kind != NULL ? kind : "");
	app_state_set_sensor_data(&sensor_data);
}

static int run_sensor_data_refresh(int sensor_index)
{
	int len;
	const models_sensor_t* sensors = models_get_sensor(&len);
	const models_sensor_t* stored;

	if (sensor_index < 0 || sensor_index >= len) {
		controller_get_sensors();
		sensors = models_get_sensor(&len);
	}
	if (sensors == NULL || sensor_index < 0 || sensor_index >= len) {
		return 1;
	}

	stored = &sensors[sensor_index];
	if (stored->mac != NULL && controller_get_sensor_live(stored->mac)) {
		publish_sensor_data_from_live(sensor_index, models_get_sensor_live(),
				stored);
		return 0;
	}

	controller_get_sensors();
	sensors = models_get_sensor(&len);
	if (sensors == NULL || sensor_index < 0 || sensor_index >= len) {
		return 1;
	}

	stored = &sensors[sensor_index];
	models_sensor_live_t fallback = {
		.mac = stored->mac,
		.kind = "",
		.name = stored->name,
		.temp = stored->last_data.temp,
		.humd = stored->last_data.humd,
		.pres = stored->last_data.pres,
		.rssi = stored->last_data.rssi,
		.bat_mv = (stored->last_data.bat < 20.0f)
				? (int)(stored->last_data.bat * 1000.0f)
				: stored->last_data.bat,
		.bat_pct = -1,
	};
	publish_sensor_data_from_live(sensor_index, &fallback, NULL);
	return 0;
}

static const char* path_basename(const char* path)
{
	const char* base;

	if (path == NULL) {
		return "";
	}
	base = strrchr(path, '/');
	return base != NULL ? base + 1 : path;
}

static int run_usb_update_detect(void)
{
	FILE* file = fopen("/proc/mounts", "r");
	app_state_usb_update_t usb_update;
	char line[500];
	char dev[80];
	char path[160];

	memset(&usb_update, 0, sizeof(usb_update));
	if (file == NULL) {
		app_state_set_usb_update(&usb_update);
		return 1;
	}

	while (fgets(line, sizeof(line), file) != NULL) {
		dev[0] = '\0';
		path[0] = '\0';
		if (sscanf(line, "%79s %159s", dev, path) != 2) {
			continue;
		}
		if (strstr(path, USB_MOUNT_DIR) != NULL) {
			usb_update.device_found = true;
			snprintf(usb_update.device_name, sizeof(usb_update.device_name),
					"%s", path_basename(path));
			snprintf(usb_update.update_dev, sizeof(usb_update.update_dev),
					"%s", path_basename(dev));
			break;
		}
	}
	fclose(file);

	app_state_set_usb_update(&usb_update);
	return usb_update.device_found ? 0 : 1;
}

static int run_usb_update_start(const char* update_dev)
{
	app_state_usb_update_t usb_update;

	memset(&usb_update, 0, sizeof(usb_update));
	snprintf(usb_update.update_dev, sizeof(usb_update.update_dev), "%s",
			update_dev != NULL ? update_dev : "");
	if (usb_update.update_dev[0] == '\0') {
		app_state_set_usb_update(&usb_update);
		return 1;
	}

	usb_update_pid = runbg_run("/usr/bin/usb_autorun.sh", "add",
			usb_update.update_dev, NULL);
	snprintf(usb_update_dev, sizeof(usb_update_dev), "%s",
			usb_update.update_dev);
	usb_update.running = usb_update_pid > 0;
	usb_update.complete = false;
	usb_update.result = usb_update.running ? 0 : 1;
	app_state_set_usb_update(&usb_update);
	return usb_update.running ? 0 : 1;
}

static int run_usb_update_poll(void)
{
	app_state_usb_update_t usb_update;
	int running;
	int result;

	memset(&usb_update, 0, sizeof(usb_update));
	snprintf(usb_update.update_dev, sizeof(usb_update.update_dev), "%s",
			usb_update_dev);
	if (usb_update_pid <= 0) {
		usb_update.complete = true;
		usb_update.result = 1;
		app_state_set_usb_update(&usb_update);
		return 1;
	}

	result = runbg_check(usb_update_pid, &running);
	if (result != 0) {
		usb_update.complete = true;
		usb_update.result = result;
		app_state_set_usb_update(&usb_update);
		return result;
	}

	if (running) {
		usb_update.running = true;
		app_state_set_usb_update(&usb_update);
		return 0;
	}

	pid_t pid = runbg_run("/usr/bin/usb_autorun.sh", "remove",
			usb_update_dev, NULL);
	usb_update.result = runbg_check_wait(pid);
	usb_update.complete = true;
	usb_update.running = false;
	usb_update_pid = -1;
	usb_update_dev[0] = '\0';
	app_state_set_usb_update(&usb_update);
	return usb_update.result;
}

static void backend_push_result(int err, backend_callback_t callback, void* userdata)
{
	if (callback == NULL) {
		return;
	}

	pthread_mutex_lock(&mutex);
	if (result_count < BACKEND_QUEUE_SIZE) {
		results[result_count].err = err;
		results[result_count].callback = callback;
		results[result_count].userdata = userdata;
		result_count++;
	}
	pthread_mutex_unlock(&mutex);
}

static int backend_submit(backend_cmd_t* cmd)
{
	pthread_mutex_lock(&mutex);
	if (queue_count >= BACKEND_QUEUE_SIZE || shutdown_requested) {
		pthread_mutex_unlock(&mutex);
		return -1;
	}

	queue[queue_tail] = *cmd;
	queue_tail = (queue_tail + 1) % BACKEND_QUEUE_SIZE;
	queue_count++;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
	return 0;
}

static bool backend_pop_command(backend_cmd_t* cmd)
{
	pthread_mutex_lock(&mutex);
	while (queue_count == 0 && !shutdown_requested) {
		pthread_cond_wait(&cond, &mutex);
	}

	if (queue_count == 0 && shutdown_requested) {
		pthread_mutex_unlock(&mutex);
		return false;
	}

	*cmd = queue[queue_head];
	memset(&queue[queue_head], 0, sizeof(queue[queue_head]));
	queue_head = (queue_head + 1) % BACKEND_QUEUE_SIZE;
	queue_count--;
	pthread_mutex_unlock(&mutex);
	return true;
}

static void run_outlets_set_all(bool status)
{
	int len;
	models_get_out_sw(&len);
	if (len <= 0) {
		controller_get_out_sw();
		models_get_out_sw(&len);
	}

	for (int i = 0; i < len; i++) {
		models_out_sw_t out_sw = {
			.line_id = i,
			.status = status,
		};
		controller_put_out_sw(&out_sw, i);
	}
	publish_outlets_from_models();
}

static void* backend_worker(void* arg)
{
	(void)arg;

	while (true) {
		backend_cmd_t cmd;
		int err = 0;

		if (!backend_pop_command(&cmd)) {
			break;
		}

		switch (cmd.type) {
		case BACKEND_CMD_OUTLETS_REFRESH:
			controller_get_out_sw();
			publish_outlets_from_models();
			break;
		case BACKEND_CMD_OUTLET_SET: {
			models_out_sw_t out_sw = {
				.line_id = cmd.line_id,
				.status = cmd.status,
			};
			controller_put_out_sw(&out_sw, cmd.line_id);
			app_state_set_outlet(cmd.line_id, cmd.status);
			break;
		}
		case BACKEND_CMD_OUTLETS_SET_ALL:
			run_outlets_set_all(cmd.status);
			break;
		case BACKEND_CMD_OUTLET_DATA_REFRESH:
			controller_get_out_data(cmd.line_id);
			publish_outlet_data_from_models(cmd.line_id);
			break;
		case BACKEND_CMD_LICENSE_REFRESH:
			controller_get_license();
			publish_license_from_models();
			break;
		case BACKEND_CMD_POWER_REFRESH:
			err = run_power_refresh();
			break;
		case BACKEND_CMD_SENSOR_DATA_REFRESH:
			err = run_sensor_data_refresh(cmd.line_id);
			break;
		case BACKEND_CMD_UPDATE_STATUS_REFRESH:
			controller_get_update_status();
			publish_update_status_from_models();
			break;
		case BACKEND_CMD_UPDATE_CONFIRM:
			controller_post_update_confirm(cmd.status);
			controller_get_update_status();
			publish_update_status_from_models();
			break;
		case BACKEND_CMD_UPDATE_SET_AUTO:
			controller_set_auto_update(cmd.status);
			publish_update_status_from_models();
			break;
		case BACKEND_CMD_UPDATE_SET_INTERVAL:
			controller_set_update_check_interval(cmd.value);
			publish_update_status_from_models();
			break;
		case BACKEND_CMD_UPDATE_SET_SERVER:
			controller_set_update_server(cmd.text);
			publish_update_status_from_models();
			break;
		case BACKEND_CMD_SYSTEM_REBOOT:
			controller_post_reboot();
			break;
		case BACKEND_CMD_SYSTEM_FACTORY_RESET:
			controller_post_fact_reset();
			break;
		case BACKEND_CMD_USB_UPDATE_DETECT:
			err = run_usb_update_detect();
			break;
		case BACKEND_CMD_USB_UPDATE_START:
			err = run_usb_update_start(cmd.text);
			break;
		case BACKEND_CMD_USB_UPDATE_POLL:
			err = run_usb_update_poll();
			break;
		case BACKEND_CMD_BLUETOOTH_REFRESH:
			controller_get_bluetooth();
			publish_bluetooth_from_models();
			break;
		case BACKEND_CMD_BLUETOOTH_SET:
			controller_put_bluetooth(cmd.status, cmd.pairable,
					cmd.discoverable);
			controller_get_bluetooth();
			publish_bluetooth_from_models();
			break;
		case BACKEND_CMD_BLUETOOTH_SCAN:
			controller_post_bluetooth_scan(cmd.status);
			controller_get_bluetooth();
			publish_bluetooth_from_models();
			break;
		case BACKEND_CMD_BLUETOOTH_DEVICE_ACTION:
			controller_post_bluetooth_device_action(cmd.text, cmd.action);
			controller_get_bluetooth();
			publish_bluetooth_from_models();
			break;
		case BACKEND_CMD_BLUETOOTH_PAIRING_RESPONSE:
			controller_post_bluetooth_pairing_response(cmd.status);
			controller_get_bluetooth();
			publish_bluetooth_from_models();
			break;
		case BACKEND_CMD_NETWORK_IF_REFRESH:
			controller_get_nw_if();
			publish_network_if_from_models();
			break;
		case BACKEND_CMD_NETWORK_IF_SAVE: {
			models_nw_if_t model;
			model_from_app_network_if(&cmd.nw_if, &model);
			controller_put_nw_if(&model);
			publish_network_if_from_models();
			break;
		}
		case BACKEND_CMD_NETWORK_INFO_REFRESH:
			controller_get_nw_info();
			publish_network_info_from_models();
			break;
		case BACKEND_CMD_NETWORK_SERVICES_REFRESH:
			controller_get_nw_services();
			publish_network_services_from_models();
			break;
		case BACKEND_CMD_NETWORK_SERVICE_SET:
			if (strcmp(cmd.text, "ssh") == 0) {
				if (cmd.status) {
					controller_post_start_ssh();
				} else {
					controller_post_stop_ssh();
				}
			} else if (strcmp(cmd.text, "snmp") == 0) {
				if (cmd.status) {
					controller_post_start_snmp();
				} else {
					controller_post_stop_snmp();
				}
			} else if (strcmp(cmd.text, "modbus") == 0) {
				if (cmd.status) {
					controller_post_start_modbus();
				} else {
					controller_post_stop_modbus();
				}
			} else {
				err = 1;
			}
			if (err == 0) {
				controller_get_nw_services();
				publish_network_services_from_models();
			}
			break;
		case BACKEND_CMD_MODBUS_REFRESH:
			controller_get_modbus();
			publish_modbus_from_models();
			break;
		case BACKEND_CMD_MODBUS_SET_ADDR: {
			models_modbus_t modbus = {
				.addr = cmd.value,
			};
			controller_put_modbus(&modbus);
			controller_get_modbus();
			publish_modbus_from_models();
			break;
		}
		case BACKEND_CMD_SENSORS_REFRESH:
			controller_get_sensors();
			publish_sensors_from_models();
			break;
		case BACKEND_CMD_BLE_SCAN_START:
			err = controller_post_ble_scan_start() ? 0 : 1;
			break;
		case BACKEND_CMD_BLE_SCAN_STOP:
			controller_post_ble_scan_stop();
			break;
		case BACKEND_CMD_BLE_DISCOVERED_REFRESH:
			controller_get_ble_discovered();
			publish_discovered_from_models();
			break;
		case BACKEND_CMD_BLE_CONFIRM_MAC:
			controller_post_ble_confirm_mac(cmd.text);
			controller_get_sensors();
			publish_sensors_from_models();
			break;
		case BACKEND_CMD_BLE_CONFIRM_ALL:
			controller_post_ble_confirm_all();
			controller_get_sensors();
			publish_sensors_from_models();
			break;
		case BACKEND_CMD_SYSTEM_INFO_REFRESH:
			controller_get_sys_info();
			publish_system_info_from_models();
			break;
		case BACKEND_CMD_PDU_INFO_REFRESH:
			controller_get_pdu_info();
			publish_pdu_info_from_models();
			break;
		case BACKEND_CMD_PDU_SET_RATED_CURRENT: {
			const models_pdu_info_t* current = models_get_pdu_info();
			models_pdu_info_t pdu_info = current != NULL ?
					*current : (models_pdu_info_t){0};
			pdu_info.rated_current = cmd.value;
			controller_put_pdu_info(&pdu_info);
			publish_pdu_info_from_models();
			break;
		}
		case BACKEND_CMD_VISUAL_CONFIG_REFRESH:
			publish_visual_config_from_config();
			break;
		case BACKEND_CMD_VISUAL_SET_INACTIVITY:
			config_set_inactivity_time(cmd.value);
			publish_visual_config_from_config();
			break;
		case BACKEND_CMD_VISUAL_SET_PDU_FIELD:
			set_visual_pdu_field(cmd.value, cmd.text);
			publish_visual_config_from_config();
			break;
		case BACKEND_CMD_VISUAL_SAVE_ROTATION_RESTART:
			config_set_rotation(cmd.value);
			publish_visual_config_from_config();
			reset_program();
			break;
		case BACKEND_CMD_LOGIN_CONFIG_REFRESH:
			publish_login_config_from_config();
			break;
		case BACKEND_CMD_LOGIN_SET_SKIP:
			config_set_skip_login(cmd.status ? 1 : 0);
			publish_login_config_from_config();
			break;
		default:
			err = 1;
			break;
		}

		backend_push_result(err, cmd.callback, cmd.userdata);
	}

	return NULL;
}

int backend_init(void)
{
	controller_init();
	publish_visual_config_from_config();
	publish_login_config_from_config();

	pthread_mutex_lock(&mutex);
	queue_head = 0;
	queue_tail = 0;
	queue_count = 0;
	result_count = 0;
	shutdown_requested = false;
	pthread_mutex_unlock(&mutex);

	if (pthread_create(&worker, NULL, backend_worker, NULL) != 0) {
		return -1;
	}

	worker_started = true;
	return 0;
}

void backend_cleanup(void)
{
	pthread_mutex_lock(&mutex);
	shutdown_requested = true;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);

	if (worker_started) {
		pthread_join(worker, NULL);
		worker_started = false;
	}
}

void backend_process(void)
{
	backend_result_t pending[BACKEND_QUEUE_SIZE];
	int pending_count;

	pthread_mutex_lock(&mutex);
	pending_count = result_count;
	memcpy(pending, results, sizeof(backend_result_t) * pending_count);
	memset(results, 0, sizeof(results));
	result_count = 0;
	pthread_mutex_unlock(&mutex);

	for (int i = 0; i < pending_count; i++) {
		if (pending[i].callback != NULL) {
			pending[i].callback(pending[i].err, pending[i].userdata);
		}
	}
}

int backend_outlets_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_OUTLETS_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_outlet_set(int line_id, bool status, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_OUTLET_SET,
		.line_id = line_id,
		.status = status,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_outlets_set_all(bool status, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_OUTLETS_SET_ALL,
		.status = status,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_outlet_data_refresh(int outlet_id, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_OUTLET_DATA_REFRESH,
		.line_id = outlet_id,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_license_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_LICENSE_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_power_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_POWER_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_sensor_data_refresh(int sensor_index, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_SENSOR_DATA_REFRESH,
		.line_id = sensor_index,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_update_status_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_UPDATE_STATUS_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_update_confirm(bool confirm, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_UPDATE_CONFIRM,
		.status = confirm,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_update_set_auto(bool enabled, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_UPDATE_SET_AUTO,
		.status = enabled,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_update_set_interval(int hours, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_UPDATE_SET_INTERVAL,
		.value = hours,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_update_set_server(const char* server, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_UPDATE_SET_SERVER,
		.callback = callback,
		.userdata = userdata,
	};
	snprintf(cmd.text, sizeof(cmd.text), "%s", server != NULL ? server : "");
	return backend_submit(&cmd);
}

int backend_system_reboot(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_SYSTEM_REBOOT,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_system_factory_reset(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_SYSTEM_FACTORY_RESET,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_usb_update_detect(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_USB_UPDATE_DETECT,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_usb_update_start(const char* update_dev, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_USB_UPDATE_START,
		.callback = callback,
		.userdata = userdata,
	};
	snprintf(cmd.text, sizeof(cmd.text), "%s",
			update_dev != NULL ? update_dev : "");
	return backend_submit(&cmd);
}

int backend_usb_update_poll(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_USB_UPDATE_POLL,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_bluetooth_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_BLUETOOTH_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_bluetooth_set(bool powered, bool pairable, bool discoverable,
		backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_BLUETOOTH_SET,
		.status = powered,
		.pairable = pairable,
		.discoverable = discoverable,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_bluetooth_scan(bool enable, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_BLUETOOTH_SCAN,
		.status = enable,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_bluetooth_device_action(const char* mac, const char* action,
		backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_BLUETOOTH_DEVICE_ACTION,
		.callback = callback,
		.userdata = userdata,
	};
	snprintf(cmd.text, sizeof(cmd.text), "%s", mac != NULL ? mac : "");
	snprintf(cmd.action, sizeof(cmd.action), "%s",
			action != NULL ? action : "");
	return backend_submit(&cmd);
}

int backend_bluetooth_pairing_response(bool accept, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_BLUETOOTH_PAIRING_RESPONSE,
		.status = accept,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_network_if_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_NETWORK_IF_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_network_if_save(const app_state_nw_if_t* nw_if,
		backend_callback_t callback, void* userdata)
{
	if (nw_if == NULL) {
		return -1;
	}
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_NETWORK_IF_SAVE,
		.nw_if = *nw_if,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_network_info_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_NETWORK_INFO_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_network_services_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_NETWORK_SERVICES_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_network_service_set(const char* service, bool enable,
		backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_NETWORK_SERVICE_SET,
		.status = enable,
		.callback = callback,
		.userdata = userdata,
	};
	snprintf(cmd.text, sizeof(cmd.text), "%s", service != NULL ? service : "");
	return backend_submit(&cmd);
}

int backend_modbus_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_MODBUS_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_modbus_set_addr(int addr, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_MODBUS_SET_ADDR,
		.value = addr,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_sensors_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_SENSORS_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_ble_scan_start(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_BLE_SCAN_START,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_ble_scan_stop(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_BLE_SCAN_STOP,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_ble_discovered_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_BLE_DISCOVERED_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_ble_confirm_mac(const char* mac, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_BLE_CONFIRM_MAC,
		.callback = callback,
		.userdata = userdata,
	};
	snprintf(cmd.text, sizeof(cmd.text), "%s", mac != NULL ? mac : "");
	return backend_submit(&cmd);
}

int backend_ble_confirm_all(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_BLE_CONFIRM_ALL,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_system_info_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_SYSTEM_INFO_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_pdu_info_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_PDU_INFO_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_pdu_set_rated_current(int rated_current,
		backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_PDU_SET_RATED_CURRENT,
		.value = rated_current,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_visual_config_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_VISUAL_CONFIG_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_visual_set_inactivity(int minutes, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_VISUAL_SET_INACTIVITY,
		.value = minutes,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_visual_set_pdu_field(int field_id, const char* value,
		backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_VISUAL_SET_PDU_FIELD,
		.value = field_id,
		.callback = callback,
		.userdata = userdata,
	};
	snprintf(cmd.text, sizeof(cmd.text), "%s", value != NULL ? value : "");
	return backend_submit(&cmd);
}

int backend_visual_save_rotation_and_restart(int rotation,
		backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_VISUAL_SAVE_ROTATION_RESTART,
		.value = rotation,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_login_config_refresh(backend_callback_t callback, void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_LOGIN_CONFIG_REFRESH,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

int backend_login_set_skip(bool skip_login, backend_callback_t callback,
		void* userdata)
{
	backend_cmd_t cmd = {
		.type = BACKEND_CMD_LOGIN_SET_SKIP,
		.status = skip_login,
		.callback = callback,
		.userdata = userdata,
	};
	return backend_submit(&cmd);
}

#endif /* SIMULATOR_ENABLED */
