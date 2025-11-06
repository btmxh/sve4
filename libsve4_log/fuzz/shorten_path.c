#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "init.h"

static char* get_cstring(const uint8_t** data, size_t* size) {
  const uint8_t* term = memchr(*data, 0, *size);
  if (!term)
    return NULL;
  size_t len = (size_t)(term - *data);
  char* str = sve4_calloc(NULL, len + 1);
  memcpy(str, *data, len);
  *data += len + 1;
  *size -= len + 1;
  return str;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 3)
    return 0;

  char outbuf[SVE4_LOG_SHORTEN_PATH_MAX_LENGTH + 1];
  size_t buf_size = data[0];
  if (buf_size > sizeof(outbuf))
    return 0;

  ++data, --size;
  char* path = get_cstring(&data, &size);
  char* trim_root_prefix = get_cstring(&data, &size);
  if (!path || !trim_root_prefix) {
    sve4_free(NULL, path);
    sve4_free(NULL, trim_root_prefix);
    return 0;
  }

  const char* result =
      sve4_log_shorten_path(outbuf, buf_size, path, trim_root_prefix);
  assert(result != NULL);

  // sanity invariants
  if (result == outbuf) {
    // result must be NUL-terminated
    assert(memchr(outbuf, '\0', sizeof(outbuf)) != NULL);
  }

  sve4_free(NULL, path);
  sve4_free(NULL, trim_root_prefix);

  return 0;
}

int main(int argc, char** argv) {
  uint8_t* buffer = NULL;
  size_t bufsize = 0;

  if (argc >= 2 && strcmp(argv[1], "@@") != 0) {
    // argv[1] is a filename; read it
    FILE* f = fopen(argv[1], "rb");
    if (!f)
      return 1;
    if (fseek(f, 0, SEEK_END) == 0) {
      long sz = ftell(f);
      if (sz > 0) {
        if ((size_t)sz > (size_t)1e8) { // avoid enormous allocations
          fclose(f);
          return 1;
        }
        bufsize = (size_t)sz;
        buffer = (uint8_t*)malloc(bufsize);
        if (!buffer) {
          fclose(f);
          return 1;
        }
        fseek(f, 0, SEEK_SET);
        fread(buffer, 1, bufsize, f);
      }
    }
    fclose(f);
  } else {
    // read stdin fully
    const size_t chunk = 4096;
    uint8_t tmp[chunk];
    while (1) {
      size_t n = fread(tmp, 1, chunk, stdin);
      if (n == 0)
        break;
      uint8_t* nb = (uint8_t*)realloc(buffer, bufsize + n);
      if (!nb) {
        free(buffer);
        return 1;
      }
      buffer = nb;
      memcpy(buffer + bufsize, tmp, n);
      bufsize += n;
      if (n < chunk)
        break;
    }
  }

  if (buffer && bufsize > 0) {
    LLVMFuzzerTestOneInput(buffer, bufsize);
  } else {
    LLVMFuzzerTestOneInput(NULL, 0);
  }

  free(buffer);
  return 0;
}
