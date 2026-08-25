add_executable(logProcessingLogIoTest
  LogIoTest.cpp
)

target_link_libraries(logProcessingLogIoTest
  PRIVATE
  Generics
  LogCommons)

