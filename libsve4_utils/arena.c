#include "arena.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#define ARENA_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <arena/arena.h>
#pragma GCC diagnostic pop

#include "allocator.h"

void* sve4__allocator_arena_alloc(sve4_allocator_t* _Nonnull self, size_t size,
                                  size_t alignment) {
  Arena* arena = (Arena*)&self->state;
  if (alignment > SVE4_MAX_ALIGN)
    return NULL;
  return arena_alloc(arena, size);
}

void* sve4__allocator_arena_grow(sve4_allocator_t* _Nonnull self,
                                 void* _Nullable ptr, size_t old_size,
                                 size_t new_size, size_t alignment) {
  Arena* arena = (Arena*)&self->state;
  if (alignment > SVE4_MAX_ALIGN)
    return NULL;
  // arena_realloc can't handle NULL as old_ptr
  if (!ptr)
    return arena_alloc(arena, new_size);
  return arena_realloc(arena, ptr, old_size, new_size);
}

void sve4__allocator_arena_free(sve4_allocator_t* _Nonnull self,
                                void* _Nullable ptr, size_t alignment) {
  (void)self;
  (void)ptr;
  (void)alignment;
}

void sve4_allocator_arena_destroy(sve4_allocator_t* arena) {
  arena_free((Arena*)&arena->state);
}

void sve4_allocator_arena_reset(sve4_allocator_t* arena) {
  arena_reset((Arena*)&arena->state);
}
