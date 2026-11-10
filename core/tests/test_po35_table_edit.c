#ifdef NDEBUG
#undef NDEBUG
#endif
#include "po35.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define FRAME_CAPACITY (PO35_PREAMBLE_BYTES + 4096u)

typedef struct packets {
  po_packet_t values[32];
  size_t count;
} packets_t;

static int collect(const po_packet_t *packet, void *user) {
  packets_t *packets = (packets_t *)user;
  assert(packets->count < sizeof(packets->values) / sizeof(packets->values[0]));
  packets->values[packets->count++] = *packet;
  return 0;
}

static void parse(const uint8_t *frame, size_t length, packets_t *packets) {
  po35_decode_result_t result;
  po_status_t status;
  *packets = (packets_t){0};
  status = po35_frame_parse(frame, length, collect, packets, &result);
  assert(status == PO_OK);
  assert(result.done && !result.stopped && result.frame_bytes == length);
}

static void append(po35_builder_t *builder, uint16_t tag, const uint8_t *data, size_t size) {
  assert(po35_builder_append_packet(builder, tag, data, size, NULL) == PO_OK);
}

static size_t full_frame(uint8_t *frame, size_t capacity, uint8_t index) {
  po35_builder_t builder;
  uint8_t raw[PO_MAX_PAYLOAD], pattern[PO35_PATTERN_BYTES];
  uint8_t settings[PO35_SETTINGS_BASE_BYTES + 3u] = {0};
  const uint8_t header[PO35_SPEECH_HEADER_BYTES] = {4, 0, 6, 60, 127, 1, 2, 3, 4};
  uint8_t records[6u * PO35_SPEECH_RECORD_BYTES];
  po35_table_1c7f_t table = {0};
  uint8_t row[PO35_TABLE_1C7F_BYTES];
  size_t length;

  for (size_t i = 0; i < sizeof(raw); ++i)
    raw[i] = (uint8_t)(29u * i + 7u);
  for (size_t i = 0; i < sizeof(pattern); ++i)
    pattern[i] = (uint8_t)(17u * i + 3u);
  for (size_t i = 0; i < sizeof(records); ++i)
    records[i] = (uint8_t)(13u * i + 5u);
  for (size_t i = 0; i < sizeof(table.data_raw); ++i)
    table.data_raw[i] = (uint8_t)(19u * i);
  settings[152] = 120;
  settings[155] = 3;
  settings[156] = 3;
  settings[157] = 3;
  settings[158] = 0;

  assert(po35_builder_init(&builder, frame, capacity) == PO_OK);
  append(&builder, 0x1234u, raw, sizeof(raw));
  table.index = (uint8_t)(index ^ 0x80u);
  assert(po35_table_1c7f_encode(&table, row, sizeof(row)) == PO_OK);
  append(&builder, PO35_TAG_TABLE_1C7F, row, sizeof(row));
  table.data_raw[0] ^= 0x55u;
  assert(po35_table_1c7f_encode(&table, row, sizeof(row)) == PO_OK);
  append(&builder, PO35_TAG_TABLE_1C7F, row, sizeof(row));
  table.data_raw[0] ^= 0x55u;
  append(&builder, PO35_TAG_PATCH, raw, 43u);
  table.index = index;
  assert(po35_table_1c7f_encode(&table, row, sizeof(row)) == PO_OK);
  append(&builder, PO35_TAG_TABLE_1C7F, row, sizeof(row));
  append(&builder, PO35_TAG_TABLE_7BE7, raw, PO35_TABLE_7BE7_BYTES);
  append(&builder, PO35_TAG_PATTERN, pattern, sizeof(pattern));
  append(&builder, PO35_TAG_SETTINGS, settings, sizeof(settings));
  append(&builder, PO35_TAG_SPEECH_HEADER, header, sizeof(header));
  append(&builder, PO35_TAG_SPEECH_DATA, records, 13u);
  append(&builder, PO35_TAG_SPEECH_DATA, records + 13u, 31u);
  append(&builder, 0x5678u, raw, 7u);
  append(&builder, PO35_TAG_SPEECH_DATA, records + 44u, sizeof(records) - 44u);
  append(&builder, PO35_TAG_SPEECH_END, NULL, 0u);
  append(&builder, 0x4321u, NULL, 0u);
  assert(po35_builder_finish(&builder, &length) == PO_OK);
  return length;
}

static void assert_filled(const uint8_t *bytes, size_t count, uint8_t value) {
  for (size_t i = 0; i < count; ++i)
    assert(bytes[i] == value);
}

static void test_preserves_unrelated_records(void) {
  uint8_t source[FRAME_CAPACITY], saved[FRAME_CAPACITY];
  uint8_t workspace[FRAME_CAPACITY], output[FRAME_CAPACITY], inverse[FRAME_CAPACITY];
  const po35_table_1c7f_t replacement = {3, {255, 0, 128, 127, 1, 2, 3, 4, 5, 6, 7, 8}};
  po35_table_1c7f_t original = {0};
  uint8_t encoded[PO35_TABLE_1C7F_BYTES];
  packets_t before, after;
  size_t length, written = 99u, changed = 0u, other_rows = 0u;

  memset(source, 0x69, sizeof(source));
  length = full_frame(source, sizeof(source), replacement.index);
  memcpy(saved, source, sizeof(source));
  memset(output, 0xa5, sizeof(output));
  assert(po35_table_1c7f_replace(source, length, &replacement, workspace, sizeof(workspace), output,
                                 sizeof(output), &written) == PO_OK);
  assert(written == length && memcmp(source, saved, sizeof(source)) == 0);
  assert_filled(output + written, sizeof(output) - written, 0xa5);
  assert(po35_table_1c7f_encode(&replacement, encoded, sizeof(encoded)) == PO_OK);
  parse(source, length, &before);
  parse(output, written, &after);
  assert(before.count == after.count);
  for (size_t i = 0; i < before.count; ++i) {
    const po_packet_t *a = before.values + i, *b = after.values + i;
    assert(a->tag_code == b->tag_code && a->payload_len == b->payload_len &&
           a->offset == b->offset);
    if (a->tag_code == PO35_TAG_TABLE_1C7F && a->payload[0] == replacement.index) {
      assert(b->payload_len == sizeof(encoded) && !memcmp(b->payload, encoded, sizeof(encoded)));
      assert(po35_table_1c7f_decode(a->payload, a->payload_len, &original) == PO_OK);
      ++changed;
    } else {
      assert(memcmp(a->payload, b->payload, a->payload_len) == 0);
      if (a->tag_code == PO35_TAG_TABLE_1C7F)
        ++other_rows;
    }
  }
  assert(changed == 1u && other_rows == 2u);

  memset(inverse, 0x5a, sizeof(inverse));
  assert(po35_table_1c7f_replace(output, length, &replacement, workspace, sizeof(workspace),
                                 inverse, sizeof(inverse), &written) == PO_OK);
  assert(written == length && !memcmp(output, inverse, length));
  assert_filled(inverse + length, sizeof(inverse) - length, 0x5a);
  assert(po35_table_1c7f_replace(output, length, &original, workspace, sizeof(workspace), inverse,
                                 sizeof(inverse), &written) == PO_OK);
  assert(written == length && !memcmp(source, inverse, length));
  assert_filled(inverse + length, sizeof(inverse) - length, 0x5a);
}

static size_t row_frame(uint8_t *frame, size_t capacity, uint8_t index) {
  const po35_table_1c7f_t row = {index, {0, 255, 128, 127, 1, 2, 3, 4, 5, 6, 7, 8}};
  uint8_t payload[PO35_TABLE_1C7F_BYTES];
  po35_builder_t builder;
  size_t length;
  assert(po35_table_1c7f_encode(&row, payload, sizeof(payload)) == PO_OK);
  assert(po35_builder_init(&builder, frame, capacity) == PO_OK);
  append(&builder, PO35_TAG_TABLE_1C7F, payload, sizeof(payload));
  assert(po35_builder_finish(&builder, &length) == PO_OK);
  return length;
}

static void test_partial_frame_and_raw_indices(void) {
  uint8_t source[PO35_PREAMBLE_BYTES + 64u], workspace[sizeof(source)], output[sizeof(source)];
  const uint8_t indices[] = {0, 7, 8, 255};
  po35_table_1c7f_t replacement = {0};
  uint8_t encoded[PO35_TABLE_1C7F_BYTES];
  packets_t packets;
  for (size_t i = 0; i < sizeof(replacement.data_raw); ++i)
    replacement.data_raw[i] = (uint8_t)(255u - 13u * i);
  for (size_t i = 0; i < sizeof(indices); ++i) {
    size_t length = row_frame(source, sizeof(source), indices[i]);
    size_t written = 99u;
    replacement.index = indices[i];
    memset(output, 0xa5, sizeof(output));
    assert(po35_table_1c7f_replace(source, length, &replacement, workspace, length, output, length,
                                   &written) == PO_OK);
    assert(written == length);
    assert_filled(output + length, sizeof(output) - length, 0xa5);
    parse(output, length, &packets);
    assert(packets.count == 1u);
    assert(po35_table_1c7f_encode(&replacement, encoded, sizeof(encoded)) == PO_OK);
    assert(!memcmp(packets.values[0].payload, encoded, sizeof(encoded)));
  }
}

static void assert_rejected(const uint8_t *source, size_t length,
                            const po35_table_1c7f_t *replacement, po_status_t expected) {
  uint8_t workspace[FRAME_CAPACITY], output[FRAME_CAPACITY];
  size_t written = 99u;
  memset(output, 0xa5, sizeof(output));
  assert(po35_table_1c7f_replace(source, length, replacement, workspace, sizeof(workspace), output,
                                 sizeof(output), &written) == expected);
  assert(written == 0u);
  assert_filled(output, sizeof(output), 0xa5);
}

static void test_row_shape_and_framing_failures(void) {
  uint8_t source[FRAME_CAPACITY], row[PO35_TABLE_1C7F_BYTES + 1u] = {3};
  uint8_t other[PO35_TABLE_1C7F_BYTES] = {9};
  po35_table_1c7f_t replacement = {3, {0}};
  po35_builder_t builder;
  size_t length;

  assert(po35_builder_init(&builder, source, sizeof(source)) == PO_OK);
  assert(po35_builder_finish(&builder, &length) == PO_OK);
  assert_rejected(source, length, &replacement, PO_ERR_PARSE);
  length = row_frame(source, sizeof(source), 4u);
  assert_rejected(source, length, &replacement, PO_ERR_PARSE);

  assert(po35_builder_init(&builder, source, sizeof(source)) == PO_OK);
  append(&builder, PO35_TAG_TABLE_7BE7, row, PO35_TABLE_7BE7_BYTES);
  assert(po35_builder_finish(&builder, &length) == PO_OK);
  assert_rejected(source, length, &replacement, PO_ERR_PARSE);

  assert(po35_builder_init(&builder, source, sizeof(source)) == PO_OK);
  append(&builder, PO35_TAG_TABLE_1C7F, row, PO35_TABLE_1C7F_BYTES);
  append(&builder, PO35_TAG_TABLE_1C7F, row, PO35_TABLE_1C7F_BYTES);
  assert(po35_builder_finish(&builder, &length) == PO_OK);
  assert_rejected(source, length, &replacement, PO_ERR_PARSE);

  /* Even an unrelated malformed table prevents an ambiguous structural edit. */
  for (size_t bad_size = 0; bad_size <= sizeof(row); ++bad_size) {
    if (bad_size == PO35_TABLE_1C7F_BYTES)
      continue;
    assert(po35_builder_init(&builder, source, sizeof(source)) == PO_OK);
    append(&builder, PO35_TAG_TABLE_1C7F, row, bad_size);
    append(&builder, PO35_TAG_TABLE_1C7F, other, sizeof(other));
    assert(po35_builder_finish(&builder, &length) == PO_OK);
    assert_rejected(source, length, &replacement, PO_ERR_PARSE);
    replacement.index = 9u;
    assert_rejected(source, length, &replacement, PO_ERR_PARSE);
    /* A later bad tail must not be hidden by an early semantic error. */
    source[length - 1u] ^= 1u;
    assert_rejected(source, length, &replacement, PO_ERR_FRAME);
    source[length - 1u] ^= 1u;
    source[length] = 0x55u;
    assert_rejected(source, length + 1u, &replacement, PO_ERR_FRAME);
    replacement.index = 3u;
  }

  length = row_frame(source, sizeof(source), replacement.index);
  for (size_t size = 0; size < length; ++size)
    assert_rejected(source, size, &replacement, PO_ERR_FRAME);
  source[length] = 0x55u;
  assert_rejected(source, length + 1u, &replacement, PO_ERR_FRAME);
  source[0] ^= 1u;
  assert_rejected(source, length, &replacement, PO_ERR_FRAME);
  source[0] ^= 1u;
  source[PO35_PREAMBLE_BYTES + 3u] ^= 1u;
  assert_rejected(source, length, &replacement, PO_ERR_FRAME);
  source[PO35_PREAMBLE_BYTES + 3u] ^= 1u;
  source[length - 1u] ^= 1u;
  assert_rejected(source, length, &replacement, PO_ERR_FRAME);
}

static void test_incomplete_speech_rejected(void) {
  uint8_t source[FRAME_CAPACITY], row[PO35_TABLE_1C7F_BYTES] = {3};
  uint8_t records[PO35_SPEECH_RECORD_BYTES] = {0};
  uint8_t header[PO35_SPEECH_HEADER_BYTES] = {0, 0, 1, 80, 140, 0, 0, 0, 0};
  const po35_table_1c7f_t replacement = {3, {0}};
  po35_builder_t builder;
  size_t length;
  for (unsigned variant = 0; variant < 5u; ++variant) {
    assert(po35_builder_init(&builder, source, sizeof(source)) == PO_OK);
    append(&builder, PO35_TAG_TABLE_1C7F, row, sizeof(row));
    if (variant != 0u)
      append(&builder, PO35_TAG_SPEECH_HEADER, header, sizeof(header));
    if (variant != 1u)
      append(&builder, PO35_TAG_SPEECH_DATA, records,
             variant == 2u ? sizeof(records) - 1u : sizeof(records));
    if (variant != 3u)
      append(&builder, PO35_TAG_SPEECH_END, NULL, 0u);
    if (variant == 4u) {
      append(&builder, PO35_TAG_SPEECH_HEADER, header, sizeof(header));
      append(&builder, PO35_TAG_SPEECH_DATA, records, sizeof(records));
      append(&builder, PO35_TAG_SPEECH_END, NULL, 0u);
    }
    assert(po35_builder_finish(&builder, &length) == PO_OK);
    assert_rejected(source, length, &replacement, PO_ERR_PARSE);
  }
}

static void test_buffer_and_null_failures(void) {
  uint8_t source[FRAME_CAPACITY], workspace[FRAME_CAPACITY], output[FRAME_CAPACITY];
  const po35_table_1c7f_t replacement = {3, {0}};
  size_t length = row_frame(source, sizeof(source), replacement.index);
  for (unsigned variant = 0; variant < 9u; ++variant) {
    const uint8_t *input = source;
    const po35_table_1c7f_t *row = &replacement;
    uint8_t *staging = workspace, *destination = output;
    size_t workspace_bytes = sizeof(workspace), capacity = sizeof(output);
    size_t written = 99u;
    size_t *written_pointer = &written;
    po_status_t expected = PO_ERR_INVALID_ARG;
    switch (variant) {
    case 0:
      input = NULL;
      break;
    case 1:
      row = NULL;
      break;
    case 2:
      staging = NULL;
      break;
    case 3:
      destination = NULL;
      break;
    case 4:
      written_pointer = NULL;
      break;
    case 5:
      workspace_bytes = length - 1u;
      expected = PO_ERR_BUFFER_TOO_SMALL;
      break;
    case 6:
      capacity = length - 1u;
      expected = PO_ERR_BUFFER_TOO_SMALL;
      break;
    case 7:
      workspace_bytes = 0u;
      expected = PO_ERR_BUFFER_TOO_SMALL;
      break;
    case 8:
      capacity = 0u;
      expected = PO_ERR_BUFFER_TOO_SMALL;
      break;
    default:
      assert(0);
    }
    memset(output, 0xa5, sizeof(output));
    assert(po35_table_1c7f_replace(input, length, row, staging, workspace_bytes, destination,
                                   capacity, written_pointer) == expected);
    assert(written == (written_pointer == NULL ? 99u : 0u));
    assert_filled(output, sizeof(output), 0xa5);
  }
}

typedef union aligned_storage {
  size_t count;
  po35_table_1c7f_t row;
  uint8_t bytes[FRAME_CAPACITY];
} aligned_storage_t;

static void test_aliases_and_overflow_leave_storage_unchanged(void) {
  aligned_storage_t source, workspace, output, replacement;
  aligned_storage_t saved_source, saved_workspace, saved_output, saved_replacement;
  for (unsigned variant = 0; variant < 13u; ++variant) {
    const uint8_t *input;
    const po35_table_1c7f_t *row = &replacement.row;
    uint8_t *staging = workspace.bytes, *destination = output.bytes;
    size_t workspace_bytes = sizeof(workspace.bytes), capacity = sizeof(output.bytes);
    size_t written = 99u, *written_pointer = &written;
    size_t length;
    memset(&source, 0x69, sizeof(source));
    memset(&workspace, 0xcc, sizeof(workspace));
    memset(&output, 0xa5, sizeof(output));
    memset(&replacement, 0x5a, sizeof(replacement));
    replacement.row.index = 3u;
    length = row_frame(source.bytes, sizeof(source.bytes), replacement.row.index);
    input = source.bytes;
    switch (variant) {
    case 0:
      staging = source.bytes;
      break;
    case 1:
      destination = source.bytes;
      break;
    case 2:
      staging = replacement.bytes;
      workspace_bytes = sizeof(replacement.row);
      break;
    case 3:
      destination = replacement.bytes;
      capacity = sizeof(replacement.row);
      break;
    case 4:
      staging = output.bytes;
      break;
    case 5:
      staging = output.bytes + 1u;
      workspace_bytes = length;
      break;
    case 6:
      written_pointer = &source.count;
      break;
    case 7:
      written_pointer = &replacement.count;
      break;
    case 8:
      written_pointer = &workspace.count;
      break;
    case 9:
      written_pointer = &output.count;
      break;
    case 10:
      length = SIZE_MAX;
      break;
    case 11:
      workspace_bytes = SIZE_MAX;
      break;
    case 12:
      capacity = SIZE_MAX;
      break;
    default:
      assert(0);
    }
    memcpy(&saved_source, &source, sizeof(source));
    memcpy(&saved_workspace, &workspace, sizeof(workspace));
    memcpy(&saved_output, &output, sizeof(output));
    memcpy(&saved_replacement, &replacement, sizeof(replacement));
    assert(po35_table_1c7f_replace(input, length, row, staging, workspace_bytes, destination,
                                   capacity, written_pointer) == PO_ERR_INVALID_ARG);
    assert(written == 99u);
    assert(!memcmp(&source, &saved_source, sizeof(source)));
    assert(!memcmp(&workspace, &saved_workspace, sizeof(workspace)));
    assert(!memcmp(&output, &saved_output, sizeof(output)));
    assert(!memcmp(&replacement, &saved_replacement, sizeof(replacement)));
  }
}

static void test_read_only_inputs_may_overlap(void) {
  aligned_storage_t source;
  uint8_t saved[sizeof(source.bytes)], workspace[sizeof(source.bytes)],
      output[sizeof(source.bytes)];
  uint8_t encoded[PO35_TABLE_1C7F_BYTES];
  packets_t packets;
  size_t length, written;
  memset(&source, 0x69, sizeof(source));
  length = row_frame(source.bytes, sizeof(source.bytes), 0u);
  /* The counter preamble is also a valid raw row with index zero. */
  assert(source.row.index == 0u);
  assert(po35_table_1c7f_encode(&source.row, encoded, sizeof(encoded)) == PO_OK);
  memcpy(saved, source.bytes, sizeof(saved));
  assert(po35_table_1c7f_replace(source.bytes, length, &source.row, workspace, sizeof(workspace),
                                 output, sizeof(output), &written) == PO_OK);
  assert(written == length && !memcmp(source.bytes, saved, sizeof(saved)));
  parse(output, written, &packets);
  assert(packets.count == 1u);
  assert(!memcmp(packets.values[0].payload, encoded, sizeof(encoded)));
}

int main(void) {
  test_preserves_unrelated_records();
  test_partial_frame_and_raw_indices();
  test_row_shape_and_framing_failures();
  test_incomplete_speech_rejected();
  test_buffer_and_null_failures();
  test_aliases_and_overflow_leave_storage_unchanged();
  test_read_only_inputs_may_overlap();
  puts("PO-35 table replacement tests passed");
  return 0;
}
