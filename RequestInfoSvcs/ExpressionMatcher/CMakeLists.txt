include_directories(/opt/foros/vanga/include)

ADD_SUBDIRECTORY(Compatibility)

add_library(ExpressionChannelMatcher STATIC
  ChannelMatcher.cpp
)

target_link_libraries(ExpressionChannelMatcher
  ExpressionChannelIndex
  UServerUtils
  boost_serialization
)

add_library(UserTriggerMatchContainer STATIC
  UserTriggerMatchContainer.cpp
)

target_link_libraries(UserTriggerMatchContainer
  RequestTriggerMatchProfilePST
  UserTriggerMatchProfilePST
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
  ConversionProcessor.cpp
  ExpressionMatcherImpl.cpp
  ExpressionMatcherLogLoader.cpp
  ExpressionMatcherMain.cpp
  ExpressionMatcherOutLogger.cpp
  ExpressionMatcherStats.cpp
  UserTriggerMatchProfileProviderImpl.cpp
)

target_link_libraries(ExpressionMatcher
  AdServerCommonsXSD
  CORBACommons
  CampaignConfigSource
  CampaignServerIDL
  CampaignTypes
  ChannelCommons
  ChannelContainer
  ChannelSessionFactory
  Commons
  ConsiderMessagesPST
  ExpressionChannelIndex
  ExpressionChannelMatcher
  ExpressionMatcherCompatibility
  ExpressionMatcherConfigXSD
  ExpressionMatcherIDL
  Generics
  LogCommons
  Logger
  PlainStorage3
  ProcessControlIDL
  ProcessControlVars
  ProfileMap
  RequestOperationProfilePST
  RequestTriggerMatchProfilePST
  SNMPAgent
  String
  Sync
  UserChannelInventoryProfilePST
  UserChannelInventoryProfilePST
  UserColoReachContainer
  UserInfoManagerControllerL
  UserInventoryContainer
  UserTriggerMatchContainer
  XMLUtility
  pthread
)