add_executable(UserBindServerCoreTest
  UserBindServerCoreTest.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindServerCore.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindOperationSaver.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindOperationLoader.cpp
)

target_link_libraries(UserBindServerCoreTest
  PRIVATE
    PrivacyFilter
    Commons
    ProfileMap
    ProfileMapRocksDB
    MessageSaver
    LogCommons
    UserBindContainer
    Generics
)

target_include_directories(UserBindServerCoreTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
