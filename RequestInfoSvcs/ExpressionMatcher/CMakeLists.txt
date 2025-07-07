include_directories(/opt/foros/vanga/include)

ADD_SUBDIRECTORY(Compatibility)

add_library(ExpressionMatcherStatistics STATIC
  Statistics.cpp
)

target_link_libraries(ExpressionMatcherStatistics
  UServerUtils
)

add_library(ExpressionChannelMatcher STATIC
  ChannelMatcher.cpp
)

target_link_libraries(ExpressionChannelMatcher
  ExpressionChannelIndex
  ExpressionMatcherStatistics
  UServerUtils
  boost_serialization
)

add_library(UserTriggerMatchContainer STATIC
  UserTriggerMatchContainer.cpp
)

target_link_libraries(UserTriggerMatchContainer
  RequestTriggerMatchProfilePST
  UserTriggerMatchProfilePST
  UServerUtils
)

add_library(UserInventoryContainer STATIC
  UserInventoryContainer.cpp
)

target_link_libraries(UserInventoryContainer
  CampaignTypes
  ExpressionMatcherCompatibility
  UserChannelInventoryProfilePST
  UServerUtils
)

add_dependencies(UserInventoryContainer UserChannelInventoryProfilePST)

add_library(UserColoReachContainer STATIC
  UserColoReachContainer.cpp
)

target_link_libraries(UserColoReachContainer
  UServerUtils
)

add_dependencies(UserColoReachContainer UserChannelInventoryProfilePST )

add_idl(ExpressionMatcherIDL  ExpressionMatcher.idl ${IDL_DIRECTORY}/RequestInfoSvcs/ExpressionMatcher)

target_link_libraries(ExpressionMatcherIDL
  CORBACommons
  CorbaTypesIDL
)

add_library(ExpressionMatcherOutLogger STATIC
  ExpressionMatcherOutLogger.cpp
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
  ExpressionMatcherOutLogger
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
  UserColoReachContainer
  UserInfoManagerControllerL
  UserInventoryContainer
  UserTriggerMatchContainer
  XMLUtility
  pthread
)