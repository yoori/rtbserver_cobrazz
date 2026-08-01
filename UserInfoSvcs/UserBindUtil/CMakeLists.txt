add_executable(UserBindUtil
  Application.cpp
)

target_link_libraries(UserBindUtil


  Generics
  CORBACommons

  LogCommons
  Commons
  ProfilingCommons


)

#install(TARGETS UserBindUtil  DESTINATION ${INSTALL_DIR})
