#include "alarms.h"
#include "lvgl/lvgl.h"

/* Function prototypes ********************************************************/
/* Callbacks ******************************************************************/
/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

void alarms_init(alarms_t* self)
{
	self->n = 0;
}

void alarms_new(alarms_t* self, alarm_desc_t* alarm)
{
	self->alarms[self->n].time = alarm->time;
	self->alarms[self->n].type = alarm->type;
	self->alarms[self->n].desc = alarm->desc;
	self->alarms[self->n].path = alarm->path;
	self->alarms[self->n].code = alarm->code;
	self->n++;
}

void alarms_del(alarms_t* self, alarm_desc_t* alarm)
{
	// for (int i = 0; i < self->n; i++) {
	// 	if (self->alarms[i] == alarm) {
	// 		self->alarms[i] = NULL;
	// 	}
	// }
	(void)alarm;
	self->n--;
}

void alarms_get(alarms_t* self);
