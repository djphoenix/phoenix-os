find_program(PVS_ANALYZER pvs-studio-analyzer)
find_program(PLOG_CONVERT plog-converter)

set(PVS_LOG ${CMAKE_BINARY_DIR}/pvs.log)
set(PVS_TASKS ${CMAKE_BINARY_DIR}/pvs.tasks)
set(PVS_HTML ${CMAKE_BINARY_DIR}/pvs.html)
set(PVS_FILTER "GA:1,2;OP:1,2,3;64:1;CS:1,2,3")

add_custom_target(check-pvs
  COMMAND "${PVS_ANALYZER}" analyze -i -j4 -a13 -o "${PVS_LOG}"
  COMMAND "${PLOG_CONVERT}" -m cwe -a "${PVS_FILTER}" -t tasklist -o "${PVS_TASKS}" "${PVS_LOG}"
  COMMAND "${PLOG_CONVERT}" -m cwe -a "${PVS_FILTER}" -t fullhtml -o "${PVS_HTML}" "${PVS_LOG}"
  DEPENDS ${LINT_SOURCES}
  VERBATIM
)
