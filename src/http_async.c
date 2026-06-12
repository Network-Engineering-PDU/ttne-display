#include "http_async.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <lvgl/lvgl.h>
#include <time.h>

#define MAX_ASYNC_REQUESTS 64
#define THREAD_POOL_SIZE 2

typedef enum {
	ASYNC_REQ_EMPTY = 0,
	ASYNC_REQ_PENDING,
	ASYNC_REQ_RUNNING,
	ASYNC_REQ_DONE
} async_req_status_t;

/* Callback result structure */
typedef struct {
	int req_id;
	int err;
	void* buffer;
	size_t len;
	http_async_callback_t callback;
	void* userdata;
} callback_result_t;

/* Async request state */
typedef struct {
	int req_id;
	char* url;
	void* buffer;
	size_t len;
	size_t buflen;
	http_async_callback_t callback;
	void* userdata;
	async_req_status_t status;
	int err;
} async_req_state_t;

/* Global state */
static async_req_state_t requests[MAX_ASYNC_REQUESTS];
static callback_result_t results[MAX_ASYNC_REQUESTS];
static int result_count = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_t threads[THREAD_POOL_SIZE];
static bool shutdown_flag = false;
static int next_req_id = 1;

/* Write callback for CURL */
static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	size_t realsize = size * nmemb;
	async_req_state_t* req = (async_req_state_t*) userdata;

	while (req->buflen < req->len + realsize + 1) {
		req->buffer = realloc(req->buffer, req->buflen + 4096);
		req->buflen += 4096;
	}
	memcpy(&((char*)req->buffer)[req->len], ptr, realsize);
	req->len += realsize;
	((char*)req->buffer)[req->len] = 0;

	return realsize;
}

/* Worker thread function */
static void* worker_thread(void* arg)
{
	(void)arg;

	while (1) {
		async_req_state_t* req = NULL;

		pthread_mutex_lock(&mutex);
		
		/* Look for a pending request */
		for (int i = 0; i < MAX_ASYNC_REQUESTS; i++) {
			if (requests[i].status == ASYNC_REQ_PENDING) {
				req = &requests[i];
				req->status = ASYNC_REQ_RUNNING;
				break;
			}
		}

		/* Wait if no requests and not shutting down */
		if (!req && !shutdown_flag) {
			pthread_cond_wait(&cond, &mutex);
			pthread_mutex_unlock(&mutex);
			continue;
		}

		if (shutdown_flag && !req) {
			pthread_mutex_unlock(&mutex);
			break;
		}

		pthread_mutex_unlock(&mutex);

		if (!req) continue;

		/* Perform HTTP request */
		CURL* curl = curl_easy_init();
		if (curl) {
			struct curl_slist* headers = NULL;
			headers = curl_slist_append(headers, "accept: application/json");

			curl_easy_setopt(curl, CURLOPT_URL, req->url);
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 1000L);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 3000L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)req);

			CURLcode res = curl_easy_perform(curl);
			int retcode = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retcode);

			LV_LOG_USER("Async curl result: %u, retcode: %d [%s]", res, retcode, req->url);

			req->err = (res != CURLE_OK || retcode < 200 || retcode >= 300) ? 1 : 0;
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
		} else {
			req->err = 1;
		}

		/* Mark as completed and add to results */
		pthread_mutex_lock(&mutex);
		req->status = ASYNC_REQ_DONE;

		if (result_count < MAX_ASYNC_REQUESTS) {
			callback_result_t* result = &results[result_count++];
			result->req_id = req->req_id;
			result->err = req->err;
			result->buffer = req->buffer;
			result->len = req->len;
			result->callback = req->callback;
			result->userdata = req->userdata;
			req->buffer = NULL;
		} else {
			free(req->buffer);
			req->buffer = NULL;
			free(req->url);
			req->url = NULL;
			req->req_id = 0;
			req->status = ASYNC_REQ_EMPTY;
		}

		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}

void http_async_init(void)
{
	memset(requests, 0, sizeof(requests));
	memset(results, 0, sizeof(results));
	result_count = 0;
	shutdown_flag = false;

	for (int i = 0; i < THREAD_POOL_SIZE; i++) {
		pthread_create(&threads[i], NULL, worker_thread, NULL);
	}
}

int http_async_get(const char* url, http_async_callback_t callback, void* userdata)
{
	pthread_mutex_lock(&mutex);

	/* Find an available request slot */
	int req_idx = -1;
	for (int i = 0; i < MAX_ASYNC_REQUESTS; i++) {
		if (requests[i].status == ASYNC_REQ_EMPTY) {
			req_idx = i;
			break;
		}
	}

	if (req_idx == -1) {
		LV_LOG_ERROR("No available async request slots");
		pthread_mutex_unlock(&mutex);
		return -1;
	}

	int req_id = next_req_id++;
	if (next_req_id > 100000) next_req_id = 1;

	async_req_state_t* req = &requests[req_idx];
	req->req_id = req_id;
	req->url = malloc(strlen(url) + 1);
	if (req->url == NULL) {
		memset(req, 0, sizeof(*req));
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	strcpy(req->url, url);
	req->buffer = malloc(4096);
	if (req->buffer == NULL) {
		free(req->url);
		memset(req, 0, sizeof(*req));
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	req->buflen = 4096;
	req->len = 0;
	req->callback = callback;
	req->userdata = userdata;
	req->status = ASYNC_REQ_PENDING;
	req->err = 0;

	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);

	return req_id;
}

void http_async_process_callbacks(void)
{
	callback_result_t pending[MAX_ASYNC_REQUESTS];
	int pending_count;

	pthread_mutex_lock(&mutex);

	pending_count = result_count;
	memcpy(pending, results, sizeof(callback_result_t) * pending_count);
	memset(results, 0, sizeof(results));
	result_count = 0;

	for (int i = 0; i < pending_count; i++) {
		callback_result_t* result = &pending[i];

		/* Find and clear the request before running user code. */
		for (int j = 0; j < MAX_ASYNC_REQUESTS; j++) {
			if (requests[j].req_id == result->req_id) {
				free(requests[j].url);
				memset(&requests[j], 0, sizeof(requests[j]));
				break;
			}
		}
	}

	pthread_mutex_unlock(&mutex);

	for (int i = 0; i < pending_count; i++) {
		callback_result_t* result = &pending[i];
		if (result->callback) {
			result->callback(result->err, result->buffer, result->len,
					result->userdata);
		} else {
			free(result->buffer);
		}
	}
}

void http_async_cleanup(void)
{
	pthread_mutex_lock(&mutex);
	shutdown_flag = true;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);

	for (int i = 0; i < THREAD_POOL_SIZE; i++) {
		pthread_join(threads[i], NULL);
	}

	/* Cleanup remaining requests */
	for (int i = 0; i < MAX_ASYNC_REQUESTS; i++) {
		if (requests[i].req_id > 0) {
			if (requests[i].url) free(requests[i].url);
			if (requests[i].buffer) free(requests[i].buffer);
		}
	}
}
