ADD_SUBDIRECTORY(Compatibility)

add_library(BillingProcessor SHARED
  BillingProcessor.cpp
)

target_link_libraries(BillingProcessor
  Generics
  Commons
  BillingServerIDL
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
  PassbackProfilePST
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
  RequestProfilePST
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
  UserInfoManagerControllerCall
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
  PlainStorage2
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
  RequestOperationProfilePST
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

  RequestOperationProfilePST
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
  UserActionProfilePST
  LogCommons
)

add_library(UserCampaignReachContainer STATIC
  UserCampaignReachContainer.cpp
)

target_link_libraries(UserCampaignReachContainer
  UserCampaignReachProfilePST
  LogCommons
  pcre
)

add_library(UserFraudProtectionContainer STATIC
  UserFraudProtectionContainer.cpp
)

target_link_libraries(UserFraudProtectionContainer
  UserFraudProtectionProfilePST
  LogCommons
)

add_library(UserSiteReachContainer STATIC
  UserSiteReachContainer.cpp
)

target_link_libraries(UserSiteReachContainer
  UserSiteReachProfilePST
  LogCommons
)

add_library(UserTagRequestMergeContainer STATIC
  UserTagRequestMergeContainer.cpp
)

target_link_libraries(UserTagRequestMergeContainer
  UserTagRequestMergeProfilePST
  LogCommons
)
