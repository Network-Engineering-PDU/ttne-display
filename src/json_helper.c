#include <lvgl/lvgl.h>
#include <cjson/cJSON.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_helper.h"
#include "models.h"

#define MAX_OUTLETS 48
#define MAX_SENSORS 8

/* Global variables ***********************************************************/
/* Function prototypes ********************************************************/

static const char* json_get_string(cJSON* json, const char* id);
static int json_get_int(int* num, cJSON* json, char* id);
static int json_get_float(float* num, cJSON* json, char* id);
static int json_get_bool(bool* bl, cJSON* json, char* id);

/* Callbacks ******************************************************************/
/* Function definitions *******************************************************/

static const char* json_get_string(cJSON* json, const char* id)
{
	cJSON* res = cJSON_GetObjectItemCaseSensitive(json, id);
	if (cJSON_IsString(res)) {
		return res->valuestring;
	}
	return NULL;
}

static int json_get_int(int* num, cJSON* json, char* id)
{
	cJSON* res = cJSON_GetObjectItem(json, id);
	if (cJSON_IsNumber(res)) {
		*num = res->valueint;
		return 0;
	}
	return 1;
}

static int json_get_float(float* num, cJSON* json, char* id)
{
	cJSON* res = cJSON_GetObjectItem(json, id);
	if (cJSON_IsNumber(res)) {
		*num = (float)res->valuedouble;
		return 0;
	}
	return 1;
}

static int json_get_bool(bool* bl, cJSON* json, char* id)
{
	cJSON* res = cJSON_GetObjectItem(json, id);
	if (cJSON_IsBool(res)) {
		*bl = cJSON_IsTrue(res);
		return 0;
	}
	return 1;
}

/* Public functions ***********************************************************/

int json_helper_update_sys_info(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}

	models_info_t info;
	const char* str;
	str = json_get_string(json, "product_name");
	if (str == NULL) {
		return 1;
	}
	info.product_name = str;
	str = json_get_string(json, "product_pn");
	if (str == NULL) {
		return 1;
	}
	info.product_pn = str;
	str = json_get_string(json, "product_sn");
	if (str == NULL) {
		return 1;
	}
	info.product_sn = str;
	str = json_get_string(json, "lan_mac");
	if (str == NULL) {
		return 1;
	}
	info.lan_mac = str;
	str = json_get_string(json, "ip");
	if (str == NULL) {
		return 1;
	}
	info.ip = str;
	str = json_get_string(json, "sw_version");
	if (str == NULL) {
		return 1;
	}
	info.sw_version = str;
	str = json_get_string(json, "om_version");
	if (str == NULL) {
		return 1;
	}
	info.om_version = str;
	str = json_get_string(json, "pmb_version");
	if (str == NULL) {
		return 1;
	}
	info.pmb_version = str;
	str = json_get_string(json, "uptime");
	if (str == NULL) {
		return 1;
	}
	info.uptime = str;
	models_set_info(&info);

	cJSON_Delete(json);

	return 0;
}

int json_helper_update_pdu_info(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}

	models_pdu_info_t pdu_info;
	const char* str;
	int err;
	err = json_get_int(&pdu_info.n_outlets, json, "outlet_count");
	if (err != 0) {
		return 1;
	}
	err = json_get_int(&pdu_info.rated_current, json, "rated_current");
	if (err != 0) {
		return 1;
	}
	str = json_get_string(json, "controller");
	if (str == NULL) {
		return 1;
	}
	pdu_info.controller = str;
	str = json_get_string(json, "type");
	if (str == NULL) {
		return 1;
	}
	pdu_info.type = str;
	models_set_pdu_info(&pdu_info);

	cJSON_Delete(json);

	return 0;
}

int json_helper_update_in_sw(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}

	models_in_sw_t in_sw;
	int err;
	err = json_get_int(&in_sw.branch, json, "branch");
	if (err != 0) {
		return 1;
	}
	err = json_get_int(&in_sw.sys_type, json, "sys_type");
	if (err != 0) {
		return 1;
	}
	err = json_get_int(&in_sw.curr_type, json, "curr_type");
	if (err != 0) {
		return 1;
	}
	models_set_in_sw(&in_sw);

	cJSON_Delete(json);

	return 0;
}


int json_helper_update_in_data(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}
	models_in_data_t in_data;

	int err;
	err = json_get_float(&in_data.voltage, json, "voltage");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&in_data.current, json, "current");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&in_data.active_power, json, "active_power");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&in_data.reactive_power, json, "reactive_power");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&in_data.apparent_power, json, "apparent_power");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&in_data.power_factor, json, "power_factor");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&in_data.phase, json, "phase");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&in_data.frequency , json, "frequency");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&in_data.energy, json, "energy");
	if (err != 0) {
		return 1;
	}

	models_set_in_data(&in_data);

	cJSON_Delete(json);

	return 0;
}

int json_helper_update_out_sw(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}
	models_out_sw_t out_sw[MAX_OUTLETS];

	int i;
	for (i = 0; i < MAX_OUTLETS; i++) {
		int err;
		char s[5];
		sprintf(s, "%d", i);
		err = json_get_bool(&out_sw[i].status, json, s);
		if (err != 0) {
			break;
		}
		out_sw[i].line_id = i + 1;
	}

	models_set_out_sw(out_sw, i);

	cJSON_Delete(json);

	return 0;
}

int json_helper_update_out_data(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}

	models_out_data_t out_data;
	int err;
	const char* str;
	err = json_get_float(&out_data.voltage, json, "voltage");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&out_data.current, json, "current");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&out_data.active_power, json, "active_power");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&out_data.reactive_power, json, "reactive_power");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&out_data.apparent_power , json, "apparent_power");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&out_data.power_factor, json, "power_factor");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&out_data.phase, json, "phase");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&out_data.frequency, json, "frequency");
	if (err != 0) {
		return 1;
	}
	err = json_get_float(&out_data.energy, json, "energy");
	if (err != 0) {
		return 1;
	}
	str = json_get_string(json, "conn");
	if (str == NULL) {
		return 1;
	}
	out_data.conn = str;
	err = json_get_int(&out_data.fuse, json, "fuse");
	if (err != 0) {
		return 1;
	}
	models_set_out_data(&out_data);

	cJSON_Delete(json);

	return 0;
}

int json_helper_update_sensors(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}

	models_sensor_t sensors[MAX_SENSORS];
	cJSON* sensor;
	int i = 0;
	cJSON_ArrayForEach(sensor, json) {
		int err;
		const char* str;

		if (i == MAX_SENSORS) {
			LV_LOG_WARN("MAX SENSORS limit reached!");
			break;
		}

		err = json_get_int(&sensors[i].id, sensor, "id");
		if (err != 0) {
			return 1;
		}
		str = json_get_string(sensor, "mac_address");
		if (str == NULL) {
			return 1;
		}
		sensors[i].mac = str;
		str = json_get_string(sensor, "name");
		sensors[i].name = str;

		cJSON* last_data = cJSON_GetObjectItem(sensor, "last_data");
		str = json_get_string(last_data, "data_datetime");
		sensors[i].last_data.datetime = str;
		err = json_get_float(&sensors[i].last_data.temp, last_data, "temperature");
		if (err != 0) {
			sensors[i].last_data.temp = NAN;
		}
		err = json_get_float(&sensors[i].last_data.humd, last_data, "humidity");
		if (err != 0) {
			sensors[i].last_data.humd = NAN;
		}
		err = json_get_float(&sensors[i].last_data.pres, last_data, "pressure");
		if (err != 0) {
			sensors[i].last_data.pres = NAN;
		}
		err = json_get_int(&sensors[i].last_data.rssi, last_data, "rssi");
		if (err != 0) {
			sensors[i].last_data.rssi = 1;
		}
		err = json_get_int(&sensors[i].last_data.bat, last_data, "battery");
		if (err != 0) {
			sensors[i].last_data.bat = -1;
		}
		i++;
	}

	models_set_sensor(sensors, i);
	cJSON_Delete(json);

	return 0;
}

int json_helper_update_nw_services(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}

	models_nw_services_t nw_services;
	bool bl;
	int err;
	err = json_get_bool(&bl, json, "snmp");
	if (err != 0) {
		return 1;
	}
	nw_services.snmp = bl;
	err = json_get_bool(&bl, json, "modbus");
	if (err != 0) {
		return 1;
	}
	nw_services.modbus = bl;
	err = json_get_bool(&bl, json, "ssh");
	if (err != 0) {
		return 1;
	}
	nw_services.ssh = bl;
	
	models_set_nw_services(&nw_services);

	cJSON_Delete(json);

	return 0;
}

int json_helper_update_nw_info(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}

	models_nw_info_t nw_info;
	bool bl;
	int err;
	err = json_get_bool(&bl, json, "connected");
	if (err != 0) {
		return 1;
	}
	nw_info.connected = bl;
	
	models_set_nw_info(&nw_info);

	cJSON_Delete(json);

	return 0;
}

int json_helper_update_nw_if(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}

	models_nw_if_t nw_if;
	const char* str;
	bool bl;
	int err;

	err = json_get_int((int*)(&nw_if.type), json, "type");
	if (err != 0) {
		return 1;
	}

	err = json_get_bool(&bl, json, "dhcp");
	if (err != 0) {
		return 1;
	}
	nw_if.dhcp = bl;

	cJSON* params = cJSON_GetObjectItem(json, "params");

	str = json_get_string(params, "ip");
	if (str == NULL) {
		return 1;
	}
	nw_if.params.ip = str;
	str = json_get_string(params, "subnet_mask");
	if (str == NULL) {
		return 1;
	}
	nw_if.params.mask = str;
	str = json_get_string(params, "gateway_ip");
	if (str == NULL) {
		return 1;
	}
	nw_if.params.gw = str;
	str = json_get_string(params, "dns");
	if (str == NULL) {
		return 1;
	}
	nw_if.params.dns = str;
	str = json_get_string(params, "ssid");
	if (str == NULL) {
		return 1;
	}
	nw_if.params.ssid = str;
	str = json_get_string(params, "password");
	if (str == NULL) {
		return 1;
	}
	nw_if.params.pass = str;
	
	models_set_nw_if(&nw_if);

	cJSON_Delete(json);

	return 0;
}

int json_helper_update_license(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}

	models_license_t license;
	const char* str;

	str = json_get_string(json, "type_id");
	if (str == NULL) {
		return 1;
	}
	license.type_id = str;
	
	models_set_license(&license);

	cJSON_Delete(json);

	return 0;
}

int json_helper_update_modbus(const char* json_str)
{
	cJSON* json = cJSON_Parse(json_str);
	if (json == NULL) {
		return 1;
	}

	models_modbus_t modbus;
	int err;
	err = json_get_int((int*)(&modbus.addr), json, "addr");
	if (err != 0) {
		return 1;
	}
	
	models_set_modbus(&modbus);

	cJSON_Delete(json);

	return 0;
}
