add_executable(PlatformMatcherTest
  PlatformMatcherTest.cpp)

target_include_directories(PlatformMatcherTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(PlatformMatcherTest
  PRIVATE
    RequestMatchers
    HTTP
)
