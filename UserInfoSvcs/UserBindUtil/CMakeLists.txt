add_executable(UserBindUtil
  Application.cpp
)

target_link_libraries(UserBindUtil
  PRIVATE
    Generics
    LogCommons
    Commons
    ProfilingCommons
)
