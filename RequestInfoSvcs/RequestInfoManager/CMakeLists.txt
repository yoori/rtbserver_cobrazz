ADD_SUBDIRECTORY(Compatibility)

add_library(BillingProcessor SHARED
  BillingProcessor.cpp
)

target_link_libraries(BillingProcessor
  Generics
  Commons
  BillingServerIDL
  RequestProfilePST
)

add_library(PassbackContainer SHARED
  PassbackContainer.cpp
)

target_link_libraries(PassbackContainer
  pcre
  Generics
  PlainStorage2
  PrivacyFilter
  Commons
  RequestInfoManagerCompatibility
)

add_library(RequestInfoContainer SHARED
  RequestInfoContainer.cpp
)

target_link_libraries(RequestInfoContainer
  pcre
  Generics
  PlainStorage2
  PrivacyFilter
  Commons
  ProfileMap
  RequestInfoManagerCompatibility
)

add_idl(RequestInfoManagerIDL RequestInfoManager.idl ${IDL_DIRECTORY}/RequestInfoSvcs/RequestInfoManager)

target_link_libraries(RequestInfoManagerIDL
  CORBACommons
  CorbaTypesIDL
)

add_executable(RequestInfoManager
  RequestInfoManagerMain.cpp 
  RequestInfoManagerImpl.cpp 
  RequestOutLogger.cpp 
  RequestLogLoader.cpp
  RequestOperationDistributor.cpp 
  ExpressionMatcherNotifier.cpp
)

target_link_libraries(RequestInfoManager
  pcre
  CORBACommons
  ProcessControlVars
  PrivacyFilter
  SNMPAgent
  RequestInfoManagerIDL
  Commons

  RequestInfoManagerConfigXSD

  BillingProcessor
  PassbackContainer
  RequestInfoContainer
  ExpressionMatcherIDL
  UserInfoManagerControllerIDL
  CampaignServerIDL
  LogCommons

  RequestInfoContainer

  BillingProcessor
  MessageSaver
  RequestLogLoader
  RequestOperationSaver
  RequestOutLogger
  TagRequestProfiler
  UserActionInfoContainer
  UserCampaignReachContainer
  UserFraudProtectionContainer
  UserSiteReachContainer
  UserTagRequestMergeContainer
  UserCampaignReachProfilePST
  UserChannelInventoryProfilePST
  UserCampaignReachProfilePST
  PlainStorage2
  UserCampaignReachProfilePST
  UserInfoManagerControllerL
  UServerUtilsMetrics
)

add_library(RequestLogLoader SHARED
  RequestLogLoader.cpp 
  RequestOperationLoader.cpp 
  RequestInfoManagerStats.cpp
)

target_link_libraries(RequestLogLoader
  Generics
  Logger
  String
  LogCommons
  PlainStorage3
  RequestInfoManagerCompatibility
  LogGeneralizerIDL
  LogGeneralizerLib
  RequestCompatibility
  LogGeneralizerConfigXSD
  Postgres
)

add_library(RequestOperationSaver SHARED
  RequestOperationSaver.cpp
)

target_link_libraries(RequestOperationSaver
  Generics
  PlainStorage3
  PrivacyFilter
  Commons
  LogCommons

  RequestLogLoader
  MessageSaver

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

#install(TARGETS BillingProcessor PassbackContainer RequestInfoContainer RequestInfoManagerIDL RequestInfoManager RequestLogLoader RequestOperationSaver  DESTINATION ${INSTALL_DIR})
