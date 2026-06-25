add_executable(GasonPerfTest
  GasonPerfTest.cpp)

target_link_libraries(GasonPerfTest
  PRIVATE
    Gason
)

target_include_directories(GasonPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
