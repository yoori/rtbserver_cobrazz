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

add_dependencies(UserColoReachContainer UserChannelInventoryProfilePST)

add_idl(ExpressionMatcherIDL
  ExpressionMatcher.idl
  ${IDL_DIRECTORY}/RequestInfoSvcs/ExpressionMatcher
)

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
  UserColoReachContainer
  UserInventoryContainer
  UserTriggerMatchContainer
  ExpressionChannelMatcher
  ExpressionChannelIndex

  ExpressionMatcherIDL
  CampaignServerIDL
  CORBACommons

  ConsiderMessagesPST
  RequestOperationProfilePST
  UserChannelInventoryProfilePST
  RequestTriggerMatchProfilePST

  ExpressionMatcherCompatibility
  UserInfoClient

  AdServerCommonsXSD
  ExpressionMatcherConfigXSD

  LogCommons
  Commons
  ProfilingCommons
  SNMPAgent
  ProfileMap
  ProfileMapRocksDB
  XMLUtility
  CampaignTypes
  CampaignConfigSource

  Logger
  String
  Generics
  Sync
  pthread
)
