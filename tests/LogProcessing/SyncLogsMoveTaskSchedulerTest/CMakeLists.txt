add_executable(SyncLogsMoveTaskSchedulerTest
  SyncLogsMoveTaskSchedulerTest.cpp
)

target_link_libraries(SyncLogsMoveTaskSchedulerTest
  PRIVATE
    SyncLogsLib
    Threads::Threads
    Generics
    Sync
    Commons
    CORBACommons
    LogCommons
)
