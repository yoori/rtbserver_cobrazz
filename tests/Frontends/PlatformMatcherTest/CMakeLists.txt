add_executable(PlatformMatcherTest
  PlatformMatcherTest.cpp)

target_link_libraries(PlatformMatcherTest
  PRIVATE
    RequestMatchers
    HTTP
)
