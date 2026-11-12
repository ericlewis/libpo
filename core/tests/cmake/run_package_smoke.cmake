# Install, relocate, and consume both package names with the configured C compiler.
foreach(name IN ITEMS PO_BUILD_DIR PO_C_COMPILER)
  if(NOT DEFINED ${name} OR "${${name}}" STREQUAL "")
    message(FATAL_ERROR "${name} is required")
  endif()
endforeach()
if(NOT EXISTS "${PO_BUILD_DIR}/CMakeCache.txt")
  message(FATAL_ERROR "PO_BUILD_DIR must be a configured build tree")
endif()
set(work "${PO_BUILD_DIR}/package-smoke")
if(DEFINED PO_INSTALL_LIBDIR)
  set(work "${PO_BUILD_DIR}/package-smoke-multiarch")
endif()
if(IS_SYMLINK "${work}")
  message(FATAL_ERROR "Refusing a symlink for the package test workspace")
endif()
if(EXISTS "${work}")
  if(NOT EXISTS "${work}/.libpo-test-workspace")
    message(FATAL_ERROR "Refusing to replace an unowned package test workspace")
  endif()
  file(REMOVE_RECURSE "${work}")
endif()
file(MAKE_DIRECTORY "${work}")
file(WRITE "${work}/.libpo-test-workspace" "libpo package smoke test\n")
function(run_checked)
  execute_process(COMMAND ${ARGV} RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 60)
  if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "Package smoke command failed (${result}): ${ARGV}\n${output}\n${error}")
  endif()
endfunction()
if(DEFINED PO_INSTALL_LIBDIR)
  set(PO_BUILD_DIR "${work}/build")
  run_checked("${CMAKE_COMMAND}" -S "${PO_SOURCE_DIR}" -B "${PO_BUILD_DIR}"
    "-DCMAKE_C_COMPILER=${PO_C_COMPILER}" "-DCMAKE_INSTALL_LIBDIR=${PO_INSTALL_LIBDIR}"
    -DCMAKE_BUILD_TYPE=Release)
  run_checked("${CMAKE_COMMAND}" --build "${PO_BUILD_DIR}" --target po32 --config Release)
endif()
set(config_args)
if(DEFINED PO_CONFIG AND NOT "${PO_CONFIG}" STREQUAL "")
  list(APPEND config_args --config "${PO_CONFIG}")
endif()
run_checked("${CMAKE_COMMAND}" --install "${PO_BUILD_DIR}" --prefix "${work}/installed" ${config_args})
file(RENAME "${work}/installed" "${work}/relocated")
file(MAKE_DIRECTORY "${work}/consumer")
file(WRITE "${work}/consumer/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.16)
project(libpo_package_consumer LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
find_package(LibPO CONFIG REQUIRED)
find_package(LibPO32 CONFIG REQUIRED)
add_executable(current consumer.c)
target_link_libraries(current PRIVATE LibPO::po)
add_executable(legacy consumer.c)
target_link_libraries(legacy PRIVATE LibPO32::po32)
set(consumers current legacy)
if(PKG_CONFIG_EXECUTABLE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(PO REQUIRED IMPORTED_TARGET po)
  pkg_check_modules(PO32 REQUIRED IMPORTED_TARGET po32)
  add_executable(pkg_current consumer.c)
  target_link_libraries(pkg_current PRIVATE PkgConfig::PO)
  add_executable(pkg_legacy consumer.c)
  target_link_libraries(pkg_legacy PRIVATE PkgConfig::PO32)
  list(APPEND consumers pkg_current pkg_legacy)
endif()
set_target_properties(${consumers} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
]=])
file(WRITE "${work}/consumer/consumer.c" [=[
#include <po.h>
static const po35_table_1c7f_t replacement = {
  255, {255, 0, 128, 127, 1, 2, 3, 4, 5, 6, 7, 8}
};
static int check_table_packet(const po_packet_t *packet, void *user) {
  unsigned *seen = (unsigned *)user;
  if (packet->tag_code == PO35_TAG_TABLE_1C7F) {
    po35_table_1c7f_t row;
    if ((*seen & 1u) || po35_table_1c7f_decode(packet->payload, packet->payload_len, &row) != PO_OK ||
        row.index != replacement.index) return 1;
    for (size_t i = 0; i < sizeof(row.data_raw); ++i)
      if (row.data_raw[i] != replacement.data_raw[i]) return 1;
    *seen |= 1u;
    return 0;
  }
  if ((*seen & 2u) || packet->tag_code != 0x1234 || packet->payload_len != 3 ||
      packet->payload[0] != 9 || packet->payload[1] != 8 || packet->payload[2] != 7) return 1;
  *seen |= 2u;
  return 0;
}
int main(void) {
  enum po32_status legacy_status = PO32_OK;
  struct po32_packet legacy_packet = {0};
  unsigned char frame[PO35_PREAMBLE_BYTES + 64];
  unsigned char workspace[sizeof(frame)], output[sizeof(frame)];
  unsigned char table_payload[PO35_TABLE_1C7F_BYTES];
  const unsigned char unknown_payload[] = {9, 8, 7};
  const po35_table_1c7f_t original = {255, {0}};
  po35_builder_t builder;
  po35_decode_result_t decoded;
  po35_synth_t synth;
  po35_synth_voice_t voice;
  po32_patch_params_t fx_patch = {0};
  size_t length;
  size_t analysis_bytes;
  size_t sample_count;
  size_t written = 0;
  unsigned seen_packets = 0;
  if (legacy_status != PO_OK || legacy_packet.payload_len != 0) return 1;
  if (po35_builder_init(&builder, frame, sizeof(frame)) != PO_OK) return 2;
  if (po35_builder_finish(&builder, &length) != PO_OK) return 3;
  if (po35_frame_parse(frame, length, 0, 0, &decoded) != PO_OK || !decoded.done) return 4;
  if (po35_synth_init(&synth, PO35_SYNTH_BANK_HIGH, 350) != PO_OK) return 5;
  if (po35_synth_voice_init(&voice, &synth, 0, 0, 80) != PO_OK || !po35_synth_voice_done(&voice)) return 6;
  if (po35_analysis_size_for_pcm(48000, 48000, 80, &analysis_bytes) != PO_OK || analysis_bytes != 80 * 44) return 8;
  if (po33_sample_count_for_pcm(48000, 48000, &sample_count) != PO_OK || sample_count != 23438) return 9;
  if (po32_fx_apply_patch(&fx_patch, PO32_FX_PATCH_FM, &fx_patch) != PO_OK || fx_patch.ModRate != 0.75f) return 10;
  if (po32_fx_pitch_patch(&fx_patch, 0.125f, &fx_patch) != PO_OK || fx_patch.OscFreq != 0.125f) return 11;
  if (po32_fx_eq_sweep_patch(&fx_patch, 0.5f, &fx_patch) != PO_OK || fx_patch.EQGain != 1.0f) return 12;
  if (po35_table_1c7f_encode(&original, table_payload, sizeof(table_payload)) != PO_OK) return 13;
  if (po35_builder_init(&builder, frame, sizeof(frame)) != PO_OK) return 14;
  if (po35_builder_append_packet(&builder, PO35_TAG_TABLE_1C7F,
      table_payload, sizeof(table_payload), 0) != PO_OK) return 15;
  if (po35_builder_append_packet(&builder, 0x1234,
      unknown_payload, sizeof(unknown_payload), 0) != PO_OK) return 16;
  if (po35_builder_finish(&builder, &length) != PO_OK) return 17;
  if (po35_table_1c7f_replace(frame, length, &replacement, workspace, sizeof(workspace),
      output, sizeof(output), &written) != PO_OK || written != length) return 18;
  if (po35_frame_parse(output, written, check_table_packet, &seen_packets, &decoded) != PO_OK ||
      !decoded.done || decoded.stopped || decoded.frame_bytes != written ||
      decoded.packet_count != 2 || seen_packets != 3) return 19;
  return po33_sample_decode(0) == 0 ? 0 : 7;
}
]=])
set(consumer_args)
if(DEFINED PO_INSTALL_LIBDIR)
  # A foreign multiarch directory need not be in this host's CMake search paths.
  list(APPEND consumer_args
    "-DLibPO_DIR=${work}/relocated/${PO_INSTALL_LIBDIR}/cmake/LibPO"
    "-DLibPO32_DIR=${work}/relocated/${PO_INSTALL_LIBDIR}/cmake/LibPO32")
endif()
if(PO_PKG_CONFIG)
  file(GLOB_RECURSE pc_files "${work}/relocated/po.pc")
  list(LENGTH pc_files pc_count)
  if(NOT pc_count EQUAL 1)
    message(FATAL_ERROR "Expected one installed po.pc, found ${pc_count}")
  endif()
  list(GET pc_files 0 pc_file)
  get_filename_component(pc_dir "${pc_file}" DIRECTORY)
  set(ENV{PKG_CONFIG_PATH} "")
  set(ENV{PKG_CONFIG_LIBDIR} "${pc_dir}")
  set(ENV{PKG_CONFIG_SYSROOT_DIR} "")
  list(APPEND consumer_args "-DPKG_CONFIG_EXECUTABLE=${PO_PKG_CONFIG}")
endif()
run_checked("${CMAKE_COMMAND}" -S "${work}/consumer" -B "${work}/consumer-build"
  "-DCMAKE_C_COMPILER=${PO_C_COMPILER}" "-DCMAKE_PREFIX_PATH=${work}/relocated"
  -DCMAKE_BUILD_TYPE=Release ${consumer_args})
run_checked("${CMAKE_COMMAND}" --build "${work}/consumer-build" --config Release)
run_checked("${work}/consumer-build/bin/current")
run_checked("${work}/consumer-build/bin/legacy")
if(PO_PKG_CONFIG)
  run_checked("${work}/consumer-build/bin/pkg_current")
  run_checked("${work}/consumer-build/bin/pkg_legacy")
endif()
