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

#install(TARGETS UserBindUtil  DESTINATION ${INSTALL_DIR})
