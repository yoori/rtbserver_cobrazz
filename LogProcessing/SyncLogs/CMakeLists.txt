add_library(SyncLogsLib STATIC
  Utils.cpp 
  FileRouter.cpp 
  RSyncFileRouter.cpp 
  RouteHelpers.cpp 
  FeedRouteMover.cpp 
  FeedRouteProcessor.cpp 
  FeedRouteGroupProcessor.cpp 
  FetchRouteProcessor.cpp
)

add_executable(SyncLogs
  SyncLogsImpl.cpp
  SyncLogsMain.cpp
)

target_link_libraries(SyncLogs
  SyncLogsLib
  pthread
  Generics
  Sync
  pcre
  Commons
  CORBACommons
  ProcessControlIDL
  StatsIDL
  Postgres
  LogCommons
  SNMPAgent
  ProcessControlVars
  UtilCommons
  String
  SyncLogsConfigXSD
  XMLUtility
)

#install(TARGETS SyncLogs DESTINATION ${INSTALL_DIR})
