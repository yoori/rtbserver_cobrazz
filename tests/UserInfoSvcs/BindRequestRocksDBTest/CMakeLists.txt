add_executable(BindRequestRocksDBTest
  BindRequestRocksDBTest.cpp
)

target_link_libraries(BindRequestRocksDBTest
  PRIVATE
    UserBindContainer
)

add_custom_target(RunBindRequestRocksDBTest
  COMMAND $<TARGET_FILE:BindRequestRocksDBTest>
  DEPENDS BindRequestRocksDBTest
)

add_executable(UserBindServerStorageTest
  UserBindServerStorageTest.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindServerCore.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindOperationSaver.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindOperationLoader.cpp
)

target_link_libraries(UserBindServerStorageTest
  PRIVATE
    UserBindContainer
    MessageSaver
    LogCommons
    UserBindOperationProfilePST
    Logger
)

add_custom_target(RunUserBindServerStorageTest
  COMMAND $<TARGET_FILE:UserBindServerStorageTest>
  DEPENDS UserBindServerStorageTest
)
