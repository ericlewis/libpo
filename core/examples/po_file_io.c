#include "po_file_io.h"

#include <stdio.h>
#include <stdlib.h>

int po_frame_read_file(const char *path, uint8_t **out_bytes, size_t *out_length) {
  FILE *file = NULL;
  uint8_t *bytes = NULL;
  long end;
  size_t length;
  int ok = 0;

  if (path == NULL || out_bytes == NULL || out_length == NULL)
    return 0;
  *out_bytes = NULL;
  *out_length = 0u;
  file = fopen(path, "rb");
  if (file == NULL || fseek(file, 0, SEEK_END) != 0 || (end = ftell(file)) < 0 ||
      (uintmax_t)end > SIZE_MAX || fseek(file, 0, SEEK_SET) != 0)
    goto cleanup;
  length = (size_t)end;
  bytes = (uint8_t *)malloc(length ? length : 1u);
  if (bytes == NULL || (length != 0u && fread(bytes, 1u, length, file) != length) || ferror(file))
    goto cleanup;
  *out_bytes = bytes;
  *out_length = length;
  bytes = NULL;
  ok = 1;

cleanup:
  free(bytes);
  if (file != NULL && fclose(file) != 0)
    ok = 0;
  if (!ok) {
    free(*out_bytes);
    *out_bytes = NULL;
    *out_length = 0u;
  }
  return ok;
}
