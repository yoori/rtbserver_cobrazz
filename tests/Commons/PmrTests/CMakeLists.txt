add_executable(PmrTests
  PmrTests.cpp)

find_library(TCMALLOC_LIBRARY tcmalloc REQUIRED)

target_link_libraries(PmrTests
  PRIVATE
    Generics
    ${TCMALLOC_LIBRARY}
)
