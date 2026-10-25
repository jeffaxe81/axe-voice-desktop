cmake_minimum_required(VERSION 3.20)
project(AXEVoiceDesktop VERSION 0.51.0 LANGUAGES CXX)
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
add_library(axv_audio_test_tone STATIC src/audio_test_tone_win32.cpp)
target_compile_features(axv_audio_test_tone PUBLIC cxx_std_17)
target_link_libraries(axv_audio_test_tone PUBLIC ole32 uuid)
if(MSVC)
  target_compile_options(axv_audio_test_tone PRIVATE /utf-8 /W4)
endif()
add_library(axv_windows_process STATIC src/windows_process.cpp)
target_compile_features(axv_windows_process PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(axv_windows_process PRIVATE /utf-8 /W4)
endif()
option(AXV_ENABLE_NATIVE_SIP "Build the native SIP release candidate GUI" OFF)
if(AXV_ENABLE_NATIVE_SIP AND NOT AXV_BUILD_NATIVE_BARESIP_ADAPTER)
  message(FATAL_ERROR "Native SIP GUI requires the audited native SDK adapter")
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
  set(AXV_BARESIP_LIBRARY "" CACHE FILEPATH "Audited Baresip SDK MSVC import library")
  set(AXV_BARESIP_HEADER_SHA256 "" CACHE STRING "Audited baresip.h SHA-256")
  set(AXV_BARESIP_LIBRARY_SHA256 "" CACHE STRING "Audited Baresip .lib SHA-256")
  set(AXV_BARESIP_VERSION "" CACHE STRING "Audited BARESIP_VERSION macro value")
  set(AXV_LIBRE_INCLUDE_DIR "" CACHE PATH "Audited libre/re include path")
  set(AXV_LIBRE_LIBRARY "" CACHE FILEPATH "Audited libre/re MSVC import library")
  set(AXV_LIBRE_HEADER_SHA256 "" CACHE STRING "Audited re.h SHA-256")
  set(AXV_LIBRE_LIBRARY_SHA256 "" CACHE STRING "Audited libre/re .lib SHA-256")
  set(AXV_REPOSITORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/axv_verify_baresip_sdk.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/axv_verify_libre_sdk.cmake")

  # SDK remains entirely optional and is NEVER linked into AXEVoice GUI.
  add_library(axv_native_baresip_adapter STATIC src/baresip_sdk_event_adapter.cpp)
  target_include_directories(axv_native_baresip_adapter PRIVATE
    "${AXV_LIBRE_INCLUDE_DIR}" "${AXV_BARESIP_INCLUDE_DIR}")
  target_link_libraries(axv_native_baresip_adapter PUBLIC
    "${AXV_BARESIP_LIBRARY}" "${AXV_LIBRE_LIBRARY}")
  target_compile_features(axv_native_baresip_adapter PRIVATE cxx_std_17)
  # An executable forces resolution of native bevent symbols. A static library
  # by itself may build even when the SDK ABI or import libraries are wrong.
  # This artifact is BUILD-ONLY; never run against an uninitialized SDK.
  add_executable(axv_native_baresip_sdk_abi_probe
    tests/baresip_sdk_abi_probe.cpp)
  target_include_directories(axv_native_baresip_sdk_abi_probe PRIVATE
    "${AXV_LIBRE_INCLUDE_DIR}" "${AXV_BARESIP_INCLUDE_DIR}")
  target_compile_features(axv_native_baresip_sdk_abi_probe PRIVATE cxx_std_17)
  target_link_libraries(axv_native_baresip_sdk_abi_probe PRIVATE
    axv_native_baresip_adapter "${AXV_BARESIP_LIBRARY}")

  # Unlike a static library alone, an EXE link probe must resolve the actual
  # SDK symbols. Builds fail on ABI/header/import-library mismatches.
  add_executable(axv_native_baresip_abi_probe src/baresip_sdk_abi_probe.cpp)
  target_include_directories(axv_native_baresip_abi_probe PRIVATE
    "${AXV_LIBRE_INCLUDE_DIR}" "${AXV_BARESIP_INCLUDE_DIR}")
  target_compile_definitions(axv_native_baresip_abi_probe PRIVATE
    AXV_BARESIP_AUDITED_VERSION="${AXV_BARESIP_VERSION}")
  target_link_libraries(axv_native_baresip_abi_probe PRIVATE
    "${AXV_BARESIP_LIBRARY}" "${AXV_LIBRE_LIBRARY}")
  target_compile_features(axv_native_baresip_abi_probe PRIVATE cxx_std_17)
  # The SDK-enabled test exercises the REAL libre re_main/mqueue/timer
  # without creating a Baresip UA, loading account secrets or sending SIP.
  add_executable(axv_native_libre_reactor_smoke
    tests/native_libre_reactor_smoke.cpp
    src/baresip_native_libre_reactor.cpp)
  target_include_directories(axv_native_libre_reactor_smoke PRIVATE
    "${AXV_LIBRE_INCLUDE_DIR}")
  target_link_libraries(axv_native_libre_reactor_smoke PRIVATE
    "${AXV_LIBRE_LIBRARY}")
  target_compile_features(axv_native_libre_reactor_smoke PRIVATE cxx_std_17)
  option(AXV_NATIVE_TLS_MEDIA "Validate TLS/G711/WASAPI candidate without enabling GUI SIP" OFF)
  set(_native_sdk_targets axv_native_baresip_adapter
    axv_native_baresip_sdk_abi_probe axv_native_baresip_abi_probe
    axv_native_libre_reactor_smoke)
  if(AXV_NATIVE_TLS_MEDIA)
    find_package(OpenSSL 3.0 REQUIRED)
    add_executable(axv_native_tls_media_probe tests/native_tls_media_probe.cpp)
    target_include_directories(axv_native_tls_media_probe PRIVATE
      "${AXV_LIBRE_INCLUDE_DIR}" "${AXV_BARESIP_INCLUDE_DIR}")
    target_compile_features(axv_native_tls_media_probe PRIVATE cxx_std_17)
    target_link_libraries(axv_native_tls_media_probe PRIVATE
      "${AXV_BARESIP_LIBRARY}" "${AXV_LIBRE_LIBRARY}")
    add_library(axv_native_sip STATIC src/baresip_sip_engine.cpp src/baresip_native_libre_reactor.cpp)
    target_include_directories(axv_native_sip PRIVATE "${AXV_LIBRE_INCLUDE_DIR}" "${AXV_BARESIP_INCLUDE_DIR}")
    target_compile_features(axv_native_sip PRIVATE cxx_std_17)
    target_link_libraries(axv_native_sip PUBLIC "${AXV_BARESIP_LIBRARY}" "${AXV_LIBRE_LIBRARY}" axv_sip_address)
    add_executable(axv_native_sip_engine_smoke tests/native_sip_engine_smoke.cpp)
    target_link_libraries(axv_native_sip_engine_smoke PRIVATE axv_native_sip)
    add_executable(axv_native_sdk_secret_smoke tests/native_sdk_secret_smoke.cpp)
    target_include_directories(axv_native_sdk_secret_smoke PRIVATE "${AXV_LIBRE_INCLUDE_DIR}" "${AXV_BARESIP_INCLUDE_DIR}")
    target_link_libraries(axv_native_sdk_secret_smoke PRIVATE "${AXV_BARESIP_LIBRARY}" "${AXV_LIBRE_LIBRARY}")
    target_compile_features(axv_native_sdk_secret_smoke PRIVATE cxx_std_17)
    list(APPEND _native_sdk_targets axv_native_sdk_secret_smoke axv_native_tls_media_probe axv_native_sip axv_native_sip_engine_smoke)
    foreach(_native_target IN LISTS _native_sdk_targets)
      target_compile_definitions(${_native_target} PRIVATE USE_TLS USE_OPENSSL)
      target_link_libraries(${_native_target} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endforeach()
  endif()
  # libre headers select Winsock declarations using WIN32 (not just _WIN32).
  # Keep this usage requirement scoped to consumers of the audited native SDK.
  foreach(_native_target IN LISTS _native_sdk_targets)
    target_compile_definitions(${_native_target} PRIVATE WIN32 NOMINMAX WIN32_LEAN_AND_MEAN)
    # Required by the static Windows libre profile; raw .lib paths do not
    # propagate its upstream RE_LIBS interface automatically.
    target_link_libraries(${_native_target} PRIVATE
      qwave iphlpapi wsock32 ws2_32 dbghelp
      winmm gdi32 crypt32 strmiids ole32 oleaut32)
  endforeach()
endif()
add_executable(AXEVoice WIN32 src/main.cpp)
target_compile_features(AXEVoice PRIVATE cxx_std_17)
target_compile_definitions(AXEVoice PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX WINVER=0x0A00 _WIN32_WINNT=0x0A00)
if(MSVC)
  target_compile_options(AXEVoice PRIVATE /utf-8 /W4)
endif()
target_link_libraries(AXEVoice PRIVATE user32 gdi32 advapi32 shell32 axv_sip_address axv_audio_devices axv_audio_test_tone axv_windows_process)
if(AXV_ENABLE_NATIVE_SIP)
  if(NOT AXV_NATIVE_TLS_MEDIA)
    message(FATAL_ERROR "Native SIP candidate requires TLS/media SDK profile")
  endif()
  target_compile_definitions(AXEVoice PRIVATE AXV_ENABLE_NATIVE_SIP)
  target_link_libraries(AXEVoice PRIVATE axv_native_sip)
endif()
include(CTest)
if(BUILD_TESTING)
  add_executable(test_native_ui_help tests/test_native_ui_help.cpp)
  target_compile_features(test_native_ui_help PRIVATE cxx_std_17)
  add_test(NAME native_ui_help COMMAND test_native_ui_help)
  set_tests_properties(native_ui_help PROPERTIES LABELS "unit")
  add_executable(test_native_session_control tests/test_native_session_control.cpp)
  target_compile_features(test_native_session_control PRIVATE cxx_std_17)
  add_test(NAME native_session_control COMMAND test_native_session_control)
  set_tests_properties(native_session_control PROPERTIES LABELS "unit")
  add_executable(test_native_account_config tests/test_native_account_config.cpp)
  target_link_libraries(test_native_account_config PRIVATE axv_sip_address)
  add_test(NAME native_account_config COMMAND test_native_account_config)
  set_tests_properties(native_account_config PROPERTIES LABELS "unit")
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
  if(AXV_BUILD_NATIVE_BARESIP_ADAPTER)
    add_test(NAME native_libre_reactor_smoke
      COMMAND axv_native_libre_reactor_smoke)
    set_tests_properties(native_libre_reactor_smoke PROPERTIES TIMEOUT 15)
  endif()
  add_executable(test_libre_owner_reactor
    tests/test_libre_owner_reactor.cpp
    src/baresip_native_libre_reactor.cpp)
  target_include_directories(test_libre_owner_reactor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_libre_owner_reactor PRIVATE cxx_std_17)
  add_test(NAME libre_owner_reactor COMMAND test_libre_owner_reactor)
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
  set_tests_properties(libre_owner_reactor PROPERTIES TIMEOUT 15)
  add_executable(test_ui_dpi_policy tests/test_ui_dpi_policy.cpp)
  target_compile_features(test_ui_dpi_policy PRIVATE cxx_std_17)
  add_test(NAME ui_dpi_policy COMMAND test_ui_dpi_policy)
  add_executable(test_ui_viewport_policy tests/test_ui_viewport_policy.cpp)
  target_compile_features(test_ui_viewport_policy PRIVATE cxx_std_17)
  add_test(NAME ui_viewport_policy COMMAND test_ui_viewport_policy)
  add_executable(test_ui_contrast_policy tests/test_ui_contrast_policy.cpp)
  target_compile_features(test_ui_contrast_policy PRIVATE cxx_std_17)
  add_test(NAME ui_contrast_policy COMMAND test_ui_contrast_policy)
  add_test(NAME ui_smoke COMMAND $<TARGET_FILE:AXEVoice> --ui-smoke)
  add_executable(test_ui_blackbox tests/test_ui_blackbox.cpp)
  target_compile_features(test_ui_blackbox PRIVATE cxx_std_17)
  target_compile_definitions(test_ui_blackbox PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  if(MSVC)
    target_compile_options(test_ui_blackbox PRIVATE /utf-8 /W4)
  endif()
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
  add_executable(test_ui_feedback_matrix tests/test_ui_feedback_matrix.cpp)
  target_compile_features(test_ui_feedback_matrix PRIVATE cxx_std_17)
  target_link_libraries(test_ui_feedback_matrix PRIVATE axv_sip_address)
  add_test(NAME ui_feedback_matrix COMMAND test_ui_feedback_matrix)
  add_executable(test_sip_runtime_observation tests/test_sip_runtime_observation.cpp)
  target_compile_features(test_sip_runtime_observation PRIVATE cxx_std_17)
  add_test(NAME sip_runtime_observation COMMAND test_sip_runtime_observation)
  add_executable(test_baresip_event_mailbox tests/test_baresip_event_mailbox.cpp)
  target_compile_features(test_baresip_event_mailbox PRIVATE cxx_std_17)
  add_test(NAME baresip_event_mailbox COMMAND test_baresip_event_mailbox)
  add_executable(test_baresip_native_ingress tests/test_baresip_native_ingress.cpp)
  target_compile_features(test_baresip_native_ingress PRIVATE cxx_std_17)
  add_test(NAME baresip_native_ingress COMMAND test_baresip_native_ingress)
  add_executable(test_baresip_native_pipeline tests/test_baresip_native_pipeline.cpp)
  target_compile_features(test_baresip_native_pipeline PRIVATE cxx_std_17)
  add_test(NAME baresip_native_pipeline COMMAND test_baresip_native_pipeline)
  add_executable(test_sip_batch_bounds tests/test_sip_batch_bounds.cpp)
  target_compile_features(test_sip_batch_bounds PRIVATE cxx_std_17)
  add_test(NAME sip_batch_bounds COMMAND test_sip_batch_bounds)
  add_executable(test_sip_dial_matrix tests/test_sip_dial_matrix.cpp)
  target_compile_features(test_sip_dial_matrix PRIVATE cxx_std_17)
  target_link_libraries(test_sip_dial_matrix PRIVATE axv_sip_address)
  add_test(NAME sip_dial_matrix COMMAND test_sip_dial_matrix)
  add_executable(test_sip_event_projection tests/test_sip_event_projection.cpp)
  target_compile_features(test_sip_event_projection PRIVATE cxx_std_17)
  add_test(NAME sip_event_projection COMMAND test_sip_event_projection)
  add_executable(test_sip_runtime_session_guard tests/test_sip_runtime_session_guard.cpp)
  target_compile_features(test_sip_runtime_session_guard PRIVATE cxx_std_17)
  add_test(NAME sip_runtime_session_guard COMMAND test_sip_runtime_session_guard)
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
  add_executable(test_sip_command_watchdog tests/test_sip_command_watchdog.cpp)
  target_compile_features(test_sip_command_watchdog PRIVATE cxx_std_17)
  target_link_libraries(test_sip_command_watchdog PRIVATE axv_sip_address)
  add_test(NAME sip_command_watchdog COMMAND test_sip_command_watchdog)
  add_executable(test_baresip_event_identity tests/test_baresip_event_identity.cpp)
  target_compile_features(test_baresip_event_identity PRIVATE cxx_std_17)
  add_test(NAME baresip_event_identity COMMAND test_baresip_event_identity)
  add_executable(test_baresip_event_reducer tests/test_baresip_event_reducer.cpp)
  target_compile_features(test_baresip_event_reducer PRIVATE cxx_std_17)
  add_test(NAME baresip_event_reducer COMMAND test_baresip_event_reducer)
  add_executable(test_baresip_call_correlation tests/test_baresip_call_correlation.cpp)
  target_compile_features(test_baresip_call_correlation PRIVATE cxx_std_17)
  add_test(NAME baresip_call_correlation COMMAND test_baresip_call_correlation)
  add_executable(test_baresip_pipeline_hardening tests/test_baresip_pipeline_hardening.cpp)
  target_compile_features(test_baresip_pipeline_hardening PRIVATE cxx_std_17)
  add_test(NAME baresip_pipeline_hardening COMMAND test_baresip_pipeline_hardening)
  add_executable(test_baresip_callback_ingress tests/test_baresip_callback_ingress.cpp)
  target_compile_features(test_baresip_callback_ingress PRIVATE cxx_std_17)
  add_test(NAME baresip_callback_ingress COMMAND test_baresip_callback_ingress)
  add_executable(test_baresip_callback_lifecycle tests/test_baresip_callback_lifecycle.cpp)
  target_compile_features(test_baresip_callback_lifecycle PRIVATE cxx_std_17)
  add_test(NAME baresip_callback_lifecycle COMMAND test_baresip_callback_lifecycle)
  add_test(NAME baresip_sdk_legacy_integrity_preflight COMMAND "${CMAKE_COMMAND}"
    "-DAXV_REPOSITORY_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
    "-DAXV_SCRATCH=${CMAKE_CURRENT_BINARY_DIR}/axv_sdk_integrity_test"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/test_baresip_sdk_integrity.cmake")
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
  add_executable(test_baresip_sdk_call_port
    tests/test_baresip_sdk_call_port.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_call_port PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_call_port PRIVATE cxx_std_17)
  if(MSVC)
    target_compile_options(test_baresip_sdk_call_port PRIVATE /utf-8 /W4)
  endif()
  target_link_libraries(test_baresip_sdk_call_port PRIVATE axv_sip_address)
  add_test(NAME baresip_sdk_call_port COMMAND test_baresip_sdk_call_port)

  add_executable(test_baresip_sdk_epochs tests/test_baresip_sdk_epochs.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_epochs PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_epochs PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_epochs COMMAND test_baresip_sdk_epochs)
  add_executable(test_baresip_sdk_registration_gateway
    tests/test_baresip_sdk_registration_gateway.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_registration_gateway PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_registration_gateway PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_registration_gateway COMMAND test_baresip_sdk_registration_gateway)
  add_executable(test_baresip_registration_watchdog tests/test_baresip_registration_watchdog.cpp)
  target_compile_features(test_baresip_registration_watchdog PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_watchdog COMMAND test_baresip_registration_watchdog)
  add_executable(test_baresip_native_thread_guard tests/test_baresip_native_thread_guard.cpp)
  target_compile_features(test_baresip_native_thread_guard PRIVATE cxx_std_17)
  add_test(NAME baresip_native_thread_guard COMMAND test_baresip_native_thread_guard)
  add_executable(test_baresip_registration_command_queue
    tests/test_baresip_registration_command_queue.cpp)
  target_compile_features(test_baresip_registration_command_queue PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_command_queue COMMAND test_baresip_registration_command_queue)
  add_executable(test_baresip_registration_command_dispatcher
    tests/test_baresip_registration_command_dispatcher.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_registration_command_dispatcher PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_registration_command_dispatcher PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_command_dispatcher COMMAND test_baresip_registration_command_dispatcher)
  add_executable(test_baresip_registration_command_races
    tests/test_baresip_registration_command_races.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_registration_command_races PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_registration_command_races PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_command_races COMMAND test_baresip_registration_command_races)
  add_executable(test_baresip_registration_lab_worker
    tests/test_baresip_registration_lab_worker.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_registration_lab_worker PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_registration_lab_worker PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_lab_worker COMMAND test_baresip_registration_lab_worker)
  # ABI type-check is compiled with fake headers only, with no native SDK.
  add_executable(test_baresip_sdk_abi_contract tests/test_baresip_sdk_abi_contract.cpp)
  target_include_directories(test_baresip_sdk_abi_contract PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_abi_contract PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_abi_contract COMMAND test_baresip_sdk_abi_contract)
  add_test(NAME baresip_sdk_integrity_preflight
    COMMAND "${CMAKE_COMMAND}"
      "-DAXV_REPOSITORY_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DAXV_SCRATCH=${CMAKE_CURRENT_BINARY_DIR}/sdk-integrity-cts"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/test_baresip_sdk_integrity.cmake")
  add_test(NAME libre_sdk_integrity_preflight
    COMMAND "${CMAKE_COMMAND}"
      "-DAXV_REPOSITORY_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DAXV_SCRATCH=${CMAKE_CURRENT_BINARY_DIR}/libre-integrity-cts"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/test_libre_sdk_integrity.cmake")
  set_tests_properties(baresip_sdk_integrity_preflight libre_sdk_integrity_preflight
    PROPERTIES TIMEOUT 15)
  set_tests_properties(baresip_registration_lab_worker PROPERTIES TIMEOUT 15)
  set_tests_properties(baresip_registration_command_races PROPERTIES TIMEOUT 15)
  set_tests_properties(baresip_registration_command_dispatcher PROPERTIES TIMEOUT 15)
  set_tests_properties(baresip_sdk_registration_gateway PROPERTIES TIMEOUT 15)
  set_tests_properties(baresip_sdk_event_adapter PROPERTIES TIMEOUT 15)
  add_executable(test_baresip_sdk_callback_epochs
    tests/test_baresip_sdk_callback_epochs.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_callback_epochs PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_callback_epochs PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_callback_epochs COMMAND test_baresip_sdk_callback_epochs)
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
  add_executable(test_audio_test_tone tests/test_audio_test_tone.cpp)
  target_compile_features(test_audio_test_tone PRIVATE cxx_std_17)
  target_link_libraries(test_audio_test_tone PRIVATE axv_audio_test_tone)
  add_test(NAME audio_test_tone COMMAND test_audio_test_tone)
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
if(BUILD_TESTING)
  add_executable(test_sip_event_projection_legacy tests/test_sip_event_projection_legacy.cpp)
  target_compile_features(test_sip_event_projection_legacy PRIVATE cxx_std_17)
  add_test(NAME sip_event_projection_legacy COMMAND test_sip_event_projection_legacy)
endif()

if(BUILD_TESTING)
# Each test belongs to exactly one quality gate. Fail the CI on empty tiers.
get_property(axv_registered_tests DIRECTORY PROPERTY TESTS)
foreach(axv_test IN LISTS axv_registered_tests)
  set_tests_properties("${axv_test}" PROPERTIES LABELS "unit")
endforeach()
set_tests_properties(ui_smoke ui_blackbox PROPERTIES LABELS "blackbox")
set_tests_properties(integration_sip_safety baresip_sdk_abi_probe windows_process sip_runtime_session_guard
  baresip_sdk_provenance
  sip_event_projection baresip_event_mailbox baresip_callback_ingress
  baresip_callback_lifecycle baresip_sdk_event_adapter baresip_sdk_single_owner
  baresip_sdk_call_port baresip_sdk_epochs sip_event_projection_legacy
  sip_batch_bounds baresip_native_ingress baresip_native_pipeline
  baresip_sdk_registration_gateway baresip_sdk_callback_epochs
  libre_owner_reactor libre_registration_owner_integration
  baresip_registration_command_dispatcher baresip_registration_command_races
  baresip_registration_lab_worker
  call_command_authorization sip_command_executor sip_command_watchdog PROPERTIES LABELS "integration")
endif()
set_target_properties(AXEVoice PROPERTIES OUTPUT_NAME "AXE-Voice")


# MinGW selects narrow entry points unless Unicode startup is explicit.
if(MINGW)
  target_link_libraries(axv_audio_test_tone PUBLIC ksuser)
  target_link_options(AXEVoice PRIVATE -municode)
  if(BUILD_TESTING)
    foreach(axv_unicode_target IN ITEMS test_ui_blackbox axv_test_child_process test_windows_process)
      target_link_options(${axv_unicode_target} PRIVATE -municode)
    endforeach()
  endif()
endif()
