#include <stdio.h>
#include <string.h>

#include "screen.h"
#include "config.h"

#include "lvgl/lvgl.h"

/* Global variables ***********************************************************/
/* Function prototypes ********************************************************/
/* Callbacks ******************************************************************/
/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void screen_init()
{
}

void screen_set_rotation(int rotation)
{
	lv_disp_t* disp = lv_disp_get_default();
	switch (rotation) {
	case 0:
		lv_disp_set_rotation(disp, LV_DISP_ROT_NONE); 
		break;
	case 1:
		lv_disp_set_rotation(disp, LV_DISP_ROT_90); 
		break;
	case 2:
		lv_disp_set_rotation(disp, LV_DISP_ROT_180); 
		break;
	case 3:
		lv_disp_set_rotation(disp, LV_DISP_ROT_270); 
		break;
	default:
		break;
	}

}

bool screen_is_landscape()
{
	int rotation = config_get_rotation();
	if (rotation == 1 || rotation == 3) {
		return true;
	}
	return false;
}