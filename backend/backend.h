#ifndef BACKEND_H
#define BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef void (*backend_callback_t)(int err, void* userdata);

int backend_init(void);
void backend_cleanup(void);
void backend_process(void);

int backend_outlets_refresh(backend_callback_t callback, void* userdata);
int backend_outlet_set(int line_id, bool status, backend_callback_t callback,
		void* userdata);
int backend_outlets_set_all(bool status, backend_callback_t callback,
		void* userdata);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* BACKEND_H */
