#include "arena.h"

#include "allocator.h"
#include "munit.h"

static MunitResult test_simple_alloc(const MunitParameter params[],
                                     void* user_data) {
  (void)params;
  (void)user_data;

  sve4_allocator_t alloc = sve4_allocator_arena_init;

  size_t size = 512;
  void* ptr = sve4_malloc(&alloc, size);
  munit_assert_ptr_not_null(ptr);

  sve4_allocator_arena_destroy(&alloc);
  return MUNIT_OK;
}

static MunitResult test_reset(const MunitParameter params[], void* user_data) {
  (void)params;
  (void)user_data;

  sve4_allocator_t alloc = sve4_allocator_arena_init;

  size_t size = 256;
  void* ptr1 = sve4_malloc(&alloc, size);
  munit_assert_ptr_not_null(ptr1);

  sve4_allocator_arena_reset(&alloc);

  void* ptr2 = sve4_malloc(&alloc, size);
  munit_assert_ptr_not_null(ptr2);

  // After reset, the arena should reuse memory, so ptr2 should be equal to ptr1
  munit_assert_ptr_equal(ptr1, ptr2);

  sve4_allocator_arena_destroy(&alloc);
  return MUNIT_OK;
}

static MunitResult test_alloc_many(const MunitParameter params[],
                                   void* user_data) {
  (void)params;
  (void)user_data;

  sve4_allocator_t alloc = sve4_allocator_arena_init;

  size_t size = 128;
  void* ptr = sve4_malloc(&alloc, size);
  munit_assert_ptr_not_null(ptr);

  sve4_malloc(&alloc, size);
  sve4_malloc(&alloc, size);
  sve4_malloc(&alloc, size);
  sve4_malloc(&alloc, size);
  sve4_malloc(&alloc, size);

  sve4_allocator_arena_destroy(&alloc);

  return MUNIT_OK;
}

static MunitResult test_realloc(const MunitParameter params[],
                                void* user_data) {
  (void)params;
  (void)user_data;

  sve4_allocator_t alloc = sve4_allocator_arena_init;

  size_t initial_size = 128;
  void* ptr = sve4_realloc(&alloc, NULL, 0, initial_size);
  munit_assert_ptr_not_null(ptr);

  for (size_t i = 0; i < initial_size; i++) {
    ((uint8_t*)ptr)[i] = (uint8_t)i;
  }

  size_t new_size = 256;
  void* new_ptr = sve4_realloc(&alloc, ptr, initial_size, new_size);
  munit_assert_ptr_not_null(new_ptr);

  for (size_t i = 0; i < initial_size; i++) {
    munit_assert_uint8(((uint8_t*)new_ptr)[i], ==, (uint8_t)i);
  }

  sve4_allocator_arena_destroy(&alloc);
  return MUNIT_OK;
}

static MunitResult test_free_random_memory(const MunitParameter params[],
                                           void* user_data) {
  (void)params;
  (void)user_data;

  sve4_allocator_t alloc = sve4_allocator_arena_init;

  sve4_free(&alloc, (void*)0x12345678);
  sve4_free(&alloc, (void*)0x727);
  sve4_free(&alloc, (void*)0x14);
  sve4_free(&alloc, (void*)0x621);
  sve4_free(&alloc, (void*)0xDEADBEEF);

  sve4_allocator_arena_destroy(&alloc);

  return MUNIT_OK;
}

static MunitResult test_alloc_aligned(const MunitParameter params[],
                                      void* user_data) {
  (void)params;
  (void)user_data;

  sve4_allocator_t alloc = sve4_allocator_arena_init;

  size_t size = 128, align = SVE4_MAX_ALIGN * 2;
  void* ptr = sve4_aligned_alloc(&alloc, size, align);
  munit_assert_ptr_null(ptr);

  sve4_allocator_arena_destroy(&alloc);

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
        "/reset",
        test_reset,
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
        "/alloc_many",
        test_alloc_many,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/free_random_memory",
        test_free_random_memory,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/alloc_aligned",
        test_alloc_aligned,
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
