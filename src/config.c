#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "lvgl/lvgl.h"

/* Global variables ***********************************************************/

typedef struct config_t {
	int rotation;
	int inactivity_time;
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
	fclose(file);
}

/* Public functions ***********************************************************/

void config_init()
{
	config.rotation = 3; // 270 degrees
	config.inactivity_time = 5; // 5 min

	FILE* file = fopen(config_file, "r");
	if (file == NULL) {
		LV_LOG_ERROR("Error opening file for reading.");
		return;
	}
	char line[100];
	char* key_rotation = "rotation";
	char* key_inactivity_time = "inactivity_time";
	while (fgets(line, 100, file) != NULL) {
		if (strstr(line, key_rotation) != NULL) {
			// TODO: change atoi
			config.rotation = atoi(line + strlen(key_rotation) + 1);
		}
		if (strstr(line, key_inactivity_time) != NULL) {
			config.inactivity_time =
					atoi(line + strlen(key_inactivity_time) + 1);
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