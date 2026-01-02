function(get_git_version output_variable)
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE TMP
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  else()
    set(TMP "Unknown")
  endif()
  set(${output_variable} ${TMP} PARENT_SCOPE)
endfunction()