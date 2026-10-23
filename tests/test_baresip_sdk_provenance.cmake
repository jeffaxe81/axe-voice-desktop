# CTest-driven provenance regression. Does not load or link Baresip.
foreach(name IN ITEMS AXV_GATE_SCRIPT AXV_FIXTURE_HEADER AXV_GATE_WORK_DIR)
  if(NOT DEFINED ${name} OR "${${name}}" STREQUAL "")
    message(FATAL_ERROR "Missing test parameter: ${name}")
  endif()
endforeach()
file(MAKE_DIRECTORY "${AXV_GATE_WORK_DIR}")
set(header "${AXV_GATE_WORK_DIR}/baresip.h")
set(library "${AXV_GATE_WORK_DIR}/fake-baresip.lib")
configure_file("${AXV_FIXTURE_HEADER}" "${header}" COPYONLY)
file(WRITE "${library}" "Synthetic fixture only. NOT a Baresip library.\n")
file(SHA256 "${header}" expected_header)
file(SHA256 "${library}" expected_library)

function(check_gate label should_pass h l hsha lsha)
  execute_process(COMMAND "${CMAKE_COMMAND}"
    "-DAXV_BARESIP_HEADER=${h}"
    "-DAXV_BARESIP_LIBRARY=${l}"
    "-DAXV_BARESIP_HEADER_SHA256=${hsha}"
    "-DAXV_BARESIP_LIBRARY_SHA256=${lsha}"
    -P "${AXV_GATE_SCRIPT}"
    RESULT_VARIABLE exitcode OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
  if(should_pass AND NOT "${exitcode}" STREQUAL "0")
    message(FATAL_ERROR "${label}: expected success, got ${exitcode}: ${stderr}")
  endif()
  if(NOT should_pass AND "${exitcode}" STREQUAL "0")
    message(FATAL_ERROR "${label}: accepted a forbidden SDK state")
  endif()
  message(STATUS "SDK provenance: ${label} PASS")
endfunction()
check_gate("matching hashes" TRUE "${header}" "${library}" "${expected_header}" "${expected_library}")
string(TOUPPER "${expected_header}" header_upper)
string(TOUPPER "${expected_library}" library_upper)
check_gate("uppercase hashes" TRUE "${header}" "${library}" "${header_upper}" "${library_upper}")
check_gate("no expected header hash" FALSE "${header}" "${library}" "" "${expected_library}")
check_gate("short library hash" FALSE "${header}" "${library}" "${expected_header}" "deadbeef")
check_gate("missing header" FALSE "${AXV_GATE_WORK_DIR}/absent.h" "${library}" "${expected_header}" "${expected_library}")
check_gate("directory as library" FALSE "${header}" "${AXV_GATE_WORK_DIR}" "${expected_header}" "${expected_library}")
file(APPEND "${library}" "TAMPERED\n")
check_gate("modified library" FALSE "${header}" "${library}" "${expected_header}" "${expected_library}")
file(APPEND "${header}" "// TAMPERED\n")
file(SHA256 "${library}" new_library_hash)
check_gate("modified header" FALSE "${header}" "${library}" "${expected_header}" "${new_library_hash}")
