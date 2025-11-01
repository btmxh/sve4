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

  sve4_aligned_free(NULL, ptr, alignment);

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

static void* alloc_fn(sve4_allocator_t* self, size_t size, size_t alignment) {
  (void)self;
  (void)alignment;
  return malloc(size);
}

static void free_fn(sve4_allocator_t* self, void* ptr, size_t alignment) {
  (void)self;
  (void)alignment;
  free(ptr);
}

static MunitResult test_custom_allocator_impl_missing(
    const MunitParameter params[], void* user_data) {
  (void)params;
  (void)user_data;

  sve4_allocator_t allocator = {
      .alloc = alloc_fn,
      .free = free_fn,
  };
  sve4_allocator_impl_missing(&allocator);

  size_t size = 64;
  void* ptr = sve4_calloc(&allocator, size);
  munit_assert_ptr_not_null(ptr);

  void* ptr2 = sve4_realloc(&allocator, ptr, size * 2);
  munit_assert_ptr_not_null(ptr2);

  sve4_free(&allocator, ptr2);

  return MUNIT_OK;
}

static void* calloc_fn(sve4_allocator_t* self, size_t size, size_t alignment) {
  (void)self;
  (void)alignment;
  return calloc(1, size);
}

static MunitResult test_custom_allocator_impl_missing_calloc(
    const MunitParameter params[], void* user_data) {
  (void)params;
  (void)user_data;

  sve4_allocator_t allocator = {
      .calloc = calloc_fn,
      .free = free_fn,
  };
  sve4_allocator_impl_missing(&allocator);

  size_t size = 64;
  void* ptr = sve4_malloc(&allocator, size);
  munit_assert_ptr_not_null(ptr);

  void* ptr2 = sve4_realloc(&allocator, ptr, size * 2);
  munit_assert_ptr_not_null(ptr2);

  sve4_free(&allocator, ptr2);

  return MUNIT_OK;
}

static void* grow_fn(sve4_allocator_t* self, void* ptr, size_t old_size,
                     size_t new_size, size_t alignment) {
  (void)self;
  (void)old_size;
  (void)alignment;
  return realloc(ptr, new_size);
}

static MunitResult test_custom_allocator_impl_missing_grow(
    const MunitParameter params[], void* user_data) {
  (void)params;
  (void)user_data;

  sve4_allocator_t allocator = {
      .grow = grow_fn,
      .free = free_fn,
  };
  sve4_allocator_impl_missing(&allocator);

  size_t size = 64;
  void* ptr = sve4_malloc(&allocator, size);
  munit_assert_ptr_not_null(ptr);

  void* ptr2 = sve4_calloc(&allocator, size * 2);
  munit_assert_ptr_not_null(ptr2);

  sve4_free(&allocator, ptr);
  sve4_free(&allocator, ptr2);

  return MUNIT_OK;
}

static MunitResult test_realloc_null(const MunitParameter params[],
                                   void* user_data) {
  (void)params;
  (void)user_data;

  size_t size = 128;
  void* ptr = sve4_realloc(NULL, NULL, size);
  munit_assert_ptr_not_null(ptr);
  sve4_free(NULL, ptr);

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
    {
        "/custom_allocator_impl_missing",
        test_custom_allocator_impl_missing,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/custom_allocator_impl_missing_calloc",
        test_custom_allocator_impl_missing_calloc,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/custom_allocator_impl_missing_grow",
        test_custom_allocator_impl_missing_grow,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/realloc_null",
        test_realloc_null,
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

int main(int argc, char* argv[]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
