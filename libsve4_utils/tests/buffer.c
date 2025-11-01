#include "buffer.h"

#include <stdbool.h>

#include "munit.h"

typedef struct {
  bool* destroyed;
} destructor_tracker;

static void destructor_tracker_destructor(char* data) {
  destructor_tracker* tracker = (destructor_tracker*)data;
  if (tracker && tracker->destroyed)
    *tracker->destroyed = true;
}

static MunitResult test_simple_buffer(const MunitParameter params[],
                                      void* user_data) {
  (void)params;
  (void)user_data;

  sve4_buffer_ref_t buf = sve4_buffer_create(NULL, 128, NULL);
  munit_assert_ptr_not_null(buf);
  munit_assert_size(buf->ref_count, ==, 1);

  sve4_buffer_free(&buf);
  munit_assert_ptr_null(buf);

  return MUNIT_OK;
}

static MunitResult test_buffer_with_destructor(const MunitParameter params[],
                                               void* user_data) {
  (void)params;
  (void)user_data;

  bool destroyed = false;
  sve4_buffer_ref_t buf = sve4_buffer_create(NULL, sizeof(destructor_tracker),
                                             destructor_tracker_destructor);
  munit_assert_ptr_not_null(buf);
  munit_assert_size(buf->ref_count, ==, 1);

  destructor_tracker* tracker = (destructor_tracker*)buf->data;
  tracker->destroyed = &destroyed;

  sve4_buffer_free(&buf);
  munit_assert_ptr_null(buf);
  munit_assert_true(destroyed);

  return MUNIT_OK;
}

static MunitResult test_ref_unref(const MunitParameter params[],
                                  void* user_data) {
  (void)params;
  (void)user_data;

  sve4_buffer_ref_t buf = sve4_buffer_create(NULL, 128, NULL);
  munit_assert_ptr_not_null(buf);
  munit_assert_size(buf->ref_count, ==, 1);
  sve4_buffer_ref_t buf2 = sve4_buffer_ref(buf);
  munit_assert_size(buf->ref_count, ==, 2);

  sve4_buffer_free(&buf);
  munit_assert_ptr_null(buf);
  sve4_buffer_free(&buf);
  munit_assert_ptr_null(buf);

  munit_assert_size(buf2->ref_count, ==, 1);
  sve4_buffer_free(&buf2);
  munit_assert_ptr_null(buf2);

  return MUNIT_OK;
}

static MunitResult test_ref_unref_destructor(const MunitParameter params[],
                                             void* user_data) {
  (void)params;
  (void)user_data;

  bool destroyed = false;
  sve4_buffer_ref_t buf = sve4_buffer_create(NULL, sizeof(destructor_tracker),
                                             destructor_tracker_destructor);
  munit_assert_ptr_not_null(buf);
  munit_assert_size(buf->ref_count, ==, 1);

  destructor_tracker* tracker = (destructor_tracker*)buf->data;
  tracker->destroyed = &destroyed;

  sve4_buffer_ref_t buf2 = sve4_buffer_ref(buf);
  munit_assert_size(buf->ref_count, ==, 2);

  sve4_buffer_free(&buf);
  munit_assert_ptr_null(buf);
  sve4_buffer_free(&buf);
  munit_assert_ptr_null(buf);
  munit_assert_false(destroyed);

  munit_assert_size(buf2->ref_count, ==, 1);
  sve4_buffer_free(&buf2);
  munit_assert_ptr_null(buf2);
  munit_assert_true(destroyed);

  return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
    {
        "/simple_buffer",
        test_simple_buffer,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/buffer_with_destructor",
        test_buffer_with_destructor,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/ref_unref",
        test_ref_unref,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/ref_unref_destructor",
        test_ref_unref_destructor,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL} /* Mark the end of the array */
};

static const MunitSuite test_suite = {
    "/buffer", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
