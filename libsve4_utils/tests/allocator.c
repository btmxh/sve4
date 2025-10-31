#include "allocator.h"

#include "munit.h"

static MunitResult test_simple_alloc(const MunitParameter params[],
                                     void* user_data) {
  (void)params;
  (void)user_data;

  size_t size = 512;
  void* ptr = sve4_malloc(NULL, size);
  munit_assert_ptr_not_null(ptr);

  sve4_free(NULL, ptr);
  return MUNIT_OK;
}

static MunitResult test_calloc(const MunitParameter params[], void* user_data) {
  (void)params;
  (void)user_data;

  size_t size = 16;
  void* ptr = sve4_calloc(NULL, size);
  munit_assert_ptr_not_null(ptr);

  for (size_t i = 0; i < size; i++) {
    munit_assert_uint8(((uint8_t*)ptr)[i], ==, 0);
  }

  sve4_free(NULL, ptr);
  return MUNIT_OK;
}

static MunitResult test_aligned_alloc(const MunitParameter params[],
                                      void* user_data) {
  (void)params;
  (void)user_data;

  size_t size = 1024;
  size_t alignment = 64;
  void* ptr = sve4_aligned_alloc(NULL, size, alignment);

  munit_assert_ptr_not_null(ptr);
  munit_assert_uint64(((uintptr_t)ptr) % alignment, ==, 0);

  sve4_free(NULL, ptr);

  return MUNIT_OK;
}

static MunitResult test_realloc(const MunitParameter params[],
                                void* user_data) {
  (void)params;
  (void)user_data;

  size_t initial_size = 128;
  void* ptr = sve4_realloc(NULL, NULL, initial_size);
  munit_assert_ptr_not_null(ptr);

  for (size_t i = 0; i < initial_size; i++) {
    ((uint8_t*)ptr)[i] = (uint8_t)i;
  }

  size_t new_size = 256;
  void* new_ptr = sve4_realloc(NULL, ptr, new_size);
  munit_assert_ptr_not_null(new_ptr);

  for (size_t i = 0; i < initial_size; i++) {
    munit_assert_uint8(((uint8_t*)new_ptr)[i], ==, (uint8_t)i);
  }

  sve4_free(NULL, new_ptr);
  return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
    {
        "/simple_alloc",
        test_simple_alloc,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/calloc",
        test_calloc,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/realloc",
        test_realloc,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/aligned_alloc",
        test_aligned_alloc,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL} /* Mark the end of the array */
};

static const MunitSuite test_suite = {
    "/allocator", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
