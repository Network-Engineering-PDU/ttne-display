#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "lvgl/lvgl.h"

/* Global variables ***********************************************************/

typedef struct config_t {
	int rotation;
	int inactivity_time;
	int skip_login;
	char pdu_company[256];
	char pdu_rack[256];
	char pdu_system[256];
	char pdu_ups[256];
	char pdu_elec_board[256];
	char pdu_breaker[256];
	char pdu_service[256];
} config_t;

static config_t config;
static struct timespec file_mtime;
static bool file_mtime_valid;

#ifdef SIMULATOR_ENABLED
static char config_file[] = "/home/guille/.cmdisplay.config";
#else
static char config_file[] = "/home/root/.cmdisplay.config";
#endif

/* Function prototypes ********************************************************/

static void update_config_file();
static void remember_file_mtime();

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
	fprintf(file, "skip_login=%d\n", config.skip_login);
	fprintf(file, "pdu_company=%s\n", config.pdu_company);
	fprintf(file, "pdu_rack=%s\n", config.pdu_rack);
	fprintf(file, "pdu_system=%s\n", config.pdu_system);
	fprintf(file, "pdu_ups=%s\n", config.pdu_ups);
	fprintf(file, "pdu_elec_board=%s\n", config.pdu_elec_board);
	fprintf(file, "pdu_breaker=%s\n", config.pdu_breaker);
	fprintf(file, "pdu_service=%s\n", config.pdu_service);
	fclose(file);
	remember_file_mtime();
}

/* Public functions ***********************************************************/

static void set_defaults()
{
	config.rotation = 2; // 180 degrees (default vertical orientation)
	config.inactivity_time = 5; // 5 min
	config.skip_login = 0;
	memset(config.pdu_company, 0, sizeof(config.pdu_company));
	memset(config.pdu_rack, 0, sizeof(config.pdu_rack));
	memset(config.pdu_system, 0, sizeof(config.pdu_system));
	memset(config.pdu_ups, 0, sizeof(config.pdu_ups));
	memset(config.pdu_elec_board, 0, sizeof(config.pdu_elec_board));
	memset(config.pdu_breaker, 0, sizeof(config.pdu_breaker));
	memset(config.pdu_service, 0, sizeof(config.pdu_service));
}

/* Copies the value of "key=value" into dst if the line starts with the key.
 * Keys are matched at the start of the line only, so a value that happens to
 * contain another key's name cannot be mistaken for that key. */
static bool parse_text(const char* line, const char* key, char* dst,
		size_t size)
{
	size_t key_len = strlen(key);

	if (strncmp(line, key, key_len) != 0 || line[key_len] != '=') {
		return false;
	}
	const char* value = line + key_len + 1;
	size_t len = strcspn(value, "\n\r");
	if (len > size - 1) {
		len = size - 1;
	}
	memcpy(dst, value, len);
	dst[len] = '\0';
	return true;
}

static bool parse_int(const char* line, const char* key, int* dst)
{
	size_t key_len = strlen(key);

	if (strncmp(line, key, key_len) != 0 || line[key_len] != '=') {
		return false;
	}
	*dst = atoi(line + key_len + 1);
	return true;
}

static bool load_config_file()
{
	FILE* file = fopen(config_file, "r");
	if (file == NULL) {
		return false;
	}

	char line[300];
	while (fgets(line, sizeof(line), file) != NULL) {
		(void)(parse_int(line, "rotation", &config.rotation) ||
		parse_int(line, "inactivity_time", &config.inactivity_time) ||
		parse_int(line, "skip_login", &config.skip_login) ||
		parse_text(line, "pdu_company", config.pdu_company,
				sizeof(config.pdu_company)) ||
		parse_text(line, "pdu_rack", config.pdu_rack,
				sizeof(config.pdu_rack)) ||
		parse_text(line, "pdu_system", config.pdu_system,
				sizeof(config.pdu_system)) ||
		parse_text(line, "pdu_ups", config.pdu_ups,
				sizeof(config.pdu_ups)) ||
		parse_text(line, "pdu_elec_board", config.pdu_elec_board,
				sizeof(config.pdu_elec_board)) ||
		parse_text(line, "pdu_breaker", config.pdu_breaker,
				sizeof(config.pdu_breaker)) ||
		parse_text(line, "pdu_service", config.pdu_service,
				sizeof(config.pdu_service)));
	}
	fclose(file);
	return true;
}

static void remember_file_mtime()
{
	struct stat st;

	if (stat(config_file, &st) == 0) {
		file_mtime = st.st_mtim;
		file_mtime_valid = true;
	}
}

void config_init()
{
	set_defaults();
	if (!load_config_file()) {
		LV_LOG_ERROR("Error opening file for reading.");
		return;
	}
	remember_file_mtime();
}

int config_reload_if_changed(int* rotation_changed)
{
	struct stat st;
	config_t old = config;

	if (rotation_changed != NULL) {
		*rotation_changed = 0;
	}
	if (stat(config_file, &st) != 0) {
		return 0;
	}
	if (file_mtime_valid && st.st_mtim.tv_sec == file_mtime.tv_sec &&
			st.st_mtim.tv_nsec == file_mtime.tv_nsec) {
		return 0;
	}

	set_defaults();
	if (!load_config_file()) {
		config = old;
		return 0;
	}
	remember_file_mtime();
	if (rotation_changed != NULL) {
		*rotation_changed = config.rotation != old.rotation;
	}
	return memcmp(&old, &config, sizeof(config)) != 0;
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

/* PDU configuration getters and setters *************************************/

void config_set_pdu_company(const char* value)
{
	if (value != NULL) {
		strncpy(config.pdu_company, value, sizeof(config.pdu_company) - 1);
		config.pdu_company[sizeof(config.pdu_company) - 1] = '\0';
		update_config_file();
	}
}

const char* config_get_pdu_company()
{
	return config.pdu_company;
}

void config_set_pdu_rack(const char* value)
{
	if (value != NULL) {
		strncpy(config.pdu_rack, value, sizeof(config.pdu_rack) - 1);
		config.pdu_rack[sizeof(config.pdu_rack) - 1] = '\0';
		update_config_file();
	}
}

const char* config_get_pdu_rack()
{
	return config.pdu_rack;
}

void config_set_pdu_system(const char* value)
{
	if (value != NULL) {
		strncpy(config.pdu_system, value, sizeof(config.pdu_system) - 1);
		config.pdu_system[sizeof(config.pdu_system) - 1] = '\0';
		update_config_file();
	}
}

const char* config_get_pdu_system()
{
	return config.pdu_system;
}

void config_set_pdu_ups(const char* value)
{
	if (value != NULL) {
		strncpy(config.pdu_ups, value, sizeof(config.pdu_ups) - 1);
		config.pdu_ups[sizeof(config.pdu_ups) - 1] = '\0';
		update_config_file();
	}
}

const char* config_get_pdu_ups()
{
	return config.pdu_ups;
}

void config_set_pdu_elec_board(const char* value)
{
	if (value != NULL) {
		strncpy(config.pdu_elec_board, value, sizeof(config.pdu_elec_board) - 1);
		config.pdu_elec_board[sizeof(config.pdu_elec_board) - 1] = '\0';
		update_config_file();
	}
}

const char* config_get_pdu_elec_board()
{
	return config.pdu_elec_board;
}

void config_set_pdu_breaker(const char* value)
{
	if (value != NULL) {
		strncpy(config.pdu_breaker, value, sizeof(config.pdu_breaker) - 1);
		config.pdu_breaker[sizeof(config.pdu_breaker) - 1] = '\0';
		update_config_file();
	}
}

const char* config_get_pdu_breaker()
{
	return config.pdu_breaker;
}

void config_set_pdu_service(const char* value)
{
	if (value != NULL) {
		strncpy(config.pdu_service, value, sizeof(config.pdu_service) - 1);
		config.pdu_service[sizeof(config.pdu_service) - 1] = '\0';
		update_config_file();
	}
}

const char* config_get_pdu_service()
{
	return config.pdu_service;
}

void config_set_skip_login(int skip_login)
{
	config.skip_login = skip_login;
	update_config_file();
}

int config_get_skip_login()
{
	return config.skip_login;
}