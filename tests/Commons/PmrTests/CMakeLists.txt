add_executable(PmrTests
  PmrTests.cpp)

find_library(TCMALLOC_LIBRARY tcmalloc REQUIRED)

target_include_directories(PmrTests
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(PmrTests
  PRIVATE
    Generics
    ${TCMALLOC_LIBRARY}
)
