#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tinycthread.h>

#include "civetweb.h"

#ifdef _WIN32
#include <windows.h>
#define TEST_HTTP_SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define TEST_HTTP_SLEEP_MS(ms) usleep((ms) * 1000)
#endif

typedef struct test_http_server test_http_server_t;

typedef enum { TEST_HTTP_TEXT, TEST_HTTP_BINARY } test_http_data_type;

typedef struct {
  const void* data;
  size_t size; // for text: if 0, use strlen(data)
  test_http_data_type type;
  int send_content_length; // 1 => include Content-Length, 0 => omit
  int chunked;             // 1 => use Transfer-Encoding: chunked
  int delay_ms;            // optional delay between chunks (simulate streaming)
} test_http_response_t;

typedef int (*test_http_request_cb)(struct mg_connection* conn,
                                    void* user_data);

struct test_http_server {
  struct mg_context* ctx;
};

struct static_handler_data {
  test_http_response_t resp;
};

/* ---------------- Internal Handlers ---------------- */

static inline void sleep_ms(int ms) {
  if (ms <= 0)
    return;
  struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L};
  thrd_sleep(&ts, NULL);
}

static int test_http_static_handler(struct mg_connection* conn, void* cbdata) {
  struct static_handler_data* data = cbdata;
  const test_http_response_t* resp = &data->resp;

  const char* ctype = resp->type == TEST_HTTP_BINARY
                          ? "application/octet-stream"
                          : "text/plain";

  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n", ctype);

  if (resp->chunked) {
    mg_printf(conn, "Transfer-Encoding: chunked\r\n");
  } else if (resp->send_content_length) {
    size_t len = resp->size ? resp->size : strlen((const char*)resp->data);
    mg_printf(conn, "Content-Length: %zu\r\n", len);
  }

  mg_printf(conn, "\r\n");

  size_t len = resp->size ? resp->size : strlen((const char*)resp->data);

  if (resp->chunked) {
    const unsigned char* p = resp->data;
    size_t sent = 0;
    while (sent < len) {
      size_t chunk = (len - sent) > 8 ? 8 : (len - sent);
      mg_printf(conn, "%zx\r\n", chunk);
      mg_write(conn, p + sent, chunk);
      mg_printf(conn, "\r\n");
      sent += chunk;
      if (resp->delay_ms)
        sleep_ms(resp->delay_ms);
    }
    mg_printf(conn, "0\r\n\r\n");
  } else {
    mg_write(conn, resp->data, len);
  }

  return 200;
}

/* ---------------- Public API ---------------- */

static inline test_http_server_t* test_http_server_start(const char* port) {
  const char* options[] = {"listening_ports", port, NULL};
  struct mg_callbacks callbacks = {0};
  test_http_server_t* srv = (test_http_server_t*)calloc(1, sizeof(*srv));
  srv->ctx = mg_start(&callbacks, NULL, options);
  return srv;
}

static inline test_http_server_t* test_http_server_start_auto(int* out_port) {
  const char* options[] = {"listening_ports", "0", NULL};
  struct mg_callbacks callbacks = {0};

  test_http_server_t* srv = (test_http_server_t*)calloc(1, sizeof(*srv));
  srv->ctx = mg_start(&callbacks, NULL, options);

  if (out_port) {
    int port = 0;
    struct mg_server_ports ports[8];
    int n = mg_get_server_ports(srv->ctx, 8, ports);
    for (int i = 0; i < n; i++) {
      if (ports[i].protocol & 1 /* PROTO_HTTP */) {
        port = ports[i].port;
        break;
      }
    }
    *out_port = port;
  }
  return srv;
}

static inline void test_http_server_stop(test_http_server_t* srv) {
  if (!srv)
    return;
  mg_stop(srv->ctx);
  free(srv);
}

static inline void
test_http_server_add_static(test_http_server_t* srv, const char* path,
                            const test_http_response_t* resp) {
  struct static_handler_data* data =
      (struct static_handler_data*)malloc(sizeof(*data));
  data->resp = *resp;
  mg_set_request_handler(srv->ctx, path, test_http_static_handler, data);
}

static inline void test_http_server_add_handler(test_http_server_t* srv,
                                                const char* path,
                                                test_http_request_cb cb,
                                                void* user_data) {
  mg_set_request_handler(srv->ctx, path, cb, user_data);
}
