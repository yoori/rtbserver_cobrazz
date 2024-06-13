ADD_SUBDIRECTORY(Compatibility)

include_directories (
  ${rocksdb_SOURCE_DIR}/include
)

add_library(BillingProcessor SHARED
  BillingProcessor.cpp
)

target_link_libraries(BillingProcessor
  BillingServerIDL
  Commons
  Generics
  RequestProfilePST
)

add_library(PassbackContainer SHARED
  PassbackContainer.cpp
)

target_link_libraries(PassbackContainer
  Commons
  Generics
  PlainStorage2
  PrivacyFilter
  RequestInfoManagerCompatibility
  pcre
)

add_library(RequestInfoContainer SHARED
  RequestInfoContainer.cpp
)

target_link_libraries(RequestInfoContainer
  Commons
  Generics
  PlainStorage2
  PrivacyFilter
  ProfileMap
  RequestInfoManagerCompatibility
  pcre
)

add_idl(RequestInfoManagerIDL RequestInfoManager.idl ${IDL_DIRECTORY}/RequestInfoSvcs/RequestInfoManager)

target_link_libraries(RequestInfoManagerIDL
  CORBACommons
  CorbaTypesIDL
)

add_executable(RequestInfoManager
  ExpressionMatcherNotifier.cpp
  RequestInfoManagerImpl.cpp
  RequestInfoManagerMain.cpp
  RequestLogLoader.cpp
  RequestOperationDistributor.cpp
  RequestOutLogger.cpp
)

target_link_libraries(RequestInfoManager
  BillingProcessor
  CORBACommons
  CampaignServerIDL
  Commons
  ExpressionMatcherIDL
  LogCommons
  MessageSaver
  PassbackContainer
  PlainStorage2
  PrivacyFilter
  ProcessControlVars
  RequestInfoContainer
  RequestInfoManagerConfigXSD
  RequestInfoManagerIDL
  RequestLogLoader
  RequestOperationSaver
  RequestOutLogger
  SNMPAgent
  TagRequestProfiler
  UserActionInfoContainer
  UserCampaignReachContainer
  UserCampaignReachProfilePST
  UserChannelInventoryProfilePST
  UserFraudProtectionContainer
  UserInfoManagerControllerIDL
  UserInfoManagerControllerL
  UserSiteReachContainer
  UserTagRequestMergeContainer
  pcre
)

add_library(RequestLogLoader SHARED
  RequestInfoManagerStats.cpp
  RequestLogLoader.cpp
  RequestOperationLoader.cpp
)

target_link_libraries(RequestLogLoader
  Generics
  LogCommons
  LogGeneralizerConfigXSD
  LogGeneralizerIDL
  LogGeneralizerLib
  Logger
  PlainStorage3
  Postgres
  RequestCompatibility
  RequestInfoManagerCompatibility
  RequestInfoStatistics
  String
)

add_library(RequestOperationSaver SHARED
  RequestOperationSaver.cpp
)

target_link_libraries(RequestOperationSaver
  Commons
  Generics
  LogCommons
  MessageSaver
  PlainStorage3
  PrivacyFilter
  RequestLogLoader
  UserCampaignReachProfilePST
)

add_library(RequestOutLogger STATIC
  RequestOutLogger.cpp
)

target_link_libraries(RequestOutLogger
  LogCommons
)

add_library(TagRequestProfiler STATIC
  TagRequestProfiler.cpp
)

target_link_libraries(TagRequestProfiler
  LogCommons
)

add_library(UserActionInfoContainer STATIC
  UserActionInfoContainer.cpp
)

target_link_libraries(UserActionInfoContainer
  LogCommons
  ProfileMap
)

add_library(UserCampaignReachContainer STATIC
  UserCampaignReachContainer.cpp
)

target_link_libraries(UserCampaignReachContainer
  LogCommons
  pcre
)

add_library(UserFraudProtectionContainer STATIC
  UserFraudProtectionContainer.cpp
)

target_link_libraries(UserFraudProtectionContainer
  LogCommons
)

add_library(UserSiteReachContainer STATIC
  UserSiteReachContainer.cpp
)

target_link_libraries(UserSiteReachContainer
  LogCommons
)

add_library(UserTagRequestMergeContainer STATIC
  UserTagRequestMergeContainer.cpp
)

target_link_libraries(UserTagRequestMergeContainer
  LogCommons
)