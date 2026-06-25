add_executable(FastJsonParserPerfTest
  FastJsonParserPerfTest.cpp)

target_include_directories(FastJsonParserPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(FastJsonParserPerfTest
  Commons
)
