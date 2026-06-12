#include <lvgl/lvgl.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "controller.h"
#include "models.h"

#include "http_helper.h"
#include "json_helper.h"
#include "http_async.h"

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
	nw_if.eth_interface = "";
	nw_if.params.ip = "";
	nw_if.params.mask = "";
	nw_if.params.gw = "";
	nw_if.params.dns = "";
	nw_if.params.ssid = "";
	nw_if.params.pass = "";
	nw_if.lan1_ip = "";
	nw_if.lan2_ip = "";
	nw_if.wifi_ip = "";
	nw_if.nw_mode = -1;  /* -1 = not set, will auto-detect on load */
	models_set_nw_if(&nw_if);

	models_bt_status_t bt_status;
	bt_status.controller_mac = "";
	bt_status.name = "";
	bt_status.powered = false;
	bt_status.pairable = false;
	bt_status.discoverable = false;
	bt_status.discovering = false;
	bt_status.pairing_request = false;
	bt_status.pairing_mac = "";
	bt_status.pairing_name = "";
	bt_status.pairing_passkey = "";
	bt_status.device_count = 0;
	models_set_bt_status(&bt_status);
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

void controller_put_pdu_info(const models_pdu_info_t* pdu_info)
{
	http_get_req_t req;
	char* url = BASE_URL "settings/pdu-info/";
	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "rated_current", pdu_info->rated_current);
	char* put_data = cJSON_PrintUnformatted(json);
	printf("[controller] Sending PDU info PUT: %s\n", put_data);
	int err = http_helper_put(&req, url, put_data);
	if (err != 0) {
		LV_LOG_ERROR("PDU info PUT error: %d", err);
		printf("[controller] PDU info PUT failed with error: %d\n", err);
	} else {
		printf("[controller] PDU info PUT succeeded\n");
	}
	cJSON_Delete(json);
	free(put_data);
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

bool controller_get_sensor_live(const char* mac)
{
	http_get_req_t req;
	char url[256];
	char mac_query[32];
	int err;
	size_t j = 0;
	size_t i;

	if (mac == NULL || mac[0] == '\0') {
		return false;
	}
	for (i = 0; mac[i] != '\0' && j + 1 < sizeof(mac_query); i++) {
		if (mac[i] != ':' && mac[i] != '-') {
			mac_query[j++] = mac[i];
		}
	}
	mac_query[j] = '\0';
	snprintf(url, sizeof(url), NE_BASE_URL "api/sensors-scan/live/?mac=%s", mac_query);
	err = http_helper_get(&req, url);
	if (err != 0 || req.buffer == NULL) {
		http_helper_free(&req);
		return false;
	}
	err = json_helper_update_sensor_live(req.buffer);
	http_helper_free(&req);
	return err == 0;
}

bool controller_post_ble_scan_start()
{
	http_get_req_t req;
	char* url = NE_BASE_URL "api/sensors-scan/start/";
	int err = http_helper_post(&req, url, NULL);
	bool ok = false;

	if (err == 0 && req.buffer != NULL) {
		cJSON* json = cJSON_Parse(req.buffer);
		if (json != NULL) {
			cJSON* result = cJSON_GetObjectItemCaseSensitive(json, "result");
			cJSON* ok_field = cJSON_GetObjectItemCaseSensitive(json, "ok");
			if ((result != NULL && cJSON_IsString(result) &&
					strcmp(result->valuestring, "OK") == 0) ||
					(ok_field != NULL && cJSON_IsTrue(ok_field))) {
				ok = true;
			}
			cJSON_Delete(json);
		}
	}
	if (!ok) {
		LV_LOG_ERROR("BLE scan start error");
	}
	http_helper_free(&req);
	return ok;
}

void controller_post_ble_scan_stop()
{
	http_get_req_t req;
	char* url = NE_BASE_URL "api/sensors-scan/stop/";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("BLE scan stop error");
	}
	http_helper_free(&req);
}

void controller_get_ble_discovered()
{
	http_get_req_t req;
	char* url = NE_BASE_URL "api/sensors-scan/discovered/";
	int err = http_helper_get(&req, url);
	if (err == 0 && req.buffer != NULL) {
		json_helper_update_discovered(req.buffer);
	}
	http_helper_free(&req);
}

void controller_post_ble_confirm_mac(const char* mac)
{
	http_get_req_t req;
	char* url = NE_BASE_URL "api/sensors-scan/confirm/";
	cJSON* json = cJSON_CreateObject();
	cJSON* macs = cJSON_CreateArray();
	cJSON_AddItemToArray(macs, cJSON_CreateString(mac));
	cJSON_AddItemToObject(json, "macs", macs);
	char* post_data = cJSON_PrintUnformatted(json);
	int err = http_helper_post(&req, url, post_data);
	if (err != 0) {
		LV_LOG_ERROR("BLE confirm error");
	}
	cJSON_free(post_data);
	cJSON_Delete(json);
	http_helper_free(&req);
}

void controller_post_ble_confirm_all()
{
	http_get_req_t req;
	char* url = NE_BASE_URL "api/sensors-scan/confirm/";
	cJSON* json = cJSON_CreateObject();
	cJSON_AddTrueToObject(json, "all");
	char* post_data = cJSON_PrintUnformatted(json);
	int err = http_helper_post(&req, url, post_data);
	if (err != 0) {
		LV_LOG_ERROR("BLE confirm all error");
	}
	cJSON_free(post_data);
	cJSON_Delete(json);
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
	cJSON_AddNumberToObject(json, "nw_mode", nw_if->nw_mode);  /* Store persistent mode */
	cJSON_AddStringToObject(json, "eth_interface",
			nw_if->eth_interface != NULL ? nw_if->eth_interface : "");
	
	/* Multi-interface support */
	if (nw_if->lan1_ip != NULL && strlen(nw_if->lan1_ip) > 0) {
		cJSON_AddStringToObject(json, "lan1_ip", nw_if->lan1_ip);
	}
	if (nw_if->lan2_ip != NULL && strlen(nw_if->lan2_ip) > 0) {
		cJSON_AddStringToObject(json, "lan2_ip", nw_if->lan2_ip);
	}
	if (nw_if->wifi_ip != NULL && strlen(nw_if->wifi_ip) > 0) {
		cJSON_AddStringToObject(json, "wifi_ip", nw_if->wifi_ip);
	}
	
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

void controller_post_start_bluetooth()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/start-bluetooth";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Bluetooth start error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_post_stop_bluetooth()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/stop-bluetooth";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Bluetooth stop error");
	}
	printf("Buffer received: %s\n", req.buffer);
	http_helper_free(&req);
}

void controller_get_bluetooth()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/bluetooth/";
	int err = http_helper_get(&req, url);
	if (err != 0) {
		LV_LOG_ERROR("Bluetooth status GET error");
		http_helper_free(&req);
		return;
	}
	err = json_helper_update_bt_status(req.buffer);
	if (err != 0) {
		LV_LOG_ERROR("Bluetooth status parse error");
	}
	http_helper_free(&req);
}

void controller_put_bluetooth(bool powered, bool pairable, bool discoverable)
{
	http_get_req_t req;
	char* url = BASE_URL "settings/bluetooth";
	cJSON *json = cJSON_CreateObject();
	cJSON_AddBoolToObject(json, "powered", powered);
	cJSON_AddBoolToObject(json, "pairable", pairable);
	cJSON_AddBoolToObject(json, "discoverable", discoverable);
	char* put_data = cJSON_PrintUnformatted(json);
	int err = http_helper_put(&req, url, put_data);
	if (err != 0) {
		LV_LOG_ERROR("Bluetooth settings PUT error");
	}
	free(put_data);
	cJSON_Delete(json);
	http_helper_free(&req);
}

void controller_post_bluetooth_scan(bool enable)
{
	http_get_req_t req;
	char* url = enable ? BASE_URL "settings/bluetooth/scan/start" :
			BASE_URL "settings/bluetooth/scan/stop";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Bluetooth scan error");
	}
	http_helper_free(&req);
}

void controller_post_bluetooth_device_action(const char* mac, const char* action)
{
	http_get_req_t req;
	char url[160];
	sprintf(url, BASE_URL "settings/bluetooth/devices/%s/%s", mac, action);
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Bluetooth device action error");
	}
	http_helper_free(&req);
}

void controller_post_bluetooth_pairing_response(bool accept)
{
	http_get_req_t req;
	char* url = accept ? BASE_URL "settings/bluetooth/pairing/accept" :
			BASE_URL "settings/bluetooth/pairing/refuse";
	int err = http_helper_post(&req, url, NULL);
	if (err != 0) {
		LV_LOG_ERROR("Bluetooth pairing response error");
	}
	http_helper_free(&req);
}

void controller_get_update_status()
{
	http_get_req_t req;
	char* url = BASE_URL "settings/update-status";
	int err = http_helper_get(&req, url);
	if (err != 0) {
		LV_LOG_ERROR("Update status GET error: %d", err);
		http_helper_free(&req);
		return;
	}
	if (req.buffer == NULL || strlen(req.buffer) == 0) {
		LV_LOG_ERROR("Update status response empty");
		http_helper_free(&req);
		return;
	}
	int parse_err = json_helper_update_update_status(req.buffer);
	if (parse_err != 0) {
		LV_LOG_ERROR("Failed to parse update status JSON");
	}
	http_helper_free(&req);
}

void controller_post_update_confirm(bool confirm)
{
	http_get_req_t req;
	char* url = BASE_URL "settings/update-confirm";
	cJSON *json = cJSON_CreateObject();
	cJSON_AddBoolToObject(json, "confirm", confirm);
	char* post_data = cJSON_PrintUnformatted(json);
	int err = http_helper_post(&req, url, post_data);
	if (err != 0) {
		LV_LOG_ERROR("Update confirm error");
	}
	printf("Buffer received: %s\n", req.buffer);
	cJSON_Delete(json);
	http_helper_free(&req);
}

void controller_put_update_settings(bool auto_update, const char* update_server)
{
	http_get_req_t req;
	char* url = BASE_URL "settings/update-settings";
	cJSON *json = cJSON_CreateObject();
	cJSON_AddBoolToObject(json, "auto_update", auto_update);
	cJSON_AddStringToObject(json, "update_server", update_server ? update_server : "");
	char* put_data = cJSON_PrintUnformatted(json);
	int err = http_helper_put(&req, url, put_data);
	if (err != 0) {
		LV_LOG_ERROR("Update settings error");
	}
	printf("Buffer received: %s\n", req.buffer);
	cJSON_Delete(json);
	http_helper_free(&req);
}

void controller_set_update_server(const char* server)
{
	const models_update_status_t* update_status = models_get_update_status();
	controller_put_update_settings(update_status->auto_update, server);
	LV_LOG_USER("Update server set to: %s", server);
}

void controller_set_auto_update(bool enabled)
{
	const models_update_status_t* update_status = models_get_update_status();
	controller_put_update_settings(enabled, update_status->update_server);
	LV_LOG_USER("Auto update: %s", enabled ? "enabled" : "disabled");
}

/* ============================================================================
 * ASYNC HTTP IMPLEMENTATIONS - These run in background threads
 * ============================================================================ */

typedef struct {
	controller_callback_t callback;
	void* userdata;
	char* buffer;
} async_ctx_t;

static void async_sys_info_cb(int err, void* buffer, size_t len, void* userdata)
{
	async_ctx_t* ctx = (async_ctx_t*)userdata;
	
	if (err == 0 && buffer != NULL) {
		int parse_err = json_helper_update_sys_info(buffer);
		if (parse_err != 0) {
			LV_LOG_ERROR("Error parsing system info");
			err = 1;
		}
	} else {
		LV_LOG_ERROR("Error fetching system info");
	}

	if (buffer) free(buffer);
	
	if (ctx->callback) {
		ctx->callback(err, ctx->userdata);
	}
	free(ctx);
}

void controller_get_sys_info_async(controller_callback_t callback, void* userdata)
{
	async_ctx_t* ctx = malloc(sizeof(async_ctx_t));
	ctx->callback = callback;
	ctx->userdata = userdata;
	ctx->buffer = NULL;

	char* url = BASE_URL "settings/system-info/";
	http_async_get(url, async_sys_info_cb, ctx);
}

static void async_pdu_info_cb(int err, void* buffer, size_t len, void* userdata)
{
	async_ctx_t* ctx = (async_ctx_t*)userdata;
	
	if (err == 0 && buffer != NULL) {
		json_helper_update_pdu_info(buffer);
	} else {
		LV_LOG_ERROR("Error fetching PDU info");
	}

	if (buffer) free(buffer);
	
	if (ctx->callback) {
		ctx->callback(err, ctx->userdata);
	}
	free(ctx);
}

void controller_get_pdu_info_async(controller_callback_t callback, void* userdata)
{
	async_ctx_t* ctx = malloc(sizeof(async_ctx_t));
	ctx->callback = callback;
	ctx->userdata = userdata;
	ctx->buffer = NULL;

	char* url = BASE_URL "settings/pdu-info/";
	http_async_get(url, async_pdu_info_cb, ctx);
}

static void async_nw_if_cb(int err, void* buffer, size_t len, void* userdata)
{
	async_ctx_t* ctx = (async_ctx_t*)userdata;
	
	if (err == 0 && buffer != NULL) {
		json_helper_update_nw_if(buffer);
	} else {
		LV_LOG_ERROR("Error fetching network interface");
	}

	if (buffer) free(buffer);
	
	if (ctx->callback) {
		ctx->callback(err, ctx->userdata);
	}
	free(ctx);
}

void controller_get_nw_if_async(controller_callback_t callback, void* userdata)
{
	async_ctx_t* ctx = malloc(sizeof(async_ctx_t));
	ctx->callback = callback;
	ctx->userdata = userdata;
	ctx->buffer = NULL;

	char* url = BASE_URL "settings/network-info/";
	http_async_get(url, async_nw_if_cb, ctx);
}