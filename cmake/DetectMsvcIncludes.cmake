# CMake 3.30's ENCODING AUTO probe can decode UTF-8 /showIncludes output as
# OEM text when MSVC has only a non-English language pack. Ninja compares raw
# bytes; a mangled prefix silently drops header dependencies. Probe without
# transcoding, inside the build tree, using the same compiler as the build.
set(_prefix_dir "${CMAKE_BINARY_DIR}/CMakeFiles/NativeIncludePrefix")
file(MAKE_DIRECTORY "${_prefix_dir}")
file(WRITE "${_prefix_dir}/probe.h" "\n")
file(WRITE "${_prefix_dir}/probe.cpp" "#include \"probe.h\"\n")
execute_process(
  COMMAND "${CMAKE_CXX_COMPILER}" /nologo /showIncludes /c probe.cpp
  WORKING_DIRECTORY "${_prefix_dir}"
  OUTPUT_VARIABLE _prefix_output
  ERROR_VARIABLE _prefix_error
  RESULT_VARIABLE _prefix_result
  ENCODING NONE
)
if(_prefix_result EQUAL 0 AND "${_prefix_output}" MATCHES "(^|\n)([^:\n][^:\n]+:[^:\n]*[^: \n][^: \n]:?[ \t]+)([A-Za-z]:\\\\|\\./|\\.\\\\|/)")
  set(CMAKE_CL_SHOWINCLUDES_PREFIX "${CMAKE_MATCH_2}")
else()
  message(FATAL_ERROR "Could not detect MSVC header dependency prefix: ${_prefix_error}")
endif()
unset(_prefix_dir)
unset(_prefix_output)
unset(_prefix_error)
unset(_prefix_result)
