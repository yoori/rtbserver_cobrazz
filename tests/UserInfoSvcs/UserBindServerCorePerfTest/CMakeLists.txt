add_executable(UserBindServerCorePerfTest
  UserBindServerCorePerfTest.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindServerCore.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindOperationSaver.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindOperationLoader.cpp
)

target_link_libraries(UserBindServerCorePerfTest
  PRIVATE
    Commons
    ProfileMap
    ProfileMapRocksDB
    MessageSaver
    LogCommons
    UserBindContainer
    UserBindOperationProfilePST
    Generics
    Logger
    Sync
)

target_include_directories(UserBindServerCorePerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
