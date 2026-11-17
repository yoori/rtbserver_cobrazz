add_executable(PmrTests
  PmrTests.cpp)

find_library(JEMALLOC_LIBRARY jemalloc REQUIRED)

target_include_directories(PmrTests
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(PmrTests
  PRIVATE
    Generics
    ${JEMALLOC_LIBRARY}
)
