#ifndef HTTP_ASYNC_H
#define HTTP_ASYNC_H

#include <stddef.h>
#include <stdbool.h>

/* Callback function type for async HTTP requests */
typedef void (*http_async_callback_t)(int err, void* buffer, size_t len, void* userdata);

/* Async request handle */
typedef struct {
	int req_id;
	char* url;
	http_async_callback_t callback;
	void* userdata;
} http_async_req_t;

/**
 * Initialize async HTTP module
 * Must be called once at startup
 */
void http_async_init(void);

/**
 * Get async HTTP request (non-blocking)
 * @param url URL to fetch
 * @param callback Function to call when request completes
 * @param userdata User data passed to callback
 * @return request ID or -1 on error
 */
int http_async_get(const char* url, http_async_callback_t callback, void* userdata);

/**
 * Cleanup async HTTP module
 * Call before exit
 */
void http_async_cleanup(void);

/**
 * Process async callbacks (call from main thread periodically)
 */
void http_async_process_callbacks(void);

#endif // HTTP_ASYNC_H
