#include <lvgl/lvgl.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <strings.h>

#include "controller.h"
#include "models.h"

#include "http_helper.h"
#include "json_helper.h"

#define BASE_URL "http://localhost:8001/"
#define NE_BASE_URL "http://localhost:80/"

/* Global variables ***********************************************************/
/* Function prototypes ********************************************************/
/* Callbacks ******************************************************************/
/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void controller_init()
{
	models_info_t info;
	info.product_name = "N/A";
	info.product_pn = "N/A";
	info.product_sn = "N/A";
	info.lan_mac = "N/A";
	info.ip = "N/A";
	info.sw_version = "N/A";
	info.om_version = "N/A";
	info.pmb_version = "N/A";
	info.uptime = "N/A";
	models_set_info(&info);

	models_pdu_info_t pdu_info;
	pdu_info.n_outlets = 0;
	pdu_info.rated_current = 0;
	pdu_info.controller = "N/A";
	pdu_info.type = "N/A";
	models_set_pdu_info(&pdu_info);

	models_nw_if_t nw_if;
	nw_if.type = UNCONF;
	nw_if.dhcp = true;
	nw_if.params.ip = "";
	nw_if.params.mask = "";
	nw_if.params.gw = "";
	nw_if.params.dns = "";
	nw_if.params.ssid = "";
	nw_if.params.pass = "";
	models_set_nw_if(&nw_if);
}

bool controller_check_conn()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/pdu-info";
	int err = http_helper_get(&req, url);
	if (err != 0) {
		return false;
	}
	return true;
}

void controller_get_sys_info()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/system-info/";
	http_helper_get(&req, url);
	int err = json_helper_update_sys_info(req.buffer);
	if (err != 0) {
		LV_LOG_ERROR("Error updating system info");
	}
	http_helper_free(&req);
}

void controller_get_pdu_info()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/pdu-info/";
	http_helper_get(&req, url);
	json_helper_update_pdu_info(req.buffer);
	http_helper_free(&req);
}

void controller_get_in_sw()
{
	http_get_req_t req;
	char* url = BASE_URL "inputs/switches/";
	http_helper_get(&req, url);
	json_helper_update_in_sw(req.buffer);
	http_helper_free(&req);
}

void controller_get_in_data(int line_id)
{
	http_get_req_t req;
	char url[100];
	sprintf(url, BASE_URL "inputs/%d/data", line_id);
	http_helper_get(&req, url);
	json_helper_update_in_data(req.buffer);
	http_helper_free(&req);
}

void controller_get_out_sw()
{
	http_get_req_t req;
	char* url = BASE_URL "outputs/switch-status/";
	http_helper_get(&req, url);
	json_helper_update_out_sw(req.buffer);
	http_helper_free(&req);
}

void controller_put_out_sw(const models_out_sw_t* out_sw, int line_id)
{
	http_get_req_t req;
	char url[100];
	sprintf(url, BASE_URL "outputs/%d/switch-status", line_id);
	cJSON *json = cJSON_CreateObject();
	cJSON_AddBoolToObject(json, "switch_status", out_sw->status);
	char* put_data = cJSON_PrintUnformatted(json);
	int err = http_helper_put(&req, url, put_data);
	if (err == 0) {
		models_set_out_sw_idx(out_sw, line_id);
	}
	printf("Buffer received: %s\n", req.buffer);
	cJSON_Delete(json);
	http_helper_free(&req);
}

void controller_get_out_data(int line_id)
{
	http_get_req_t req;
	char url[100];
	sprintf(url, BASE_URL "outputs/%d/data", line_id-1);
	http_helper_get(&req, url);
	json_helper_update_out_data(req.buffer);
	http_helper_free(&req);
}

// API DJango
void controller_get_sensors()
{
	http_get_req_t req;
	char* url = NE_BASE_URL "api/sensors-data/";
	http_helper_get(&req, url);
	json_helper_update_sensors(req.buffer);
	http_helper_free(&req);
}

void controller_get_nw_services()
{
	http_get_req_t req;
	char* url = BASE_URL "network/services/";
	http_helper_get(&req, url);
	json_helper_update_nw_services(req.buffer);
	http_helper_free(&req);
}

void controller_get_nw_info()
{
	http_get_req_t req;
	char* url = BASE_URL "network/info/";
	http_helper_get(&req, url);
	json_helper_update_nw_info(req.buffer);
	http_helper_free(&req);
}

void controller_get_nw_if()
{
	http_get_req_t req;
	char* url = BASE_URL "network/interfaces/";
	http_helper_get(&req, url);
	json_helper_update_nw_if(req.buffer);
	http_helper_free(&req);
}

void controller_put_nw_if(const models_nw_if_t* nw_if)
{
	http_get_req_t req;
	char* url = BASE_URL "network/interfaces";
	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "type", nw_if->type);
	cJSON_AddBoolToObject(json, "dhcp", nw_if->dhcp);
	cJSON* json_params = cJSON_AddObjectToObject(json, "params");
	cJSON_AddStringToObject(json_params, "ip", nw_if->params.ip);
	cJSON_AddStringToObject(json_params, "subnet_mask", nw_if->params.mask);
	cJSON_AddStringToObject(json_params, "gateway_ip", nw_if->params.gw);
	cJSON_AddStringToObject(json_params, "dns", nw_if->params.dns);
	cJSON_AddStringToObject(json_params, "ssid", nw_if->params.ssid);
	cJSON_AddStringToObject(json_params, "password", nw_if->params.pass);
	char* put_data = cJSON_PrintUnformatted(json);
	int err = http_helper_put(&req, url, put_data);
	if (err == 0) {
		models_set_nw_if(nw_if);
	}
	printf("Buffer received: %s\n", req.buffer);
	cJSON_Delete(json);
	http_helper_free(&req);
}

void controller_post_nw_reset()
{
	http_get_req_t req;
	char* url = BASE_URL "network/reset";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Nework reset error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_post_fact_reset()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/factory-reset";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Factory reset error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_post_reboot()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/system-reboot";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Reboot error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_post_start_scan()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/start-scan";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Start scan error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_post_stop_scan()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/stop-scan";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Stop scan error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_put_license(const models_license_t* license)
{
	http_get_req_t req;
	char* url = BASE_URL "settings/license";
	cJSON *json = cJSON_CreateObject();
	cJSON_AddStringToObject(json, "type_id", license->type_id);
	char* put_data = cJSON_PrintUnformatted(json);
	int err = http_helper_put(&req, url, put_data);
	if (err != 0) {
		LV_LOG_ERROR("License error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_get_license()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/license";
	http_helper_get(&req, url);
	json_helper_update_license(req.buffer);
	http_helper_free(&req);
}

void controller_put_modbus(const models_modbus_t* modbus)
{
	http_get_req_t req;
	char* url = BASE_URL "settings/modbus";
	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "addr", modbus->addr);
	char* put_data = cJSON_PrintUnformatted(json);
	int err = http_helper_put(&req, url, put_data);
	if (err != 0) {
		LV_LOG_ERROR("Modbus error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_get_modbus()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/modbus";
	http_helper_get(&req, url);
	json_helper_update_modbus(req.buffer);
	http_helper_free(&req);
}

void controller_post_start_ssh()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/start-ssh";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("SSH start error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_post_stop_ssh()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/stop-ssh";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("SSH stop error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_post_start_snmp()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/start-snmp";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("SNMP start error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_post_stop_snmp()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/stop-snmp";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("SNMP stop error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_post_start_modbus()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/start-modbus";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Modbus start error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_post_stop_modbus()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/stop-modbus";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Modbus stop error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}