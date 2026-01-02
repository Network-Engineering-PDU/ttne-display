#include <stdlib.h>
#include <lvgl/lvgl.h>
#include <string.h>
#include "models.h"

/* Global variables ***********************************************************/

static models_info_t info;
static models_pdu_info_t pdu_info;
static models_in_sw_t in_sw;
static models_out_sw_t* out_sw_list;
static int out_sw_list_len;
static models_in_data_t in_data;
static models_out_data_t out_data;
static models_sensor_t* sensor_list;
static int sensor_list_len;
static models_nw_services_t nw_services;
static models_nw_info_t nw_info;
static models_nw_if_t nw_if;
static models_license_t license;
static models_modbus_t modbus;

/* Function prototypes ********************************************************/

static const char* stralloc(const char* str);

/* Callbacks ******************************************************************/
/* Function definitions *******************************************************/

static const char* stralloc(const char* str)
{
	char* dest;
	if (str == NULL) {
		str = "N/A";
	}
	dest = malloc(strlen(str) + 1);
	strcpy(dest, str);
	return dest;
}

/* Public functions ***********************************************************/

const models_info_t* models_get_info()
{
	return &info;
}

void models_set_info(const models_info_t* l_info)
{
	free((void*)info.product_name);
	free((void*)info.product_pn);
	free((void*)info.product_sn);
	free((void*)info.lan_mac);
	free((void*)info.ip);
	free((void*)info.sw_version);
	free((void*)info.om_version);
	free((void*)info.pmb_version);
	free((void*)info.uptime);

	info.product_name = stralloc(l_info->product_name);
	info.product_pn = stralloc(l_info->product_pn);
	info.product_sn = stralloc(l_info->product_sn);
	info.lan_mac = stralloc(l_info->lan_mac);
	info.ip = stralloc(l_info->ip);
	info.sw_version = stralloc(l_info->sw_version);
	info.om_version = stralloc(l_info->om_version);
	info.pmb_version = stralloc(l_info->pmb_version);
	info.uptime = stralloc(l_info->uptime);
}

const models_pdu_info_t* models_get_pdu_info()
{
	return &pdu_info;
}

void models_set_pdu_info(const models_pdu_info_t* l_pdu_info)
{
	free((void*)pdu_info.controller);
	free((void*)pdu_info.type);

	pdu_info.n_outlets = l_pdu_info->n_outlets;
	pdu_info.rated_current = l_pdu_info->rated_current;
	pdu_info.controller = stralloc(l_pdu_info->controller);
	pdu_info.type = stralloc(l_pdu_info->type);
}

const models_in_sw_t* models_get_in_sw()
{
	return &in_sw;
}

void models_set_in_sw(const models_in_sw_t* l_in_sw)
{
	in_sw.branch = l_in_sw->branch;
	in_sw.sys_type = l_in_sw->sys_type;
	in_sw.curr_type = l_in_sw->curr_type;
}

const models_in_data_t* models_get_in_data()
{
	return &in_data;
}

void models_set_in_data(const models_in_data_t* l_in_data)
{
	in_data.voltage = l_in_data->voltage;
	in_data.current = l_in_data->current;
	in_data.active_power = l_in_data->active_power;
	in_data.reactive_power = l_in_data->reactive_power;
	in_data.apparent_power = l_in_data->apparent_power;
	in_data.power_factor = l_in_data->power_factor;
	in_data.phase = l_in_data->phase;
	in_data.frequency = l_in_data->frequency;
	in_data.energy = l_in_data->energy;
}

const models_out_sw_t* models_get_out_sw(int* len)
{
	*len = out_sw_list_len;

	return out_sw_list;
}

const models_out_sw_t* models_get_out_sw_id(int id)
{
	return &out_sw_list[id-1];
}

//TODO: 1 o todos?
void models_set_out_sw(const models_out_sw_t* l_out_sw, int len)
{
	free((void*)out_sw_list);
	out_sw_list = malloc(len * sizeof(models_out_sw_t));
	for (int i = 0; i < len; i++) {
		out_sw_list[i].line_id = l_out_sw[i].line_id;
		out_sw_list[i].status = l_out_sw[i].status;
	}
	out_sw_list_len = len;
}

void models_set_out_sw_idx(const models_out_sw_t* l_out_sw, int line_id)
{
	if (line_id >= out_sw_list_len) {
		LV_LOG_ERROR("line_id (%d) greater than outlets array length (%d)",
				line_id, out_sw_list_len);
		return;
	}
	out_sw_list[line_id].line_id = l_out_sw->line_id;
	out_sw_list[line_id].status = l_out_sw->status;
}

const models_out_data_t* models_get_out_data()
{
	return &out_data;
}

void models_set_out_data(const models_out_data_t* l_out_data)
{
	free((void*)out_data.conn);
	out_data.voltage = l_out_data->voltage;
	out_data.current = l_out_data->current;
	out_data.active_power = l_out_data->active_power;
	out_data.reactive_power = l_out_data->reactive_power;
	out_data.apparent_power = l_out_data->apparent_power;
	out_data.power_factor = l_out_data->power_factor;
	out_data.phase = l_out_data->phase;
	out_data.frequency = l_out_data->frequency;
	out_data.energy = l_out_data->energy;
	out_data.conn = stralloc(l_out_data->conn);
	out_data.fuse = l_out_data->fuse;
}

const models_sensor_t* models_get_sensor(int* len)
{
	*len = sensor_list_len;

	return sensor_list;
}

const models_sensor_t* models_get_sensor_id(int id)
{
	return &sensor_list[id-1];
}

void models_set_sensor(const models_sensor_t* l_sensor, int len)
{
	for (int i = 0; i < sensor_list_len; i++) {
		free((void*)sensor_list[i].mac);
		free((void*)sensor_list[i].name);
		free((void*)sensor_list[i].last_data.datetime);
	}
	free((void*)sensor_list);
	sensor_list = malloc(len * sizeof(models_sensor_t));
	for (int i = 0; i < len; i++) {
		sensor_list[i].id = l_sensor[i].id;
		sensor_list[i].mac = stralloc(l_sensor[i].mac);
		sensor_list[i].name = stralloc(l_sensor[i].name);
		sensor_list[i].last_data.datetime =
				stralloc(l_sensor[i].last_data.datetime);
		sensor_list[i].last_data.temp = l_sensor[i].last_data.temp;
		sensor_list[i].last_data.humd = l_sensor[i].last_data.humd;
		sensor_list[i].last_data.pres = l_sensor[i].last_data.pres;
		sensor_list[i].last_data.rssi = l_sensor[i].last_data.rssi;
		sensor_list[i].last_data.bat = l_sensor[i].last_data.bat;
	}
	sensor_list_len = len;
}

const models_nw_services_t* models_get_nw_services()
{
	return &nw_services;
}

void models_set_nw_services(const models_nw_services_t* l_nw_services)
{
	nw_services.ssh = l_nw_services->ssh;
	nw_services.snmp = l_nw_services->snmp;
	nw_services.modbus = l_nw_services->modbus;
}

const models_nw_info_t* models_get_nw_info()
{
	return &nw_info;
}

const models_nw_if_t* models_get_nw_if()
{
	return &nw_if;
}

void models_set_nw_info(const models_nw_info_t* l_nw_info)
{
	nw_info.connected = l_nw_info->connected;
}

void models_set_nw_if(const models_nw_if_t* l_nw_if)
{
	free((void*)nw_if.params.ip);
	free((void*)nw_if.params.mask);
	free((void*)nw_if.params.gw);
	free((void*)nw_if.params.dns);
	free((void*)nw_if.params.ssid);
	free((void*)nw_if.params.pass);

	nw_if.type = l_nw_if->type;
	nw_if.dhcp = l_nw_if->dhcp;
	nw_if.params.ip = stralloc(l_nw_if->params.ip);
	nw_if.params.mask = stralloc(l_nw_if->params.mask);
	nw_if.params.gw = stralloc(l_nw_if->params.gw);
	nw_if.params.dns = stralloc(l_nw_if->params.dns);
	nw_if.params.ssid = stralloc(l_nw_if->params.ssid);
	nw_if.params.pass = stralloc(l_nw_if->params.pass);
}

const models_license_t* models_get_license()
{
	return &license;
}

void models_set_license(const models_license_t* l_license)
{
	free((void*)license.type_id);
	license.type_id = stralloc(l_license->type_id);
}

const models_modbus_t* models_get_modbus()
{
	return &modbus;
}

void models_set_modbus(const models_modbus_t* l_modbus)
{
	modbus.addr = l_modbus->addr;
}