ADD_SUBDIRECTORY(Compatibility)

add_idl(UserInfoManagerIDL  UserInfoManager.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserInfoManager)
add_idl(UserInfoManagerControlIDL  UserInfoManagerControl.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserInfoManager)

target_link_libraries(UserInfoManagerIDL
  CorbaTypesIDL
  CampaignFreqIDL
  FreqCapInfoIDL
)

add_library(UserInfoContainer SHARED
  UserInfoContainer.cpp 
  UserOperationLoader.cpp
  UserOperationSaver.cpp
)

add_dependencies(UserInfoContainer UserChannelBaseProfilePST )

include(${UNIXCOMMONS}/src/UServerUtils/Grpc/Generator/GrpcTargets.cmake)
add_grpc_library(UserInfoManagerProto
  PROTOS
    UserInfoManager.proto
  SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(UserInfoContainer
  Generics
  PrivacyFilter
  LogCommons
  Commons
  Allocator
  PlainStorage2
  PlainStorage3
  ChannelMatcher
  LogCommons
  UserFreqCapProfile
  UserOperationProfilesPST
  UserBaseProfileAdapter
  UserHistoryProfileAdapter
  UserOperationProfilesAdapter
  UserFreqCapProfileAdapter
)

add_executable(UserInfoManager
  UserInfoManagerMain.cpp
  FileRWStats.cpp 
  UserInfoManagerImpl.cpp 
  UserInfoManagerControlImpl.cpp 
  UserInfoContainer.cpp 
  UserInfoManagerLogger.cpp 
  UserInfoManagerStat.cpp
)

target_link_libraries(UserInfoManager
  CORBACommons
  LogCommons
  ProcessControlVars
  Commons
  AdServerCommonsXSD
  PlainStorage2
  PrivacyFilter
  ProfileMap
  UserInfoManagerConfigXSD
  UserInfoExchangerIDL
  UserInfoManagerIDL
  UserInfoManagerControlIDL
  CampaignServerIDL
  LogCommons
  UserInfoContainer
  StatsDumper
  UserOperationProfilesPST
  UserInfoManagerProto
  UServerUtils
)