add_executable(LogGeneralizerAdmin
  Application.cpp
)

target_link_libraries(LogGeneralizerAdmin
  LogGeneralizerLib
  LogGeneralizerIDL
  pthread
  Generics
  Commons
  CORBACommons
  ProcessControlIDL
  StatsIDL
  Postgres
  LogCommons
  LogGeneralizerConfigXSD
  SNMPAgent
  ProcessControlVars
  UtilCommons
  String
)

#install(TARGETS LogGeneralizerAdmin DESTINATION ${INSTALL_DIR})
