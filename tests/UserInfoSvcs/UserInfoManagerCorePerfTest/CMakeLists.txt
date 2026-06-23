add_executable(UserInfoManagerCorePerfTest
  UserInfoManagerCorePerfTest.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserInfoManager/FileRWStats.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserInfoManager/UserInfoManagerCore.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserInfoManager/UserInfoManagerLogger.cpp
)

target_link_libraries(UserInfoManagerCorePerfTest
  PRIVATE
    LogCommons
    Commons
    PlainStorage2
    PrivacyFilter
    ProfileMap
    ProfileMapRocksDB
    UserInfoManagerConfigXSD
    UserInfoContainer
    UserOperationProfilesPST
    Generics
    jemalloc
)

target_include_directories(UserInfoManagerCorePerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(UserInfoManagerCorePerfTest
  PRIVATE
    USER_INFO_MANAGER_CONFIG_XSD="${CMAKE_SOURCE_DIR}/xsd/UserInfoSvcs/UserInfoManagerConfig.xsd"
)
