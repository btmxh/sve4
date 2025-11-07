#pragma once

#include <stdatomic.h>
#include <stddef.h>

#include "sve4_utils_export.h"

#include "allocator.h"
#include "defines.h"

typedef void (*sve4_destructor_t)(char* _Nonnull data);

typedef struct SVE4_UTILS_EXPORT {
  sve4_destructor_t _Nullable destructor;
  sve4_allocator_t* _Nullable allocator;
  atomic_size_t ref_count;
  char data[];
} sve4_buffer_t;

typedef sve4_buffer_t* sve4_buffer_ref_t;

SVE4_UTILS_EXPORT
sve4_buffer_ref_t _Nullable sve4_buffer_create(
    sve4_allocator_t* _Nullable allocator, size_t size,
    sve4_destructor_t _Nullable destructor);

SVE4_UTILS_EXPORT
sve4_buffer_ref_t _Nullable sve4_buffer_ref(sve4_buffer_ref_t _Nullable buffer);
SVE4_UTILS_EXPORT
void sve4_buffer_unref(sve4_buffer_ref_t _Nullable buffer);
SVE4_UTILS_EXPORT
void sve4_buffer_free(sve4_buffer_ref_t _Nullable* _Nullable buffer);

SVE4_UTILS_EXPORT
void* _Nonnull sve4_buffer_get_data(sve4_buffer_ref_t _Nonnull buffer);
