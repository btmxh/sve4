#include "buffer.h"

#include <stdatomic.h>
#include <stdlib.h>

#include "allocator.h"

sve4_buffer_ref_t sve4_buffer_create(sve4_allocator_t* _Nullable allocator,
                                     size_t size,
                                     sve4_destructor_t _Nullable destructor) {
  sve4_buffer_t* buffer = sve4_calloc(allocator, sizeof(sve4_buffer_t) + size);
  if (!buffer)
    return NULL;
  buffer->destructor = destructor;
  buffer->allocator = allocator;
  atomic_init(&buffer->ref_count, 1);
  return buffer;
}

sve4_buffer_ref_t sve4_buffer_ref(sve4_buffer_ref_t buffer) {
  if (!buffer)
    return NULL;
  atomic_fetch_add_explicit(&buffer->ref_count, 1, memory_order_relaxed);
  return buffer;
}

void sve4_buffer_unref(sve4_buffer_ref_t buffer) {
  if (!buffer)
    return;
  if (atomic_fetch_sub_explicit(&buffer->ref_count, 1, memory_order_acq_rel) ==
      1) {
    if (buffer->destructor)
      buffer->destructor(buffer->data);
    sve4_free(buffer->allocator, buffer);
  }
}

void sve4_buffer_free(sve4_buffer_ref_t* buffer) {
  if (!buffer)
    return;
  sve4_buffer_unref(*buffer);
  *buffer = NULL;
}

void* sve4_buffer_get_data(sve4_buffer_ref_t _Nonnull buffer) {
  return buffer->data;
}
