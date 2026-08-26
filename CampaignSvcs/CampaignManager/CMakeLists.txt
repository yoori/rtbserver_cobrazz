# CampaignManagerGrpc
set(CAMPAIGN_MANAGER_GRPC_PROTO_DIR ${PB_DIRECTORY}/CampaignSvcs/CampaignManager)
set(CAMPAIGN_MANAGER_GRPC_PROTO CampaignManagerGrpc.proto)

add_grpc_sources(
  CampaignManagerGrpc
  ${CAMPAIGN_MANAGER_GRPC_PROTO}
  ${CAMPAIGN_MANAGER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(CampaignManagerGrpc
  PUBLIC
    protobuf::libprotobuf
    GrpcBatch
)

add_library(CampaignManagerGrpcClient STATIC)

add_adserver_grpc_client_sources(
  CampaignManagerGrpcClient
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CampaignManagerGrpc.proto
  ${CMAKE_CURRENT_BINARY_DIR}
  AdServer::CampaignSvcs
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(CampaignManagerGrpcClient
  PUBLIC
    Commons
    GrpcBatch
    CampaignManagerGrpc
    gRPC::grpc++
  PRIVATE
    Sync
)

add_library(BidRateProvider SHARED
  BidCostProvider.cpp
)

target_link_libraries(BidRateProvider
  PRIVATE
    Generics
)

add_library(CTRProviderCore SHARED
  CTRProvider.cpp
  CTR/Algorithm.cpp
  CTR/Feature.cpp
  CTR/FeatureDescriptorResolver.cpp
)

target_link_libraries(CTRProviderCore
  PRIVATE
    Generics
    CampaignConfig
)

set_target_properties(CTRProviderCore PROPERTIES OUTPUT_NAME CTRProvider)

add_library(CTRProvider ALIAS CTRProviderCore)

add_library(CTRProviderImplLib SHARED
  CTRProviderImpl.cpp
  CTR/FTRLCTREvaluator.cpp
  CTR/TrivialCTREvaluator.cpp
  CTR/XGBoostPredictor.cpp
  CTR/XGBoostCTREvaluator.cpp
  CTR/VangaCTREvaluator.cpp
  CTR/CatBoostCTREvaluator.cpp
)

target_link_libraries(CTRProviderImplLib
  PRIVATE
    Boost::headers
    CTRProviderCore
    Commons
    ProfilingCommons
    xgboost::xgboost
    Vanga::DTree
    catboost::model
)

add_library(CampaignConfig SHARED
  CampaignConfig.cpp
  CreativeTemplate.cpp
)

target_link_libraries(CampaignConfig
  PRIVATE
    Generics
    HTTP
    Boost::headers
)

add_library(CampaignConfigSource
  CampaignConfigSource.cpp
)

target_link_libraries(CampaignConfigSource
  PRIVATE
    Generics
    HTTP
    CORBACommons
    CampaignCommonsIDL
    CorbaTypesIDL
    CampaignServerIDL
    CampaignConfig
    CampaignIndex
    CreativeTemplateArgs
    DomainParser
)

add_library(CampaignIndex
  CampaignIndex.cpp
  GeoChannelIndex.cpp
)

target_link_libraries(CampaignIndex
  PRIVATE
    Generics
    CampaignConfig
    CreativeTemplateArgs
    Boost::headers
)

add_library(CreativeTemplateArgs SHARED
  CreativeTemplateArgs.cpp
)

target_link_libraries(CreativeTemplateArgs
  PRIVATE
    Generics
    Boost::headers
)

add_library(InstantiateAdArgs STATIC
  InstantiateAd/CacheArgs.cpp
  InstantiateAd/InstantiateAdAdvertiserCreativeArgs.cpp
  InstantiateAd/InstantiateAdCommonCreativeArgs.cpp
  InstantiateAd/InstantiateAdContext.cpp
  InstantiateAd/InstantiateAdCreativeArgs.cpp
  InstantiateAd/InstantiateAdNoPassbackArgs.cpp
  InstantiateAd/InstantiateAdRequestArgs.cpp
  InstantiateAd/ProcessorArgs.cpp
  InstantiateAd/TokenProcessorArgs.cpp
  InstantiateAd/UnionArgs.cpp
)

target_link_libraries(InstantiateAdArgs
  PRIVATE
    Generics
    AdServerCommonsXSD
    CampaignManagerConfigXSD
    CreativeTemplateArgs
)

add_library(SecToken
  SecToken.cpp
)

target_link_libraries(SecToken
  PRIVATE
    Commons
    Generics
    Logger
)

add_library(PassbackTemplate
  PassbackTemplate.cpp
)

target_link_libraries(PassbackTemplate
  PRIVATE
    CreativeTemplateArgs
    Commons
    Generics
)

add_library(DomainParser
  DomainParser.cpp
)

target_link_libraries(DomainParser
  PRIVATE
    DomainConfigXSD
)

add_library(CampaignBilling SHARED
  CampaignBilling.cpp
)

target_link_libraries(CampaignBilling
  PUBLIC
    CampaignTypes
)

add_library(CampaignBillingAccount SHARED
  CampaignBillingAccount.cpp
)

target_link_libraries(CampaignBillingAccount
  PUBLIC
    CampaignBilling
    CampaignConfig
)

add_library(CampaignManagerLogger SHARED
  CampaignManagerLogger.cpp
  CampaignManagerLogAdapter.cpp
)

target_link_libraries(CampaignManagerLogger
  PRIVATE
    Generics
    LogCommons
    CreativeTemplateArgs
    CampaignConfig
    CampaignBillingAccount
    ExpressionChannelIndex
    Boost::headers
)

target_include_directories(CampaignManagerLogger
  PUBLIC
    ${CMAKE_SOURCE_DIR}/Utils/Predictor
)

add_executable(CampaignManager
  CampaignManagerCore.cpp
  CampaignManagerGrpc.cpp
  CampaignManagerMain.cpp
  CreativeInstantiator.cpp
  CreativeTextGenerator.cpp
  ConfigManips.cpp
  SelectCreative.cpp
  CampaignSelector.cpp
  BillingStateContainer.cpp
)

target_link_libraries(CampaignManager
  PRIVATE
    PkgConfig::Jemalloc
    CampaignConfigSource
    HTTP
    HttpServer
    CORBACommons
    Commons
    CORBAConfig
    AdServerCommonsXSD
    CampaignManagerConfigXSD
    CampaignCommonsIDL
    CampaignServerIDL
    CorbaTypesIDL
    ProcessControlIDL
    CampaignTypes
    ExpressionChannelIndex
    ExpressionChannel
    CreativeTemplateArgs
    InstantiateAdArgs
    CampaignConfig
    CampaignBillingAccount
    CampaignIndex
    DomainParser
    CampaignManagerLogger
    LogCommons
    PassbackTemplate
    SecToken
    CTRProviderImplLib
    BidRateProvider
    DomainConfigXSD
    CampaignManagerGrpc
    BillingServerGrpcClient
    Generics
    Logger
    Sync
    Boost::headers
)

target_include_directories(CampaignManager PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)
