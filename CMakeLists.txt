cmake_minimum_required(VERSION 3.20)
project(AXEVoiceDesktop VERSION 0.71.0 LANGUAGES CXX)
if(NOT WIN32)
  message(FATAL_ERROR "This bootstrap currently supports Windows only")
endif()
add_library(axv_sip_address STATIC src/sip_address.cpp)
target_compile_features(axv_sip_address PUBLIC cxx_std_17)
add_library(axv_audio_devices STATIC src/audio_devices_win32.cpp)
target_compile_features(axv_audio_devices PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(axv_audio_devices PRIVATE /utf-8 /W4)
endif()
target_link_libraries(axv_audio_devices PUBLIC ole32 uuid propsys)
add_library(axv_ringtone_preview STATIC src/ringtone_preview_win32.cpp)
target_compile_features(axv_ringtone_preview PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(axv_ringtone_preview PRIVATE /utf-8 /W4)
endif()
target_link_libraries(axv_ringtone_preview PUBLIC ole32 uuid)
add_library(axv_incoming_ringtone_worker STATIC src/incoming_ringtone_worker.cpp)
target_compile_features(axv_incoming_ringtone_worker PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(axv_incoming_ringtone_worker PRIVATE /utf-8 /W4)
endif()
target_link_libraries(axv_incoming_ringtone_worker PUBLIC axv_ringtone_preview)
add_library(axv_windows_process STATIC src/windows_process.cpp)
target_compile_features(axv_windows_process PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(axv_windows_process PRIVATE /utf-8 /W4)
endif()
option(AXV_BUILD_NATIVE_BARESIP_ADAPTER
  "Compile isolated Baresip C API callback adapter (NOT linked into AXEVoice)" OFF)
if(AXV_BUILD_NATIVE_BARESIP_ADAPTER)
  set(AXV_BARESIP_INCLUDE_DIR "" CACHE PATH "Audited Baresip SDK include path")
  set(AXV_BARESIP_LIBRARY "" CACHE FILEPATH "Audited Baresip SDK library path")
  set(AXV_BARESIP_HEADER_SHA256 "" CACHE STRING "Approved SHA256 of baresip.h")
  set(AXV_BARESIP_LIBRARY_SHA256 "" CACHE STRING "Approved SHA256 of the Baresip SDK library")
  set(AXV_BARESIP_HEADER "${AXV_BARESIP_INCLUDE_DIR}/baresip.h")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/verify-baresip-sdk.cmake")
  add_library(axv_native_baresip_adapter STATIC src/baresip_sdk_event_adapter.cpp)
  target_include_directories(axv_native_baresip_adapter PRIVATE "${AXV_BARESIP_INCLUDE_DIR}")
  target_link_libraries(axv_native_baresip_adapter PUBLIC "${AXV_BARESIP_LIBRARY}")
  target_compile_features(axv_native_baresip_adapter PRIVATE cxx_std_17)
  # An executable forces resolution of native bevent symbols. A static library
  # by itself may build even when the SDK ABI or import libraries are wrong.
  # This artifact is BUILD-ONLY; never run against an uninitialized SDK.
  add_executable(axv_native_baresip_sdk_abi_probe
    tests/baresip_sdk_abi_probe.cpp)
  target_include_directories(axv_native_baresip_sdk_abi_probe PRIVATE
    "${AXV_BARESIP_INCLUDE_DIR}")
  target_compile_features(axv_native_baresip_sdk_abi_probe PRIVATE cxx_std_17)
  target_link_libraries(axv_native_baresip_sdk_abi_probe PRIVATE
    axv_native_baresip_adapter "${AXV_BARESIP_LIBRARY}")
endif()
add_executable(AXEVoice WIN32 src/main.cpp)
target_compile_features(AXEVoice PRIVATE cxx_std_17)
target_compile_definitions(AXEVoice PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
if(MSVC)
  target_compile_options(AXEVoice PRIVATE /utf-8 /W4)
endif()
target_link_libraries(AXEVoice PRIVATE user32 gdi32 advapi32 shell32 axv_sip_address axv_audio_devices axv_windows_process axv_ringtone_preview)
include(CTest)
if(BUILD_TESTING)
  # Native libre event-loop owner and pointer-free registration primitives
  # imported from the isolated SIP track. These tests use only fake SDK APIs;
  # AXEVoice.exe remains intentionally disconnected from native SIP/RTP.
  add_executable(test_libre_owner_reactor
    tests/test_libre_owner_reactor.cpp
    src/baresip_native_libre_reactor.cpp)
  target_include_directories(test_libre_owner_reactor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_libre_owner_reactor PRIVATE cxx_std_17)
  add_test(NAME libre_owner_reactor COMMAND test_libre_owner_reactor)
  set_tests_properties(libre_owner_reactor PROPERTIES TIMEOUT 15)

  # Native owner-thread integration via deterministic fake libre+Baresip API.
  # Not linked into AXEVoice.exe; no SIP credentials or network activity.
  add_executable(test_libre_registration_owner_integration
    tests/test_libre_registration_owner_integration.cpp
    src/baresip_native_libre_reactor.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_libre_registration_owner_integration PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_libre_registration_owner_integration PRIVATE cxx_std_17)
  add_test(NAME libre_registration_owner_integration
    COMMAND test_libre_registration_owner_integration)
  set_tests_properties(libre_registration_owner_integration PROPERTIES TIMEOUT 15)
  # Distinct integration scenario: private native UA is provisioned and
  # destroyed exactly once on the libre owner thread; tests rejection too.
  add_test(NAME libre_registration_owned_ua_lifecycle
    COMMAND test_libre_registration_owner_integration --owned-ua)
  set_tests_properties(libre_registration_owned_ua_lifecycle PROPERTIES TIMEOUT 15)
  # Real SDK function signatures, but deterministic fake implementations.
  # The production binary still links no native SIP account factory.
  add_test(NAME libre_registration_native_sdk_ua_factory
    COMMAND test_libre_registration_owner_integration --sdk-ua)
  set_tests_properties(libre_registration_native_sdk_ua_factory PROPERTIES TIMEOUT 15)
  # The fake audited SDK refuses UA allocation before core readiness, and
  # confirms shutdown order: unregister > ua_destroy > core.release.
  add_test(NAME libre_registration_core_bootstrap_gate
    COMMAND test_libre_registration_owner_integration --core-bootstrap)
  set_tests_properties(libre_registration_core_bootstrap_gate PROPERTIES TIMEOUT 15)

  add_executable(test_baresip_registration_command_queue
    tests/test_baresip_registration_command_queue.cpp)
  target_compile_features(test_baresip_registration_command_queue PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_command_queue
    COMMAND test_baresip_registration_command_queue)

  add_executable(test_baresip_registration_watchdog
    tests/test_baresip_registration_watchdog.cpp)
  target_compile_features(test_baresip_registration_watchdog PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_watchdog
    COMMAND test_baresip_registration_watchdog)

  add_executable(test_baresip_native_thread_guard
    tests/test_baresip_native_thread_guard.cpp)
  target_compile_features(test_baresip_native_thread_guard PRIVATE cxx_std_17)
  add_test(NAME baresip_native_thread_guard
    COMMAND test_baresip_native_thread_guard)
  add_executable(test_baresip_sdk_abi_probe
    tests/baresip_sdk_abi_probe.cpp
    tests/fakes/baresip_sdk_stub.cpp)
  target_include_directories(test_baresip_sdk_abi_probe PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_abi_probe PRIVATE cxx_std_17)
  target_compile_definitions(test_baresip_sdk_abi_probe PRIVATE
    AXV_FAKE_SDK_ABI_TEST)
  add_test(NAME baresip_sdk_abi_probe COMMAND test_baresip_sdk_abi_probe)
  add_test(NAME baresip_sdk_provenance
    COMMAND "${CMAKE_COMMAND}"
      "-DAXV_GATE_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cmake/verify-baresip-sdk.cmake"
      "-DAXV_FIXTURE_HEADER=${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes/baresip.h"
      "-DAXV_GATE_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/sdk-provenance-fixture"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_baresip_sdk_provenance.cmake")
  add_executable(test_ui_dpi_policy tests/test_ui_dpi_policy.cpp)
  target_compile_features(test_ui_dpi_policy PRIVATE cxx_std_17)
  add_test(NAME ui_dpi_policy COMMAND test_ui_dpi_policy)
  add_executable(test_ui_contrast_policy tests/test_ui_contrast_policy.cpp)
  target_compile_features(test_ui_contrast_policy PRIVATE cxx_std_17)
  add_test(NAME ui_contrast_policy COMMAND test_ui_contrast_policy)
  add_test(NAME ui_smoke COMMAND $<TARGET_FILE:AXEVoice> --ui-smoke)
  add_executable(test_ui_blackbox tests/test_ui_blackbox.cpp)
  target_compile_features(test_ui_blackbox PRIVATE cxx_std_17)
  target_compile_definitions(test_ui_blackbox PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(test_ui_blackbox PRIVATE user32)
  add_test(NAME ui_blackbox COMMAND $<TARGET_FILE:test_ui_blackbox> $<TARGET_FILE:AXEVoice>)
  set_tests_properties(ui_blackbox PROPERTIES TIMEOUT 35)
  add_executable(test_integration_sip_safety tests/test_integration_sip_safety.cpp)
  target_compile_features(test_integration_sip_safety PRIVATE cxx_std_17)
  target_link_libraries(test_integration_sip_safety PRIVATE axv_sip_address)
  add_test(NAME integration_sip_safety COMMAND test_integration_sip_safety)
  add_executable(test_call_readiness tests/test_call_readiness.cpp)
  target_compile_features(test_call_readiness PRIVATE cxx_std_17)
  add_test(NAME call_readiness COMMAND test_call_readiness)
  set_tests_properties(ui_smoke PROPERTIES TIMEOUT 10)
  add_executable(test_dialpad_model tests/test_dialpad_model.cpp)
  target_compile_features(test_dialpad_model PRIVATE cxx_std_17)
  add_test(NAME dialpad_model COMMAND test_dialpad_model)
  add_executable(test_sip_address tests/test_sip_address.cpp)
  target_link_libraries(test_sip_address PRIVATE axv_sip_address)
  add_test(NAME sip_address COMMAND test_sip_address)
  add_executable(test_ui_help tests/test_ui_help.cpp)
  target_compile_features(test_ui_help PRIVATE cxx_std_17)
  add_test(NAME ui_help COMMAND test_ui_help)
  add_executable(test_account_form_feedback tests/test_account_form_feedback.cpp)
  target_compile_features(test_account_form_feedback PRIVATE cxx_std_17)
  add_test(NAME account_form_feedback COMMAND test_account_form_feedback)
  add_executable(test_destination_feedback tests/test_destination_feedback.cpp)
  target_compile_features(test_destination_feedback PRIVATE cxx_std_17)
  target_link_libraries(test_destination_feedback PRIVATE axv_sip_address)
  add_test(NAME destination_feedback COMMAND test_destination_feedback)
  add_executable(test_sip_runtime_observation tests/test_sip_runtime_observation.cpp)
  target_compile_features(test_sip_runtime_observation PRIVATE cxx_std_17)
  add_test(NAME sip_runtime_observation COMMAND test_sip_runtime_observation)
  add_executable(test_baresip_event_mailbox tests/test_baresip_event_mailbox.cpp)
  target_compile_features(test_baresip_event_mailbox PRIVATE cxx_std_17)
  add_test(NAME baresip_event_mailbox COMMAND test_baresip_event_mailbox)
  add_executable(test_sip_event_projection tests/test_sip_event_projection.cpp)
  target_compile_features(test_sip_event_projection PRIVATE cxx_std_17)
  add_test(NAME sip_event_projection COMMAND test_sip_event_projection)
  add_executable(test_sip_runtime_session_guard tests/test_sip_runtime_session_guard.cpp)
  target_compile_features(test_sip_runtime_session_guard PRIVATE cxx_std_17)
  add_test(NAME sip_runtime_session_guard COMMAND test_sip_runtime_session_guard)
  add_executable(test_incoming_ringtone_gate tests/test_incoming_ringtone_gate.cpp)
  target_compile_features(test_incoming_ringtone_gate PRIVATE cxx_std_17)
  add_test(NAME incoming_ringtone_gate COMMAND test_incoming_ringtone_gate)
  add_executable(test_integration_incoming_ringtone tests/test_integration_incoming_ringtone.cpp)
  target_compile_features(test_integration_incoming_ringtone PRIVATE cxx_std_17)
  add_test(NAME integration_incoming_ringtone COMMAND test_integration_incoming_ringtone)
  add_executable(test_incoming_ringtone_coordinator tests/test_incoming_ringtone_coordinator.cpp)
  target_compile_features(test_incoming_ringtone_coordinator PRIVATE cxx_std_17)
  add_test(NAME incoming_ringtone_coordinator COMMAND test_incoming_ringtone_coordinator)
  add_executable(test_integration_ringtone_playback_control tests/test_integration_ringtone_playback_control.cpp)
  target_compile_features(test_integration_ringtone_playback_control PRIVATE cxx_std_17)
  add_test(NAME integration_ringtone_playback_control COMMAND test_integration_ringtone_playback_control)
  add_executable(test_cancellable_ringtone_worker tests/test_cancellable_ringtone_worker.cpp)
  target_compile_features(test_cancellable_ringtone_worker PRIVATE cxx_std_17)
  target_link_libraries(test_cancellable_ringtone_worker PRIVATE axv_incoming_ringtone_worker)
  add_test(NAME cancellable_ringtone_worker COMMAND test_cancellable_ringtone_worker)
  add_executable(test_integration_ringtone_worker tests/test_integration_ringtone_worker.cpp)
  target_compile_features(test_integration_ringtone_worker PRIVATE cxx_std_17)
  target_link_libraries(test_integration_ringtone_worker PRIVATE axv_incoming_ringtone_worker)
  add_test(NAME integration_ringtone_worker COMMAND test_integration_ringtone_worker)
  add_executable(test_cooperative_ringtone_cancel tests/test_cooperative_ringtone_cancel.cpp)
  target_compile_features(test_cooperative_ringtone_cancel PRIVATE cxx_std_17)
  target_link_libraries(test_cooperative_ringtone_cancel PRIVATE axv_incoming_ringtone_worker)
  add_test(NAME cooperative_ringtone_cancel COMMAND test_cooperative_ringtone_cancel)
  add_executable(test_ring_repetition_watchdog tests/test_ring_repetition_watchdog.cpp)
  target_compile_features(test_ring_repetition_watchdog PRIVATE cxx_std_17)
  target_link_libraries(test_ring_repetition_watchdog PRIVATE axv_incoming_ringtone_worker)
  add_test(NAME ring_repetition_watchdog COMMAND test_ring_repetition_watchdog)
  add_executable(test_integration_ring_fencing_matrix tests/test_integration_ring_fencing_matrix.cpp)
  target_compile_features(test_integration_ring_fencing_matrix PRIVATE cxx_std_17)
  add_test(NAME integration_ring_fencing_matrix COMMAND test_integration_ring_fencing_matrix)
  # Produces the SHA-256 manifest of the exact LAB binary validated by this run.
  add_test(NAME lab_artifact_manifest COMMAND "${CMAKE_COMMAND}"
    "-DAXV_EXE=$<TARGET_FILE:AXEVoice>"
    "-DAXV_OUTPUT=$<TARGET_FILE_DIR:AXEVoice>/AXE-Voice-LAB.sha256"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/verify-lab-artifact.cmake")
  add_test(NAME lab_artifact_hash_verifier COMMAND pwsh
    -NoProfile -NonInteractive -File
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-lab-hash-verifier.ps1"
    -SourceDir "$<TARGET_FILE_DIR:AXEVoice>"
    -Verifier "$<TARGET_FILE_DIR:AXEVoice>/verify-axe-voice-lab.ps1")
  set_tests_properties(lab_artifact_hash_verifier PROPERTIES DEPENDS lab_artifact_manifest)
  add_test(NAME lab_artifact_manifest_negative COMMAND "${CMAKE_COMMAND}"
    "-DAXV_NEGATIVE_DIR=$<TARGET_FILE_DIR:AXEVoice>/manifest-negative-test"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-lab-artifact-negative.cmake")
  add_executable(test_dial_authorization tests/test_dial_authorization.cpp)
  target_compile_features(test_dial_authorization PRIVATE cxx_std_17)
  target_link_libraries(test_dial_authorization PRIVATE axv_sip_address)
  add_test(NAME dial_authorization COMMAND test_dial_authorization)
  add_executable(test_call_command_authorization tests/test_call_command_authorization.cpp)
  target_compile_features(test_call_command_authorization PRIVATE cxx_std_17)
  target_link_libraries(test_call_command_authorization PRIVATE axv_sip_address)
  add_test(NAME call_command_authorization COMMAND test_call_command_authorization)
  add_executable(test_sip_command_executor tests/test_sip_command_executor.cpp)
  target_compile_features(test_sip_command_executor PRIVATE cxx_std_17)
  target_link_libraries(test_sip_command_executor PRIVATE axv_sip_address)
  add_test(NAME sip_command_executor COMMAND test_sip_command_executor)
  add_executable(test_baresip_event_reducer tests/test_baresip_event_reducer.cpp)
  target_compile_features(test_baresip_event_reducer PRIVATE cxx_std_17)
  add_test(NAME baresip_event_reducer COMMAND test_baresip_event_reducer)
  add_executable(test_baresip_call_correlation tests/test_baresip_call_correlation.cpp)
  target_compile_features(test_baresip_call_correlation PRIVATE cxx_std_17)
  add_test(NAME baresip_call_correlation COMMAND test_baresip_call_correlation)
  add_executable(test_baresip_callback_ingress tests/test_baresip_callback_ingress.cpp)
  target_compile_features(test_baresip_callback_ingress PRIVATE cxx_std_17)
  add_test(NAME baresip_callback_ingress COMMAND test_baresip_callback_ingress)
  add_executable(test_baresip_callback_lifecycle tests/test_baresip_callback_lifecycle.cpp)
  target_compile_features(test_baresip_callback_lifecycle PRIVATE cxx_std_17)
  add_test(NAME baresip_callback_lifecycle COMMAND test_baresip_callback_lifecycle)
  add_executable(test_baresip_sdk_event_adapter
    tests/test_baresip_sdk_event_adapter.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_event_adapter PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_event_adapter PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_event_adapter COMMAND test_baresip_sdk_event_adapter)
  add_executable(test_baresip_sdk_single_owner
    tests/test_baresip_sdk_single_owner.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_single_owner PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_single_owner PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_single_owner COMMAND test_baresip_sdk_single_owner)
  add_executable(test_baresip_sdk_epochs tests/test_baresip_sdk_epochs.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_epochs PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_epochs PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_epochs COMMAND test_baresip_sdk_epochs)
  add_executable(test_baresip_sdk_callback_quiescence
    tests/test_baresip_sdk_callback_quiescence.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_callback_quiescence PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_callback_quiescence PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_callback_quiescence
    COMMAND test_baresip_sdk_callback_quiescence)
  set_tests_properties(baresip_sdk_callback_quiescence PROPERTIES TIMEOUT 15)
  add_executable(test_sip_engine tests/test_sip_engine.cpp)
  target_compile_features(test_sip_engine PRIVATE cxx_std_17)
  add_test(NAME sip_engine COMMAND test_sip_engine)
  add_executable(test_sip_account tests/test_sip_account.cpp)
  target_compile_features(test_sip_account PRIVATE cxx_std_17)
  add_test(NAME sip_account COMMAND test_sip_account)
  add_executable(test_sip_policy tests/test_sip_policy.cpp)
  target_compile_features(test_sip_policy PRIVATE cxx_std_17)
  add_test(NAME sip_policy COMMAND test_sip_policy)
  add_executable(test_sip_guardrails tests/test_sip_guardrails.cpp)
  target_compile_features(test_sip_guardrails PRIVATE cxx_std_17)
  add_test(NAME sip_guardrails COMMAND test_sip_guardrails)
  add_executable(test_sip_factory tests/test_sip_factory.cpp)
  target_compile_features(test_sip_factory PRIVATE cxx_std_17)
  add_test(NAME sip_factory COMMAND test_sip_factory)
  add_executable(test_baresip_probe tests/test_baresip_probe.cpp)
  target_compile_features(test_baresip_probe PRIVATE cxx_std_17)
  add_test(NAME baresip_probe COMMAND test_baresip_probe)
  add_executable(test_engine_lifecycle tests/test_engine_lifecycle.cpp)
  target_compile_features(test_engine_lifecycle PRIVATE cxx_std_17)
  add_test(NAME engine_lifecycle COMMAND test_engine_lifecycle)
  add_executable(test_audio_inventory tests/test_audio_inventory.cpp)
  target_compile_features(test_audio_inventory PRIVATE cxx_std_17)
  if(MSVC)
    target_compile_options(test_audio_inventory PRIVATE /utf-8 /W4)
  endif()
  add_test(NAME audio_inventory COMMAND test_audio_inventory)
  add_executable(test_audio_selection_feedback tests/test_audio_selection_feedback.cpp)
  target_compile_features(test_audio_selection_feedback PRIVATE cxx_std_17)
  add_test(NAME audio_selection_feedback COMMAND test_audio_selection_feedback)
  add_executable(test_audio_routing_preview tests/test_audio_routing_preview.cpp)
  target_compile_features(test_audio_routing_preview PRIVATE cxx_std_17)
  add_test(NAME audio_routing_preview COMMAND test_audio_routing_preview)
  add_executable(test_audio_role_swap tests/test_audio_role_swap.cpp)
  target_compile_features(test_audio_role_swap PRIVATE cxx_std_17)
  add_test(NAME audio_role_swap COMMAND test_audio_role_swap)
  add_executable(test_audio_route_change_fence tests/test_audio_route_change_fence.cpp)
  target_compile_features(test_audio_route_change_fence PRIVATE cxx_std_17)
  add_test(NAME audio_route_change_fence COMMAND test_audio_route_change_fence)
  add_executable(test_ringtone_preview tests/test_ringtone_preview.cpp)
  target_compile_features(test_ringtone_preview PRIVATE cxx_std_17)
  target_link_libraries(test_ringtone_preview PRIVATE axv_ringtone_preview)
  add_test(NAME ringtone_preview COMMAND test_ringtone_preview)
  add_executable(test_audio_preview_target tests/test_audio_preview_target.cpp)
  target_compile_features(test_audio_preview_target PRIVATE cxx_std_17)
  add_test(NAME audio_preview_target COMMAND test_audio_preview_target)
  add_executable(test_audio_preview_cancel_policy tests/test_audio_preview_cancel_policy.cpp)
  target_compile_features(test_audio_preview_cancel_policy PRIVATE cxx_std_17)
  add_test(NAME audio_preview_cancel_policy COMMAND test_audio_preview_cancel_policy)
  add_executable(test_audio_preview_levels tests/test_audio_preview_levels.cpp)
  target_compile_features(test_audio_preview_levels PRIVATE cxx_std_17)
  add_test(NAME audio_preview_levels COMMAND test_audio_preview_levels)
  add_executable(test_baresip_modules tests/test_baresip_modules.cpp)
  target_compile_features(test_baresip_modules PRIVATE cxx_std_17)
  add_test(NAME baresip_modules COMMAND test_baresip_modules)
  add_executable(test_baresip_profile tests/test_baresip_profile.cpp)
  target_compile_features(test_baresip_profile PRIVATE cxx_std_17)
  add_test(NAME baresip_profile COMMAND test_baresip_profile)
  add_executable(axv_test_child_process tests/child_process.cpp)
  target_compile_features(axv_test_child_process PRIVATE cxx_std_17)
  add_executable(axv_test_interactive_process tests/interactive_process.cpp)
  add_executable(axv_test_exit_process tests/exit_process.cpp)
  add_executable(test_shutdown_policy tests/test_shutdown_policy.cpp)
  target_compile_features(test_shutdown_policy PRIVATE cxx_std_17)
  add_test(NAME shutdown_policy COMMAND test_shutdown_policy)
  add_executable(test_windows_process tests/test_windows_process.cpp)
  target_compile_features(test_windows_process PRIVATE cxx_std_17)
  if(MSVC)
    target_compile_options(test_windows_process PRIVATE /utf-8 /W4)
  endif()
  target_link_libraries(test_windows_process PRIVATE axv_windows_process)
  add_test(NAME windows_process COMMAND test_windows_process $<TARGET_FILE:axv_test_child_process> $<TARGET_FILE:axv_test_exit_process> $<TARGET_FILE:axv_test_interactive_process>)
endif()
# Each test belongs to exactly one quality gate. Fail the CI on empty tiers.
get_property(axv_registered_tests DIRECTORY PROPERTY TESTS)
foreach(axv_test IN LISTS axv_registered_tests)
  set_tests_properties("${axv_test}" PROPERTIES LABELS "unit")
endforeach()
set_tests_properties(ui_smoke ui_blackbox PROPERTIES LABELS "blackbox")
set_tests_properties(integration_sip_safety baresip_sdk_abi_probe windows_process sip_runtime_session_guard
  baresip_sdk_provenance
  sip_event_projection baresip_event_mailbox baresip_callback_ingress
  baresip_callback_lifecycle baresip_sdk_event_adapter baresip_sdk_single_owner baresip_sdk_epochs
  baresip_sdk_callback_quiescence
  call_command_authorization sip_command_executor integration_incoming_ringtone
  integration_ringtone_playback_control integration_ringtone_worker integration_ring_fencing_matrix
  lab_artifact_manifest lab_artifact_manifest_negative
  lab_artifact_hash_verifier libre_owner_reactor
  libre_registration_owner_integration
  libre_registration_owned_ua_lifecycle
  libre_registration_native_sdk_ua_factory
  libre_registration_core_bootstrap_gate PROPERTIES LABELS "integration")
set_target_properties(AXEVoice PROPERTIES OUTPUT_NAME "AXE-Voice")
add_custom_command(TARGET AXEVoice POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify-axe-voice-lab.ps1"
    "$<TARGET_FILE_DIR:AXEVoice>/verify-axe-voice-lab.ps1"
  COMMENT "Copy LAB integrity verifier beside AXE-Voice.exe")
