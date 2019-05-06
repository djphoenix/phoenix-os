function(add_lint_target SOURCES_LIST)
  find_program(LINT cpplint)
  list(REMOVE_DUPLICATES SOURCES_LIST)
  list(SORT SOURCES_LIST)
  list(FILTER SOURCES_LIST INCLUDE REGEX "\\.(c(c|uh?|pp|xx|\\+\\+|)?|h(pp|\\+\\+|xx)?)$")

  add_custom_target(lint
    COMMAND "${CMAKE_COMMAND}" -E chdir
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${LINT}" ${SOURCES_LIST}
    DEPENDS ${SOURCES_LIST}
    VERBATIM
  )
endfunction()
