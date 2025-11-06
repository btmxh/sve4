#include "allocator.h"

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"

static void* libc_alloc(sve4_allocator_t* _Nonnull self, size_t size,
                        size_t alignment) {
  (void)self;
  if (sve4_likely(alignment <= SVE4_MAX_ALIGN))
    return malloc(size);
  return aligned_alloc(alignment, size);
}

static void* libc_calloc(sve4_allocator_t* _Nonnull self, size_t size,
                         size_t alignment) {
  (void)self;
  if (sve4_likely(alignment <= SVE4_MAX_ALIGN))
    return calloc(1, size);
  void* ptr = aligned_alloc(alignment, size);
  if (ptr)
    memset(ptr, 0, size);
  return ptr;
}

static void libc_free(sve4_allocator_t* _Nonnull self, void* _Nullable ptr,
                      size_t alignment) {
  (void)self;
#ifndef _MSC_VER
  (void)alignment;
  free(ptr);
#else
  if (alignment > SVE4_MAX_ALIGN)
    _aligned_free(ptr);
  else
    free(ptr);
#endif
}

static void* libc_grow(sve4_allocator_t* _Nonnull self, void* _Nullable ptr,
                       size_t old_size, size_t new_size, size_t alignment) {
  (void)self;
  if (sve4_likely(alignment <= SVE4_MAX_ALIGN))
    return realloc(ptr, new_size);
#ifdef _MSC_VER
  (void)old_size;
  return _aligned_realloc(ptr, new_size, alignment);
#else
  void* new_ptr = aligned_alloc(alignment, new_size);
  if (new_ptr) {
    size_t copy_size = old_size < new_size ? old_size : new_size;
    if (ptr)
      memcpy(new_ptr, ptr, copy_size);
    libc_free(self, ptr, alignment);
  }
  return new_ptr;
#endif
}

static const sve4_allocator_t libc_allocator = {
    {NULL, NULL}, libc_alloc, libc_calloc, libc_grow, libc_free};

static inline sve4_allocator_t* _Nonnull sve4_allocator_get_or_default(
    sve4_allocator_t* _Nullable allocator) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  return allocator ? allocator : (sve4_allocator_t*)&libc_allocator;
#pragma GCC diagnostic pop
}

static void* alloc_based_on_grow(sve4_allocator_t* _Nonnull self, size_t size,
                                 size_t alignment) {
  return self->grow(self, NULL, 0, size, alignment);
}

static void* calloc_based_on_alloc(sve4_allocator_t* _Nonnull self, size_t size,
                                   size_t alignment) {
  void* ptr = self->alloc(self, size, alignment);
  if (ptr)
    memset(ptr, 0, size);
  return ptr;
}

static void* calloc_based_on_grow(sve4_allocator_t* _Nonnull self, size_t size,
                                  size_t alignment) {
  void* ptr = self->grow(self, NULL, 0, size, alignment);
  if (ptr)
    memset(ptr, 0, size);
  return ptr;
}

#define GROW_BASED_ON_ALLOC_FUNC(func)                                         \
  static void* grow_based_on_##func(sve4_allocator_t* _Nonnull self,           \
                                    void* _Nullable ptr, size_t old_size,      \
                                    size_t new_size, size_t alignment) {       \
    void* new_ptr = self->func(self, new_size, alignment);                     \
    if (new_ptr) {                                                             \
      size_t copy_size = old_size < new_size ? old_size : new_size;            \
      if (ptr)                                                                 \
        memcpy(new_ptr, ptr, copy_size);                                       \
      self->free(self, ptr, alignment);                                        \
    }                                                                          \
    return new_ptr;                                                            \
  }

static void default_free(sve4_allocator_t* _Nonnull self, void* _Nullable ptr,
                         size_t alignment) {
  (void)self;
  (void)ptr;
  (void)alignment;
}

GROW_BASED_ON_ALLOC_FUNC(alloc)
GROW_BASED_ON_ALLOC_FUNC(calloc)

void sve4_allocator_impl_missing(sve4_allocator_t* _Nonnull allocator) {
  assert((allocator->alloc || allocator->calloc || allocator->grow) &&
         "At least one allocation function must be provided");
  if (!allocator->alloc)
    allocator->alloc =
        allocator->calloc ? allocator->calloc : alloc_based_on_grow;
  if (!allocator->calloc)
    allocator->calloc =
        allocator->alloc ? calloc_based_on_alloc : calloc_based_on_grow;
  if (!allocator->grow)
    allocator->grow =
        allocator->alloc ? grow_based_on_alloc : grow_based_on_calloc;
  if (!allocator->free)
    allocator->free = default_free;
}

void* _Nullable sve4_malloc(sve4_allocator_t* _Nullable allocator,
                            size_t size) {
  sve4_allocator_t* alloc = sve4_allocator_get_or_default(allocator);
  return alloc->alloc(alloc, size, SVE4_MAX_ALIGN);
}

void* _Nullable sve4_calloc(sve4_allocator_t* _Nullable allocator,
                            size_t size) {
  sve4_allocator_t* alloc = sve4_allocator_get_or_default(allocator);
  return alloc->calloc(alloc, size, SVE4_MAX_ALIGN);
}

void* _Nullable sve4_aligned_alloc(sve4_allocator_t* _Nullable allocator,
                                   size_t size, size_t alignment) {
  sve4_allocator_t* alloc = sve4_allocator_get_or_default(allocator);
  return alloc->alloc(alloc, size, alignment);
}

void* _Nullable sve4_aligned_calloc(sve4_allocator_t* _Nullable allocator,
                                    size_t size, size_t alignment) {
  sve4_allocator_t* alloc = sve4_allocator_get_or_default(allocator);
  return alloc->calloc(alloc, size, alignment);
}

void* _Nullable sve4_realloc(sve4_allocator_t* _Nullable allocator,
                             void* _Nullable ptr, size_t old_size,
                             size_t new_size) {
  sve4_allocator_t* alloc = sve4_allocator_get_or_default(allocator);
  if (!ptr)
    return alloc->alloc(alloc, new_size, SVE4_MAX_ALIGN);
  return alloc->grow(alloc, ptr, old_size, new_size, SVE4_MAX_ALIGN);
}

void* _Nullable sve4_aligned_realloc(sve4_allocator_t* _Nullable allocator,
                                     void* _Nullable ptr, size_t old_size,
                                     size_t new_size, size_t alignment) {
  sve4_allocator_t* alloc = sve4_allocator_get_or_default(allocator);
  if (!ptr)
    return alloc->alloc(alloc, new_size, alignment);
  return alloc->grow(alloc, ptr, old_size, new_size, alignment);
}

void sve4_free(sve4_allocator_t* _Nullable allocator, void* _Nullable ptr) {
  if (!ptr)
    return;
  sve4_allocator_t* alloc = sve4_allocator_get_or_default(allocator);
  alloc->free(alloc, ptr, SVE4_MAX_ALIGN);
}

void sve4_aligned_free(sve4_allocator_t* _Nullable allocator,
                       void* _Nullable ptr, size_t alignment) {
  if (!ptr)
    return;
  sve4_allocator_t* alloc = sve4_allocator_get_or_default(allocator);
  alloc->free(alloc, ptr, alignment);
}
