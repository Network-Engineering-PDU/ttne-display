function(get_git_version output_variable)
  if(DEFINED DISPLAY_VERSION AND NOT "${DISPLAY_VERSION}" STREQUAL "")
    set(TMP "${DISPLAY_VERSION}")
  else()
    find_package(Git QUIET)
  endif()
  if(NOT DEFINED TMP AND GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE TMP
      ERROR_QUIET
      RESULT_VARIABLE GIT_DESCRIBE_RESULT
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
  if(NOT DEFINED TMP OR "${TMP}" STREQUAL "" OR (DEFINED GIT_DESCRIBE_RESULT AND NOT GIT_DESCRIBE_RESULT EQUAL 0))
    set(TMP "Unknown")
  endif()
  set(${output_variable} ${TMP} PARENT_SCOPE)
endfunction()
