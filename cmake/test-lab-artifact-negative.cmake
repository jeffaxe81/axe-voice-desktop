# Negative integrity checks in an isolated build directory. Never overwrite
# the validated AXE-Voice.exe or publish fake executables.
if(NOT DEFINED AXV_NEGATIVE_DIR OR AXV_NEGATIVE_DIR STREQUAL "")
  message(FATAL_ERROR "Negative test directory missing")
endif()
file(MAKE_DIRECTORY "${AXV_NEGATIVE_DIR}")
set(_validator "${CMAKE_CURRENT_LIST_DIR}/verify-lab-artifact.cmake")
function(expect_invalid input label)
  execute_process(COMMAND "${CMAKE_COMMAND}"
    "-DAXV_EXE=${input}"
    "-DAXV_OUTPUT=${AXV_NEGATIVE_DIR}/rejected.sha256"
    -P "${_validator}"
    RESULT_VARIABLE exit_code OUTPUT_QUIET ERROR_QUIET)
  if(exit_code EQUAL 0)
    message(FATAL_ERROR "Unsafe LAB manifest validator accepted: ${label}")
  endif()
endfunction()
expect_invalid("${AXV_NEGATIVE_DIR}/absent.exe" "missing file")
set(_bad_name "${AXV_NEGATIVE_DIR}/wrong-name.exe")
file(WRITE "${_bad_name}" "MZ")
expect_invalid("${_bad_name}" "wrong filename")
set(_fake "${AXV_NEGATIVE_DIR}/AXE-Voice.exe")
file(WRITE "${_fake}" "MZ")
expect_invalid("${_fake}" "truncated executable")
string(REPEAT "Z" 20000 _bytes)
file(WRITE "${_fake}" "${_bytes}")
expect_invalid("${_fake}" "non-Windows header")
file(WRITE "${_fake}" "MZ${_bytes}")
expect_invalid("${_fake}" "MZ-only forged payload without PE header")
if(EXISTS "${AXV_NEGATIVE_DIR}/rejected.sha256")
  message(FATAL_ERROR "Invalid file produced a hash manifest")
endif()
message(STATUS "LAB manifest negative cases: 5/5 rejected")
