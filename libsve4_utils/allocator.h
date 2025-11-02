#pragma once

#include <assert.h>
#include <stddef.h>

#include "defines.h"
#include "sve4_utils_export.h"

typedef struct sve4_allocator_t {
  void* _Nullable ctx;

  void* _Nullable (*_Nullable alloc)(struct sve4_allocator_t* _Nonnull self,
                                     size_t size, size_t alignment);
  void* _Nullable (*_Nullable calloc)(struct sve4_allocator_t* _Nonnull self,
                                      size_t size, size_t alignment);
  void* _Nullable (*_Nullable grow)(struct sve4_allocator_t* _Nonnull self,
                                    void* _Nullable ptr, size_t old_size,
                                    size_t new_size, size_t alignment);
  void (*_Nullable free)(struct sve4_allocator_t* _Nonnull self,
                         void* _Nullable ptr, size_t alignment);
} sve4_allocator_t;

SVE4_UTILS_EXPORT
void sve4_allocator_impl_missing(sve4_allocator_t* _Nonnull allocator);

SVE4_UTILS_EXPORT
void* _Nullable sve4_malloc(sve4_allocator_t* _Nullable allocator, size_t size);
SVE4_UTILS_EXPORT
void* _Nullable sve4_calloc(sve4_allocator_t* _Nullable allocator, size_t size);
SVE4_UTILS_EXPORT
void* _Nullable sve4_aligned_alloc(sve4_allocator_t* _Nullable allocator,
                                   size_t size, size_t alignment);
SVE4_UTILS_EXPORT
void* _Nullable sve4_aligned_calloc(sve4_allocator_t* _Nullable allocator,
                                    size_t size, size_t alignment);
SVE4_UTILS_EXPORT
void* _Nullable sve4_realloc(sve4_allocator_t* _Nullable allocator,
                             void* _Nullable ptr, size_t old_size,
                             size_t new_size);
SVE4_UTILS_EXPORT
void* _Nullable sve4_aligned_realloc(sve4_allocator_t* _Nullable allocator,
                                     void* _Nullable ptr, size_t old_size,
                                     size_t new_size, size_t alignment);
SVE4_UTILS_EXPORT
void sve4_free(sve4_allocator_t* _Nullable allocator, void* _Nullable ptr);
SVE4_UTILS_EXPORT
void sve4_aligned_free(sve4_allocator_t* _Nullable allocator,
                       void* _Nullable ptr, size_t alignment);

static inline size_t sve4_align_up(size_t n, size_t alignment) {
  assert((alignment & (alignment - 1)) == 0 &&
         "Alignment must be a power of two");
  return (n + alignment - 1) & ~(alignment - 1);
}
