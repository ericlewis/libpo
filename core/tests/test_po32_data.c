#ifdef NDEBUG
#undef NDEBUG
#endif
#include "po32.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Decoded records from TE's official Basic Beats 1 pack:
 * https://teenage.engineering/downloads/po-32#basic-beats-1
 * State payload SHA-256: 3228f26e1ad5a204855307d0b2b16f6ca0c526d0c3b3e6f1d942e7335f503a3a
 * Pattern 2 payload SHA-256: 47fcbdf74b9dd39e836365f322a665292c8b07100e35df21653713ab2196e2f4
 * These fixtures contain retained inactive selectors/accents, but no trigger
 * bit 6. That bit is covered separately by synthetic all-byte tests. */
static const uint8_t captured_state[] = {
    0x44, 0x7a, 0xff, 0x79, 0x55, 0xff, 0x01, 0xff, 0xa3, 0xde, 0xb8, 0xa0, 0x48, 0x01, 0x01,
    0x01, 0x37, 0x80, 0x80, 0x80, 0x57, 0x20, 0xff, 0xff, 0x70, 0x2e, 0x01, 0xf4, 0x54, 0xff,
    0x80, 0x80, 0x8c, 0x00, 0x16, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x08, 0x0a, 0x0b, 0x0b, 0x0c, 0x0c, 0x0c, 0x0d, 0x0e, 0x0e, 0x0f, 0x0f};
static const uint8_t captured_pattern[] = {
    0x02, 0xb1, 0x30, 0x30, 0x30, 0xb1, 0x30, 0x30, 0x30, 0xb1, 0x30, 0x30, 0x30, 0xb1, 0x30, 0x30,
    0x30, 0x7e, 0x10, 0x7e, 0x13, 0x7e, 0x15, 0x7e, 0x19, 0x7e, 0x1f, 0x7e, 0x20, 0x7e, 0x21, 0x7e,
    0x23, 0x7e, 0x29, 0x7e, 0x2a, 0x7e, 0x2b, 0x7e, 0x2b, 0x7e, 0x2c, 0x7e, 0x2c, 0x7e, 0x2c, 0x7e,
    0x0b, 0x80, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xa0, 0x20, 0x21, 0x20, 0xa0, 0x20, 0x21, 0x20, 0xa0, 0x20, 0x21, 0x20, 0xa0, 0x20, 0x21,
    0x20, 0x01, 0x9f, 0x01, 0xa5, 0x01, 0xa7, 0x01, 0xb0, 0x01, 0xb3, 0x01, 0xca, 0x01, 0xe7, 0x01,
    0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0x90, 0x01, 0x96, 0x01, 0x97, 0x01,
    0x9e, 0x80, 0x00, 0x80, 0x00, 0x81, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x81, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x11, 0x11};

static void assert_filled(const void *data, size_t count, uint8_t value) {
  const uint8_t *bytes = data;
  for (size_t i = 0; i < count; ++i)
    assert(bytes[i] == value);
}

static void test_captured_records(void) {
  static const uint8_t chain[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  8,
                                  10, 11, 11, 12, 12, 12, 13, 14, 14, 15, 15};
  po32_state_raw_t state;
  po32_pattern_raw_t pattern;
  uint8_t encoded[PO32_MAX_PAYLOAD + 8];
  size_t written = 999;
  po32_status_t status = po32_state_raw_decode(captured_state, sizeof(captured_state), &state);
  assert(status == PO32_OK);
  assert(state.pattern_count == sizeof(chain) && state.tempo == 140 && state.swing_times_12 == 0);
  assert(memcmp(state.pattern_numbers, chain, sizeof(chain)) == 0);
  assert(state.morph_pairs[0].flag == 0x44 && state.morph_pairs[0].morph == 0x7a);
  memset(encoded, 0xa5, sizeof(encoded));
  status = po32_state_raw_encode(&state, encoded, sizeof(encoded), &written);
  assert(status == PO32_OK && written == sizeof(captured_state));
  assert(memcmp(encoded, captured_state, written) == 0);
  assert_filled(encoded + written, sizeof(encoded) - written, 0xa5);

  status = po32_pattern_raw_decode(captured_pattern, sizeof(captured_pattern), &pattern);
  assert(status == PO32_OK && pattern.pattern_number == 2);
  assert(pattern.triggers[0][1] == 0x30);
  assert(pattern.triggers[1][0] == 0x80 && pattern.triggers[1][4] == 0xa0);
  assert(pattern.morph_lanes[0][0].flag == 0x7e && pattern.morph_lanes[0][0].morph == 0x10);
  assert(pattern.accent_bits == 0x1111);
  memset(encoded, 0xa5, sizeof(encoded));
  status = po32_pattern_raw_encode(&pattern, encoded, sizeof(encoded));
  assert(status == PO32_OK);
  assert(memcmp(encoded, captured_pattern, sizeof(captured_pattern)) == 0);
  assert_filled(encoded + sizeof(captured_pattern), sizeof(encoded) - sizeof(captured_pattern),
                0xa5);

  /* Existing callers retain their historical size and conversion behavior. */
  po32_state_packet_t legacy_state;
  po32_pattern_packet_t legacy_pattern;
  po32_packet_t legacy_encoded;
  status =
      po32_packet_decode(PO32_TAG_STATE, captured_state, sizeof(captured_state), &legacy_state);
  assert(status == PO32_ERR_FRAME);
  status = po32_packet_decode(PO32_TAG_PATTERN, captured_pattern, sizeof(captured_pattern),
                              &legacy_pattern);
  assert(status == PO32_OK);
  status = po32_packet_encode(PO32_TAG_PATTERN, &legacy_pattern, &legacy_encoded);
  assert(status == PO32_OK);
  assert(legacy_encoded.payload[2] == 0 && legacy_encoded.payload[49] == 0);
  assert(memcmp(legacy_encoded.payload, captured_pattern, sizeof(captured_pattern)) != 0);
}

static void test_chain_boundaries(void) {
  static const size_t counts[] = {0, 16, 17, 22, 35, 36, 64, 128, 220};
  po32_state_raw_t state = {0}, decoded;
  uint8_t expected[PO32_MAX_PAYLOAD], encoded[PO32_MAX_PAYLOAD + 4];
  for (size_t i = 0; i < PO32_STATE_MORPH_PAIR_COUNT; ++i) {
    state.morph_pairs[i].flag = expected[2 * i] = (uint8_t)(i * 17);
    state.morph_pairs[i].morph = expected[2 * i + 1] = (uint8_t)(255 - i * 13);
  }
  state.tempo = expected[32] = 240;
  state.swing_times_12 = expected[33] = 255;
  /* Raw chain IDs are preserved, not limited to observed device slot IDs. */
  for (size_t i = 0; i < PO32_CHAIN_MAX_COUNT; ++i)
    state.pattern_numbers[i] = expected[35 + i] = (uint8_t)(i * 13 + 5);
  for (size_t n = 0; n < sizeof(counts) / sizeof(counts[0]); ++n) {
    size_t written = 999;
    state.pattern_count = counts[n];
    expected[34] = (uint8_t)counts[n];
    memset(encoded, 0xa5, sizeof(encoded));
    po32_status_t status = po32_state_raw_encode(&state, encoded, sizeof(encoded), &written);
    assert(status == PO32_OK && written == 35 + counts[n]);
    assert(memcmp(encoded, expected, written) == 0);
    assert_filled(encoded + written, sizeof(encoded) - written, 0xa5);
    status = po32_state_raw_decode(encoded, written, &decoded);
    assert(status == PO32_OK && decoded.pattern_count == counts[n]);
    assert(decoded.tempo == state.tempo && decoded.swing_times_12 == state.swing_times_12);
    assert(memcmp(decoded.pattern_numbers, state.pattern_numbers, counts[n]) == 0);
    for (size_t i = 0; i < PO32_STATE_MORPH_PAIR_COUNT; ++i) {
      assert(decoded.morph_pairs[i].flag == state.morph_pairs[i].flag);
      assert(decoded.morph_pairs[i].morph == state.morph_pairs[i].morph);
    }
    status = po32_state_raw_encode(&decoded, encoded, written, NULL);
    assert(status == PO32_OK && memcmp(encoded, expected, written) == 0);
  }
}

static void test_state_edit_locality(void) {
  po32_state_raw_t state;
  uint8_t encoded[sizeof(captured_state)], expected[sizeof(captured_state)];
  for (size_t i = 0; i < 22; ++i) {
    po32_status_t status = po32_state_raw_decode(captured_state, sizeof(captured_state), &state);
    assert(status == PO32_OK);
    state.pattern_numbers[i] ^= 0x0f;
    memcpy(expected, captured_state, sizeof(expected));
    expected[35 + i] ^= 0x0f;
    status = po32_state_raw_encode(&state, encoded, sizeof(encoded), NULL);
    assert(status == PO32_OK && memcmp(encoded, expected, sizeof(encoded)) == 0);
  }
}

static void test_all_trigger_values_and_edit_locality(void) {
  uint8_t wire[PO32_PATTERN_PAYLOAD_BYTES], expected[PO32_PATTERN_PAYLOAD_BYTES],
      encoded[PO32_PATTERN_PAYLOAD_BYTES];
  for (size_t i = 0; i < sizeof(wire); ++i)
    wire[i] = (uint8_t)(i * 71 + 19);
  for (size_t lane = 0; lane < PO32_PATTERN_LANE_COUNT; ++lane) {
    for (size_t step = 0; step < PO32_PATTERN_STEP_COUNT; ++step) {
      const size_t offset = 1 + 48 * lane + step;
      for (unsigned raw = 0; raw <= 255; ++raw) {
        po32_pattern_raw_t pattern;
        po32_pattern_control_t control;
        po32_status_t status = po32_pattern_raw_decode(wire, sizeof(wire), &pattern);
        assert(status == PO32_OK);
        pattern.triggers[lane][step] = (uint8_t)raw;
        memcpy(expected, wire, sizeof(expected));
        expected[offset] = (uint8_t)raw;
        status = po32_pattern_raw_encode(&pattern, encoded, sizeof(encoded));
        assert(status == PO32_OK && memcmp(encoded, expected, sizeof(encoded)) == 0);
        status = po32_pattern_raw_decode(encoded, sizeof(encoded), &pattern);
        assert(status == PO32_OK && pattern.triggers[lane][step] == raw);
        status = po32_pattern_control_decode(pattern.triggers[lane][step], &control);
        assert(status == PO32_OK);
        control.fill_rate_raw = 5;
        status = po32_pattern_control_encode(&control, &pattern.triggers[lane][step]);
        assert(status == PO32_OK);
        expected[offset] = (uint8_t)((raw & 0xf0) | 5);
        status = po32_pattern_raw_encode(&pattern, encoded, sizeof(encoded));
        assert(status == PO32_OK && memcmp(encoded, expected, sizeof(encoded)) == 0);
      }
    }
  }
}

static void test_failures_leave_outputs_unchanged(void) {
  po32_state_raw_t state = {0}, decoded_state;
  po32_pattern_raw_t pattern = {0}, decoded_pattern;
  uint8_t bytes[PO32_MAX_PAYLOAD + 1] = {0}, encoded[PO32_MAX_PAYLOAD + 1];
  size_t written = 999;
  po32_status_t status;
  memset(encoded, 0xa5, sizeof(encoded));
  memset(&decoded_state, 0xa5, sizeof(decoded_state));
  memset(&decoded_pattern, 0xa5, sizeof(decoded_pattern));
  status = po32_state_raw_decode(NULL, 35, &decoded_state);
  assert(status == PO32_ERR_INVALID_ARG);
  status = po32_state_raw_decode(bytes, 35, NULL);
  assert(status == PO32_ERR_INVALID_ARG);
  const size_t bad_state_sizes[] = {0, 34, 36, 256};
  for (size_t i = 0; i < sizeof(bad_state_sizes) / sizeof(bad_state_sizes[0]); ++i) {
    status = po32_state_raw_decode(bytes, bad_state_sizes[i], &decoded_state);
    assert(status == PO32_ERR_FRAME);
    assert_filled(&decoded_state, sizeof(decoded_state), 0xa5);
  }
  bytes[34] = 22;
  status = po32_state_raw_decode(bytes, 56, &decoded_state);
  assert(status == PO32_ERR_FRAME);
  bytes[34] = 221;
  status = po32_state_raw_decode(bytes, 255, &decoded_state);
  assert(status == PO32_ERR_FRAME);
  assert_filled(&decoded_state, sizeof(decoded_state), 0xa5);
  status = po32_state_raw_encode(NULL, encoded, sizeof(encoded), &written);
  assert(status == PO32_ERR_INVALID_ARG && written == 999);
  status = po32_state_raw_encode(&state, NULL, sizeof(encoded), &written);
  assert(status == PO32_ERR_INVALID_ARG && written == 999);
  status = po32_state_raw_encode(&state, encoded, 34, &written);
  assert(status == PO32_ERR_BUFFER_TOO_SMALL && written == 999);
  state.pattern_count = 22;
  status = po32_state_raw_encode(&state, encoded, 56, &written);
  assert(status == PO32_ERR_BUFFER_TOO_SMALL && written == 999);
  state.pattern_count = 221;
  status = po32_state_raw_encode(&state, encoded, sizeof(encoded), &written);
  assert(status == PO32_ERR_RANGE && written == 999);
  state.pattern_count = SIZE_MAX;
  status = po32_state_raw_encode(&state, encoded, sizeof(encoded), &written);
  assert(status == PO32_ERR_RANGE && written == 999);
  assert_filled(encoded, sizeof(encoded), 0xa5);

  status = po32_pattern_raw_decode(NULL, sizeof(captured_pattern), &decoded_pattern);
  assert(status == PO32_ERR_INVALID_ARG);
  status = po32_pattern_raw_decode(captured_pattern, sizeof(captured_pattern), NULL);
  assert(status == PO32_ERR_INVALID_ARG);
  const size_t bad_pattern_sizes[] = {0, 210, 212};
  for (size_t i = 0; i < sizeof(bad_pattern_sizes) / sizeof(bad_pattern_sizes[0]); ++i) {
    status = po32_pattern_raw_decode(bytes, bad_pattern_sizes[i], &decoded_pattern);
    assert(status == PO32_ERR_FRAME);
    assert_filled(&decoded_pattern, sizeof(decoded_pattern), 0xa5);
  }
  status = po32_pattern_raw_encode(NULL, encoded, sizeof(encoded));
  assert(status == PO32_ERR_INVALID_ARG);
  status = po32_pattern_raw_encode(&pattern, NULL, sizeof(encoded));
  assert(status == PO32_ERR_INVALID_ARG);
  status = po32_pattern_raw_encode(&pattern, encoded, 210);
  assert(status == PO32_ERR_BUFFER_TOO_SMALL);
  assert_filled(encoded, sizeof(encoded), 0xa5);
}

typedef struct parsed_records {
  const uint8_t *state;
  size_t state_bytes, count;
} parsed_records;

static int check_frame_packet(const po32_packet_t *packet, void *user) {
  parsed_records *records = user;
  if (records->count == 0) {
    po32_state_raw_t state;
    assert(packet->tag_code == PO32_TAG_STATE && packet->payload_len == records->state_bytes);
    assert(memcmp(packet->payload, records->state, records->state_bytes) == 0);
    assert(po32_state_raw_decode(packet->payload, packet->payload_len, &state) == PO32_OK);
    assert(state.pattern_count == PO32_CHAIN_MAX_COUNT);
  } else {
    assert(records->count == 1);
    assert(packet->tag_code == PO32_TAG_PATTERN && packet->payload_len == sizeof(captured_pattern));
    assert(memcmp(packet->payload, captured_pattern, sizeof(captured_pattern)) == 0);
  }
  ++records->count;
  return 0;
}

static void test_max_chain_modem_roundtrip(void) {
  uint8_t payload[PO32_MAX_PAYLOAD], frame[1024], decoded[1024];
  po32_state_raw_t state = {0};
  po32_builder_t builder;
  po32_final_tail_t tail;
  po32_decode_result_t result;
  po32_demodulator_t demodulator;
  size_t written = 0, frame_bytes = 0, decoded_bytes = 0;
  state.pattern_count = PO32_CHAIN_MAX_COUNT;
  state.tempo = 140;
  for (size_t i = 0; i < state.pattern_count; ++i)
    state.pattern_numbers[i] = (uint8_t)(i % 16);
  po32_status_t status = po32_state_raw_encode(&state, payload, sizeof(payload), &written);
  assert(status == PO32_OK && written == PO32_MAX_PAYLOAD);
  po32_builder_init(&builder, frame, sizeof(frame));
  status = po32_builder_append_packet(&builder, PO32_TAG_STATE, payload, written, NULL);
  assert(status == PO32_OK);
  status = po32_builder_append_packet(&builder, PO32_TAG_PATTERN, captured_pattern,
                                      sizeof(captured_pattern), NULL);
  assert(status == PO32_OK);
  status = po32_builder_finish(&builder, &frame_bytes);
  assert(status == PO32_OK);
  parsed_records records = {payload, written, 0};
  status = po32_frame_parse(frame, frame_bytes, check_frame_packet, &records, &tail);
  assert(status == PO32_OK && records.count == 2);

  const size_t sample_count = po32_render_sample_count(frame_bytes, 44100);
  float *samples = malloc(sample_count * sizeof(*samples));
  assert(samples != NULL);
  status = po32_render_dpsk_f32(frame, frame_bytes, 44100, samples, sample_count);
  assert(status == PO32_OK);
  status = po32_decode_f32(samples, sample_count, 44100.0f, &result, decoded, sizeof(decoded),
                           &decoded_bytes);
  assert(status == PO32_OK && result.done && result.packet_count == 2);
  assert(decoded_bytes == frame_bytes && memcmp(decoded, frame, frame_bytes) == 0);

  records.count = 0;
  po32_demodulator_init(&demodulator, 44100.0f);
  for (size_t offset = 0; offset < sample_count;) {
    const size_t remaining = sample_count - offset;
    const size_t count = remaining < 257 ? remaining : 257;
    status =
        po32_demodulator_push(&demodulator, samples + offset, count, check_frame_packet, &records);
    assert(status == PO32_OK);
    offset += count;
  }
  po32_demodulator_flush(&demodulator, check_frame_packet, &records);
  assert(po32_demodulator_done(&demodulator) && records.count == 2);
  free(samples);
}

int main(void) {
  test_captured_records();
  test_chain_boundaries();
  test_state_edit_locality();
  test_all_trigger_values_and_edit_locality();
  test_failures_leave_outputs_unchanged();
  test_max_chain_modem_roundtrip();
  puts("PO-32 lossless data tests passed");
  return 0;
}
