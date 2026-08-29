add_executable(UserPropertiesLoggerPerfTest
  UserPropertiesLoggerPerfTest.cpp
)

target_link_libraries(UserPropertiesLoggerPerfTest
  PRIVATE
    Generics
    LogCommons
)
