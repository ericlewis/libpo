#ifndef PO_FILE_IO_H
#define PO_FILE_IO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Load one raw byte file into hosted caller-owned storage. Invalid arguments
 * leave outputs untouched; with valid arguments, read failures clear them.
 * Caller frees bytes after success. */
int po_frame_read_file(const char *path, uint8_t **out_bytes, size_t *out_length);

#ifdef __cplusplus
}
#endif
#endif
