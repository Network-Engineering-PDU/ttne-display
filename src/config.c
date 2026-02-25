#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "lvgl/lvgl.h"

/* Global variables ***********************************************************/

typedef struct config_t {
	int rotation;
	int inactivity_time;
	char pdu_company[256];
	char pdu_rack[256];
	char pdu_system[256];
	char pdu_ups[256];
	char pdu_elec_board[256];
	char pdu_breaker[256];
	char pdu_service[256];
} config_t;

static config_t config;

#ifdef SIMULATOR_ENABLED
static char config_file[] = "/home/guille/.cmdisplay.config";
#else
static char config_file[] = "/home/root/.cmdisplay.config";
#endif

/* Function prototypes ********************************************************/

static void update_config_file();

/* Callbacks ******************************************************************/
/* Function definitions *******************************************************/

static void update_config_file()
{
	FILE* file = fopen(config_file, "w");
	if (file == NULL) {
		LV_LOG_ERROR("Error opening file for writing.");
		return;
	}
	fprintf(file, "rotation=%d\n", config.rotation);
	fprintf(file, "inactivity_time=%d\n", config.inactivity_time);
	fprintf(file, "pdu_company=%s\n", config.pdu_company);
	fprintf(file, "pdu_rack=%s\n", config.pdu_rack);
	fprintf(file, "pdu_system=%s\n", config.pdu_system);
	fprintf(file, "pdu_ups=%s\n", config.pdu_ups);
	fprintf(file, "pdu_elec_board=%s\n", config.pdu_elec_board);
	fprintf(file, "pdu_breaker=%s\n", config.pdu_breaker);
	fprintf(file, "pdu_service=%s\n", config.pdu_service);
	fclose(file);
}

/* Public functions ***********************************************************/

void config_init()
{
	config.rotation = 3; // 270 degrees
	config.inactivity_time = 5; // 5 min
	memset(config.pdu_company, 0, sizeof(config.pdu_company));
	memset(config.pdu_rack, 0, sizeof(config.pdu_rack));
	memset(config.pdu_system, 0, sizeof(config.pdu_system));
	memset(config.pdu_ups, 0, sizeof(config.pdu_ups));
	memset(config.pdu_elec_board, 0, sizeof(config.pdu_elec_board));
	memset(config.pdu_breaker, 0, sizeof(config.pdu_breaker));
	memset(config.pdu_service, 0, sizeof(config.pdu_service));

	FILE* file = fopen(config_file, "r");
	if (file == NULL) {
		LV_LOG_ERROR("Error opening file for reading.");
		return;
	}
	char line[256];
	char* key_rotation = "rotation";
	char* key_inactivity_time = "inactivity_time";
	char* key_pdu_company = "pdu_company";
	char* key_pdu_rack = "pdu_rack";
	char* key_pdu_system = "pdu_system";
	char* key_pdu_ups = "pdu_ups";
	char* key_pdu_elec_board = "pdu_elec_board";
	char* key_pdu_breaker = "pdu_breaker";
	char* key_pdu_service = "pdu_service";

	while (fgets(line, sizeof(line), file) != NULL) {
		if (strstr(line, key_rotation) != NULL) {
			config.rotation = atoi(line + strlen(key_rotation) + 1);
		}
		if (strstr(line, key_inactivity_time) != NULL) {
			config.inactivity_time = atoi(line + strlen(key_inactivity_time) + 1);
		}
		if (strstr(line, key_pdu_company) != NULL) {
			char* value = line + strlen(key_pdu_company) + 1;
			sscanf(value, "%255[^\n]", config.pdu_company);
		}
		if (strstr(line, key_pdu_rack) != NULL) {
			char* value = line + strlen(key_pdu_rack) + 1;
			sscanf(value, "%255[^\n]", config.pdu_rack);
		}
		if (strstr(line, key_pdu_system) != NULL) {
			char* value = line + strlen(key_pdu_system) + 1;
			sscanf(value, "%255[^\n]", config.pdu_system);
		}
		if (strstr(line, key_pdu_ups) != NULL) {
			char* value = line + strlen(key_pdu_ups) + 1;
			sscanf(value, "%255[^\n]", config.pdu_ups);
		}
		if (strstr(line, key_pdu_elec_board) != NULL) {
			char* value = line + strlen(key_pdu_elec_board) + 1;
			sscanf(value, "%255[^\n]", config.pdu_elec_board);
		}
		if (strstr(line, key_pdu_breaker) != NULL) {
			char* value = line + strlen(key_pdu_breaker) + 1;
			sscanf(value, "%255[^\n]", config.pdu_breaker);
		}
		if (strstr(line, key_pdu_service) != NULL) {
			char* value = line + strlen(key_pdu_service) + 1;
			sscanf(value, "%255[^\n]", config.pdu_service);
		}
	}
	fclose(file);
}

void config_set_rotation(int rotation)
{
	config.rotation = rotation;
	update_config_file();
}

int config_get_rotation()
{
	return config.rotation;
}

void config_set_inactivity_time(int inactivity_time)
{
	config.inactivity_time = inactivity_time;
	update_config_file();
}

int config_get_inactivity_time()
{
	return config.inactivity_time;
}

void config_set_pdu_company(const char* value)
{
	strncpy(config.pdu_company, value, sizeof(config.pdu_company) - 1);
	config.pdu_company[sizeof(config.pdu_company) - 1] = '\0';
	update_config_file();
}

const char* config_get_pdu_company()
{
	return config.pdu_company;
}

void config_set_pdu_rack(const char* value)
{
	strncpy(config.pdu_rack, value, sizeof(config.pdu_rack) - 1);
	config.pdu_rack[sizeof(config.pdu_rack) - 1] = '\0';
	update_config_file();
}

const char* config_get_pdu_rack()
{
	return config.pdu_rack;
}

void config_set_pdu_system(const char* value)
{
	strncpy(config.pdu_system, value, sizeof(config.pdu_system) - 1);
	config.pdu_system[sizeof(config.pdu_system) - 1] = '\0';
	update_config_file();
}

const char* config_get_pdu_system()
{
	return config.pdu_system;
}

void config_set_pdu_ups(const char* value)
{
	strncpy(config.pdu_ups, value, sizeof(config.pdu_ups) - 1);
	config.pdu_ups[sizeof(config.pdu_ups) - 1] = '\0';
	update_config_file();
}

const char* config_get_pdu_ups()
{
	return config.pdu_ups;
}

void config_set_pdu_elec_board(const char* value)
{
	strncpy(config.pdu_elec_board, value, sizeof(config.pdu_elec_board) - 1);
	config.pdu_elec_board[sizeof(config.pdu_elec_board) - 1] = '\0';
	update_config_file();
}

const char* config_get_pdu_elec_board()
{
	return config.pdu_elec_board;
}

void config_set_pdu_breaker(const char* value)
{
	strncpy(config.pdu_breaker, value, sizeof(config.pdu_breaker) - 1);
	config.pdu_breaker[sizeof(config.pdu_breaker) - 1] = '\0';
	update_config_file();
}

const char* config_get_pdu_breaker()
{
	return config.pdu_breaker;
}

void config_set_pdu_service(const char* value)
{
	strncpy(config.pdu_service, value, sizeof(config.pdu_service) - 1);
	config.pdu_service[sizeof(config.pdu_service) - 1] = '\0';
	update_config_file();
}

const char* config_get_pdu_service()
{
	return config.pdu_service;
}