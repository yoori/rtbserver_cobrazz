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
# FileReceiver
# UserFreqCapProfile
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
 UServerUtilsMetrics

)

#install(TARGETS UserInfoManager UserInfoContainer UserInfoManagerIDL UserInfoManagerControlIDL  DESTINATION ${INSTALL_DIR})
