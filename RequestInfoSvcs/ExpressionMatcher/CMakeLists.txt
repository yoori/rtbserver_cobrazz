include_directories( /opt/foros/vanga/include)


ADD_SUBDIRECTORY(Compatibility)

add_library(ExpressionChannelMatcher STATIC
ChannelMatcher.cpp
)
add_library(UserTriggerMatchContainer STATIC
UserTriggerMatchContainer.cpp
)
target_link_libraries(UserTriggerMatchContainer
UserTriggerMatchProfilePST
RequestTriggerMatchProfilePST
)

add_library(UserInventoryContainer STATIC
UserInventoryContainer.cpp
)
add_dependencies(UserInventoryContainer UserChannelInventoryProfilePST)

add_library(UserColoReachContainer STATIC
UserColoReachContainer.cpp
)
add_dependencies(UserColoReachContainer UserChannelInventoryProfilePST )

add_idl(ExpressionMatcherIDL  ExpressionMatcher.idl ${IDL_DIRECTORY}/RequestInfoSvcs/ExpressionMatcher)
target_link_libraries(ExpressionMatcherIDL
CORBACommons
CorbaTypesIDL
)

add_executable(ExpressionMatcher
  ExpressionMatcherLogLoader.cpp 
  ExpressionMatcherImpl.cpp 
  ExpressionMatcherMain.cpp 
  ExpressionMatcherOutLogger.cpp 
  ExpressionMatcherStats.cpp 
  UserTriggerMatchProfileProviderImpl.cpp 
  ConversionProcessor.cpp
)

target_link_libraries(ExpressionMatcher
  ConsiderMessagesPST
  RequestOperationProfilePST
  ExpressionMatcherIDL
  UserColoReachContainer
  UserInventoryContainer
  UserTriggerMatchContainer
  ExpressionChannelMatcher
  CORBACommons
  ProcessControlIDL
  Logger
  String
  Generics
  Sync
  pthread
  ExpressionChannelIndex
  UserChannelInventoryProfilePST

  LogCommons
  ProcessControlVars
  Commons


  AdServerCommonsXSD
  ExpressionMatcherConfigXSD

  UserChannelInventoryProfilePST
  ExpressionMatcherCompatibility
  RequestTriggerMatchProfilePST
  PlainStorage3
  SNMPAgent
  UserInfoManagerControllerL
  CampaignServerIDL
  ProfileMap
  XMLUtility
  ChannelCommons
  ChannelContainer
  ChannelSessionFactory
  CampaignTypes
  CampaignConfigSource
)


#install(TARGETS ExpressionMatcher ExpressionMatcherIDL  DESTINATION ${INSTALL_DIR})
