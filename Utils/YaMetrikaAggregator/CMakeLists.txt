set(REQUIRED_GO_VERSION "1.25")
set(GO_EXECUTABLE_NAME YaMetrikaAggregator)
set(GO_OUTPUT ${CMAKE_BINARY_DIR}/bin/${GO_EXECUTABLE_NAME})

find_program(GO_EXECUTABLE go REQUIRED)
if(NOT GO_EXECUTABLE)
  message(FATAL_ERROR
    "Go compiler not found. "
    "Please ensure 'go' is in your PATH (run 'go version' to check).")
endif()

execute_process(
  COMMAND ${GO_EXECUTABLE} version
  OUTPUT_VARIABLE GO_VERSION_RAW
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REGEX REPLACE "go version go([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" GO_VERSION "${GO_VERSION_RAW}")
if(GO_VERSION STREQUAL "")
  message(FATAL_ERROR "Failed to parse Go version from: ${GO_VERSION_RAW}")
endif()

if(GO_VERSION VERSION_LESS ${REQUIRED_GO_VERSION})
  message(FATAL_ERROR "Go ${REQUIRED_GO_VERSION}+ is required. Found version: ${GO_VERSION}")
endif()

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/go.mod GO_MOD_CONTENT)
string(REGEX MATCH "go[ \t]+([0-9]+\\.[0-9]+)" _ ${GO_MOD_CONTENT})
if(NOT CMAKE_MATCH_1)
  message(FATAL_ERROR "go.mod does not specify Go version (missing 'go X.XX' line)")
endif()

set(GO_MOD_VERSION ${CMAKE_MATCH_1})
if(GO_MOD_VERSION VERSION_LESS ${REQUIRED_GO_VERSION})
  message(FATAL_ERROR
    "go.mod requires Go ${GO_MOD_VERSION}, but ${REQUIRED_GO_VERSION}+ is needed. "
    "Please update go.mod to 'go ${REQUIRED_GO_VERSION}' or higher.")
endif()

set(GO_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/application.go
  ${CMAKE_CURRENT_SOURCE_DIR}/config.go
  ${CMAKE_CURRENT_SOURCE_DIR}/http_client.go
  ${CMAKE_CURRENT_SOURCE_DIR}/logger.go
  ${CMAKE_CURRENT_SOURCE_DIR}/main.go
  ${CMAKE_CURRENT_SOURCE_DIR}/postgres.go
  ${CMAKE_CURRENT_SOURCE_DIR}/ya_metrika_client.go
  ${CMAKE_CURRENT_SOURCE_DIR}/go.mod
)

add_custom_command(
  OUTPUT ${GO_OUTPUT}
  COMMAND ${GO_EXECUTABLE} build -o ${GO_OUTPUT} .
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${GO_SOURCES}
  COMMENT "Building Go app → ${GO_OUTPUT}"
)

add_custom_target(go_app ALL DEPENDS ${GO_OUTPUT})