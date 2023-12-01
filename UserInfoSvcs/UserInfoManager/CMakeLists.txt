ADD_SUBDIRECTORY(Compatibility)

add_idl(UserInfoManagerIDL  UserInfoManager.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserInfoManager)
add_idl(UserInfoManagerControlIDL  UserInfoManagerControl.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserInfoManager)

target_link_libraries(UserInfoManagerIDL
  CampaignFreqIDL
  CorbaTypesIDL
  FreqCapInfoIDL
)

add_library(UserInfoContainer SHARED
  UserInfoContainer.cpp 
  UserOperationLoader.cpp
  UserOperationSaver.cpp
)

add_dependencies(UserInfoContainer UserChannelBaseProfilePST )
target_link_libraries(UserInfoContainer
  Allocator
  ChannelMatcher
  Commons
  Generics
  LogCommons
  LogCommons
  PlainStorage2
  PlainStorage3
  PrivacyFilter
  UserBaseProfileAdapter
  UserFreqCapProfile
  UserFreqCapProfileAdapter
  UserHistoryProfileAdapter
  UserOperationProfilesAdapter
  UserOperationProfilesPST
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
  AdServerCommonsXSD
  CORBACommons
  CampaignServerIDL
  Commons
  LogCommons
  LogCommons
  PlainStorage2
  PrivacyFilter
  ProcessControlVars
  ProfileMap
  StatsDumper
  UserInfoContainer
  UserInfoExchangerIDL
  UserInfoManagerConfigXSD
  UserInfoManagerControlIDL
  UserInfoManagerIDL
  UserOperationProfilesPST
  UServerUtilsMetrics
)