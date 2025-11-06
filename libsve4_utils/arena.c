#include "arena.h"

#include <arena/arena.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>

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
  return arena_realloc(arena, ptr, old_size, new_size);
}

void sve4_allocator_arena_destroy(sve4_allocator_t* arena) {
  arena_free((Arena*)&arena->state);
}

void sve4_allocator_arena_reset(sve4_allocator_t* arena) {
  arena_reset((Arena*)&arena->state);
}
