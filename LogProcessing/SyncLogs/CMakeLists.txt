add_library(SyncLogs STATIC
  Utils.cpp 
  FileRouter.cpp 
  RSyncFileRouter.cpp 
  RouteHelpers.cpp 
  FeedRouteMover.cpp 
  FeedRouteProcessor.cpp 
  FeedRouteGroupProcessor.cpp 
  FetchRouteProcessor.cpp

)



add_executable(SyncLogsMain

    SyncLogsImpl.cpp
    SyncLogsMain.cpp
)
target_link_libraries(SyncLogsMain
    SyncLogs
#    LogGeneralizer
    pthread
    Generics
    Sync
    pcre
    
#    LogGeneralizerIDL
    Commons
    CORBACommons
    ProcessControlIDL
    StatsIDL
    Postgres
    LogCommons
#    LogGeneralizerConfigXSD
    SNMPAgent
    ProcessControlVars
    UtilCommons
    String
    SyncLogsConfigXSD
    XMLUtility


)

install(TARGETS SyncLogsMain  DESTINATION ${INSTALL_DIR})
