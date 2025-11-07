#pragma once

#include <stdbool.h>

#include "allocator.h"
#include "defines.h"

// ARENA API (currently using tsoding/arena implementation)

// this being a macro allows static initialization
#define sve4_allocator_arena_init                                              \
  ((sve4_allocator_t){                                                         \
      .state = {0},                                                            \
      .alloc = sve4__allocator_arena_alloc,                                    \
      .grow = sve4__allocator_arena_grow,                                      \
      .free = sve4__allocator_arena_free,                                      \
  })

SVE4_UTILS_EXPORT
void* _Nullable sve4__allocator_arena_alloc(sve4_allocator_t* _Nonnull self,
                                            size_t size, size_t alignment);
SVE4_UTILS_EXPORT
void* _Nullable sve4__allocator_arena_grow(sve4_allocator_t* _Nonnull self,
                                           void* _Nullable ptr, size_t old_size,
                                           size_t new_size, size_t alignment);
SVE4_UTILS_EXPORT
void sve4__allocator_arena_free(sve4_allocator_t* _Nonnull self,
                                void* _Nullable ptr, size_t alignment);

SVE4_UTILS_EXPORT
void sve4_allocator_arena_destroy(sve4_allocator_t* _Nonnull arena);

SVE4_UTILS_EXPORT
void sve4_allocator_arena_reset(sve4_allocator_t* _Nonnull arena);
