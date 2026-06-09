add_executable(UserInfoManagerCoreTest
  UserInfoManagerCoreTest.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserInfoManager/FileRWStats.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserInfoManager/UserInfoManagerCore.cpp
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserInfoManager/UserInfoManagerLogger.cpp
)

target_link_libraries(UserInfoManagerCoreTest
  PRIVATE
    CORBACommons
    LogCommons
    Commons
    PlainStorage2
    PrivacyFilter
    ProfileMap
    ProfileMapRocksDB
    UserInfoManagerConfigXSD
    CampaignServerIDL
    UserInfoContainer
    UserOperationProfilesPST
    Generics
)

target_include_directories(UserInfoManagerCoreTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(UserInfoManagerCoreTest
  PRIVATE
    USER_INFO_MANAGER_CONFIG_XSD="${CMAKE_SOURCE_DIR}/xsd/UserInfoSvcs/UserInfoManagerConfig.xsd"
)
