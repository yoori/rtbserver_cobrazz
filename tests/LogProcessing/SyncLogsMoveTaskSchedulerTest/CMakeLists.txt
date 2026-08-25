add_executable(SyncLogsMoveTaskSchedulerTest
  SyncLogsMoveTaskSchedulerTest.cpp
)

target_link_libraries(SyncLogsMoveTaskSchedulerTest
  PRIVATE
    SyncLogsLib
    pthread
    Generics
    Sync
    Commons
    CORBACommons
    LogCommons
    UtilCommons
)
