#include "libsve4_decode/ffmpeg_packet_queue.h"

#include <stdlib.h>
#include <string.h>

#include "libsve4_log/init_test.h"

#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <tinycthread.h>

#include "munit.h"

static AVPacket* make_packet(const char* msg) {
  size_t len = strlen(msg);
  AVPacket* pkt = av_packet_alloc();
  munit_assert_ptr_not_null(pkt);
  int err = av_new_packet(pkt, (int)len);
  munit_assert_int(err, ==, 0);
  memcpy(pkt->data, msg, len);
  return pkt;
}

#define assert_success(err)                                                    \
  do {                                                                         \
    munit_assert_int((int)err.source, ==, SVE4_DECODE_ERROR_SRC_DEFAULT);      \
    munit_assert_int((int)err.error_code, ==,                                  \
                     SVE4_DECODE_ERROR_DEFAULT_SUCCESS);                       \
  } while (0);

/* 1. Basic init / push / pop / free */
static MunitResult test_basic(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  sve4_ffmpeg_packet_queue_t queue;
  sve4_decode_error_t err = sve4_ffmpeg_packet_queue_init(&queue, 4);
  assert_success(err);

  AVPacket* pkt = make_packet("hello");
  err = sve4_ffmpeg_packet_queue_push(&queue, pkt, NULL, false);
  assert_success(err);

  AVPacket* popped = NULL;
  err = sve4_ffmpeg_packet_queue_pop(&queue, &popped, NULL);
  assert_success(err);
  munit_assert_ptr(popped, !=, NULL);
  munit_assert_int(popped->size, ==, 5);
  munit_assert_memory_equal(5, popped->data, "hello");

  av_packet_free(&popped);
  sve4_ffmpeg_packet_queue_free(&queue);
  return MUNIT_OK;
}

/* 2. Push multiple packets, check FIFO */
static MunitResult test_multiple_push(const MunitParameter params[],
                                      void* data) {
  (void)params;
  (void)data;

  sve4_ffmpeg_packet_queue_t queue;
  sve4_ffmpeg_packet_queue_init(&queue, 4);

  AVPacket* p1 = make_packet("a");
  AVPacket* p2 = make_packet("b");
  AVPacket* p3 = make_packet("c");

  sve4_ffmpeg_packet_queue_push(&queue, p1, NULL, false);
  sve4_ffmpeg_packet_queue_push(&queue, p2, NULL, false);
  sve4_ffmpeg_packet_queue_push(&queue, p3, NULL, false);

  AVPacket* popped = NULL;
  sve4_ffmpeg_packet_queue_pop(&queue, &popped, NULL);
  munit_assert_memory_equal(1, popped->data, "a");
  av_packet_free(&popped);
  sve4_ffmpeg_packet_queue_pop(&queue, &popped, NULL);
  munit_assert_memory_equal(1, popped->data, "b");
  av_packet_free(&popped);
  sve4_ffmpeg_packet_queue_pop(&queue, &popped, NULL);
  munit_assert_memory_equal(1, popped->data, "c");
  av_packet_free(&popped);

  sve4_ffmpeg_packet_queue_free(&queue);
  return MUNIT_OK;
}

/* 3. Queue empty check */
static MunitResult test_empty_check(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;
  sve4_ffmpeg_packet_queue_t queue;
  sve4_ffmpeg_packet_queue_init(&queue, 2);

  bool empty = false;
  sve4_decode_error_t err = sve4_ffmpeg_packet_queue_is_empty(&queue, &empty);
  assert_success(err);
  munit_assert_true(empty);

  AVPacket* pkt = make_packet("x");
  sve4_ffmpeg_packet_queue_push(&queue, pkt, NULL, false);
  sve4_ffmpeg_packet_queue_is_empty(&queue, &empty);
  munit_assert_false(empty);

  AVPacket* popped = NULL;
  sve4_ffmpeg_packet_queue_pop(&queue, &popped, NULL);
  av_packet_free(&popped);
  sve4_ffmpeg_packet_queue_is_empty(&queue, &empty);
  munit_assert_true(empty);

  sve4_ffmpeg_packet_queue_free(&queue);
  return MUNIT_OK;
}

/* 4. Flush behavior */
static MunitResult test_clear(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;
  sve4_ffmpeg_packet_queue_t queue;
  sve4_ffmpeg_packet_queue_init(&queue, 4);

  AVPacket* pkt1 = make_packet("x");
  AVPacket* pkt2 = make_packet("y");
  sve4_ffmpeg_packet_queue_push(&queue, pkt1, NULL, false);
  sve4_ffmpeg_packet_queue_push(&queue, pkt2, NULL, false);

  sve4_decode_error_t err;
  err = sve4_ffmpeg_packet_queue_clear(&queue);
  assert_success(err);

  bool empty;
  sve4_ffmpeg_packet_queue_is_empty(&queue, &empty);
  munit_assert_true(empty);

  sve4_ffmpeg_packet_queue_free(&queue);
  return MUNIT_OK;
}

/* 5. Force push when full */
static MunitResult test_force_push(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;
  sve4_ffmpeg_packet_queue_t queue;
  sve4_ffmpeg_packet_queue_init(&queue, 2);

  AVPacket* p1 = make_packet("a");
  AVPacket* p2 = make_packet("b");
  AVPacket* p3 = make_packet("c");

  sve4_ffmpeg_packet_queue_push(&queue, p1, NULL, false);
  sve4_ffmpeg_packet_queue_push(&queue, p2, NULL, false);
  sve4_ffmpeg_packet_queue_push(&queue, p3, NULL, true);

  AVPacket* popped = NULL;
  sve4_ffmpeg_packet_queue_pop(&queue, &popped, NULL);
  munit_assert_memory_equal(1, popped->data, "a");
  av_packet_free(&popped);
  sve4_ffmpeg_packet_queue_pop(&queue, &popped, NULL);
  munit_assert_memory_equal(1, popped->data, "b");
  av_packet_free(&popped);
  sve4_ffmpeg_packet_queue_pop(&queue, &popped, NULL);
  munit_assert_memory_equal(1, popped->data, "c");
  av_packet_free(&popped);

  sve4_ffmpeg_packet_queue_free(&queue);
  return MUNIT_OK;
}

/* 6. Single producer / single consumer multithreaded */
typedef struct {
  sve4_ffmpeg_packet_queue_t* queue;
  int count;
} thread_data_t;

static int producer_thread(void* arg) {
  thread_data_t* d = arg;
  for (int i = 0; i < d->count; ++i) {
    char msg[16];
    snprintf(msg, sizeof(msg), "%d", i);
    AVPacket* pkt = make_packet(msg);
    sve4_ffmpeg_packet_queue_push(d->queue, pkt, NULL, false);
  }
  return 0;
}

static int consumer_thread(void* arg) {
  thread_data_t* d = arg;
  for (int i = 0; i < d->count; ++i) {
    AVPacket* pkt = NULL;
    sve4_ffmpeg_packet_queue_pop(d->queue, &pkt, NULL);
    av_packet_free(&pkt);
  }
  return 0;
}

static MunitResult test_multithreaded(const MunitParameter params[],
                                      void* data) {
  (void)params;
  (void)data;
  sve4_ffmpeg_packet_queue_t queue;
  sve4_ffmpeg_packet_queue_init(&queue, 4);

  thread_data_t td = {.queue = &queue, .count = 10};
  thrd_t t1, t2;
  thrd_create(&t1, producer_thread, &td);
  thrd_create(&t2, consumer_thread, &td);
  thrd_join(t1, NULL);
  thrd_join(t2, NULL);

  bool empty;
  sve4_ffmpeg_packet_queue_is_empty(&queue, &empty);
  munit_assert_true(empty);

  sve4_ffmpeg_packet_queue_free(&queue);
  return MUNIT_OK;
}

/* 7. Pop timeout behavior */
static MunitResult test_timeout(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;
  sve4_ffmpeg_packet_queue_t queue;
  sve4_ffmpeg_packet_queue_init(&queue, 2);

  AVPacket* pkt = NULL;
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 100 * 1000000}; // 100ms
  sve4_decode_error_t err = sve4_ffmpeg_packet_queue_pop(&queue, &pkt, &ts);
  munit_assert_int((int)err.source, ==, SVE4_DECODE_ERROR_SRC_DEFAULT);
  munit_assert_int((int)err.error_code, ==, SVE4_DECODE_ERROR_DEFAULT_TIMEOUT);

  sve4_ffmpeg_packet_queue_free(&queue);
  return MUNIT_OK;
}

/* MUnit test suite */
static MunitTest test_suite_tests[] = {
    {"/basic", test_basic, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/multiple_push", test_multiple_push, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {"/empty_check", test_empty_check, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {"/clear", test_clear, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/force_push", test_force_push, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/multithreaded", test_multithreaded, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {"/timeout", test_timeout, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite = {"/sve4_ffmpeg_packet_queue",
                                      test_suite_tests, NULL, 1,
                                      MUNIT_SUITE_OPTION_NONE};

int main(int argc, char* argv[]) {
  sve4_log_test_setup();
  int ret = munit_suite_main(&test_suite, NULL, argc, argv);
  sve4_log_test_teardown();
  return ret;
}
