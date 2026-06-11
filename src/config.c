#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "lvgl/lvgl.h"

/* Global variables ***********************************************************/

typedef struct config_t {
	int rotation;
	int inactivity_time;
	int skip_login;
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
	fprintf(file, "skip_login=%d\n", config.skip_login);
	fclose(file);
}

/* Public functions ***********************************************************/

void config_init()
{
	config.rotation = 3; // 270 degrees
	config.inactivity_time = 5; // 5 min
	config.skip_login = 0;

	FILE* file = fopen(config_file, "r");
	if (file == NULL) {
		LV_LOG_ERROR("Error opening file for reading.");
		return;
	}
	char line[100];
	char* key_rotation = "rotation";
	char* key_inactivity_time = "inactivity_time";
	char* key_skip_login = "skip_login";
	while (fgets(line, 100, file) != NULL) {
		if (strstr(line, key_rotation) != NULL) {
			// TODO: change atoi
			config.rotation = atoi(line + strlen(key_rotation) + 1);
		}
		if (strstr(line, key_inactivity_time) != NULL) {
			config.inactivity_time =
					atoi(line + strlen(key_inactivity_time) + 1);
		}
		if (strstr(line, key_skip_login) != NULL) {
			config.skip_login = atoi(line + strlen(key_skip_login) + 1);
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

void config_set_skip_login(int skip_login)
{
	config.skip_login = skip_login ? 1 : 0;
	update_config_file();
}

int config_get_skip_login()
{
	return config.skip_login;
}
