add_executable(FastJsonParserTest
  FastJsonParserTest.cpp)

target_include_directories(FastJsonParserTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(FastJsonParserTest
  PRIVATE
    Commons
)
