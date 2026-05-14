add_executable(RefPoolTest
  RefPoolTest.cpp
)

target_link_libraries(RefPoolTest
  PRIVATE
  Generics
)

target_include_directories(RefPoolTest
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
