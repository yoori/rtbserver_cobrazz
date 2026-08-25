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

target_link_libraries(SyncLogsLib
  PUBLIC
    Generics
    Sync
    Commons
    LogCommons
)

add_executable(SyncLogs
  SyncLogsImpl.cpp
  SyncLogsMain.cpp
)

target_link_libraries(SyncLogs
  PRIVATE
    SyncLogsLib
    Threads::Threads
    Generics
    Commons
    CORBAConfig
    CORBACommons
    LogCommons
    Logger
    ProcessControlIDL
    ProcessControlVars
    Sync
    SyncLogsConfigXSD
    XMLUtility
)
