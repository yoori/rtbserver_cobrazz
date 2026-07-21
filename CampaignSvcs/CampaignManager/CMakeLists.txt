include_directories(/opt/foros/vanga/include . ${LIBXML2_INCLUDE_DIR})

# CampaignManagerGrpc
set(CAMPAIGN_MANAGER_GRPC_PROTO_DIR ${PB_DIRECTORY}/CampaignSvcs/CampaignManager)
set(CAMPAIGN_MANAGER_GRPC_PROTO CampaignManagerGrpc.proto)

add_grpc_sources(
  CampaignManagerGrpc
  ${CAMPAIGN_MANAGER_GRPC_PROTO}
  ${CAMPAIGN_MANAGER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(CampaignManagerGrpc protobuf::libprotobuf GrpcBatch)

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
  gRPC::grpc++)

add_library(BidRateProvider SHARED
  BidCostProvider.cpp
)

target_link_libraries(BidRateProvider
  Generics
  CORBACommons
  CorbaTypesIDL
  pthread
  pcre
  HTTP
)

add_definitions(-D_REENTRANT -D_USE_OCCI -DXSD_CXX11)

add_library(CTRProviderCore SHARED
  CTRProvider.cpp
  CTR/Algorithm.cpp
  CTR/Feature.cpp
  CTR/FeatureDescriptorResolver.cpp
)

link_directories(/opt/foros/vanga/lib)

target_link_libraries(CTRProviderCore
  Generics
  PlainStorage3
  CampaignConfig
  pcre
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
  CTRProviderCore
  xgboost
  /opt/foros/vanga/lib/libVangaDTree.so
  catboost::model
)

add_library(CampaignConfig SHARED
  CampaignConfig.cpp
  CreativeTemplate.cpp
)

target_link_libraries(CampaignConfig
  Generics
  HTTP
  ExpressionChannel
)

add_library(CampaignConfigSource
  CampaignConfigSource.cpp
)

target_link_libraries(CampaignConfigSource
  Generics
  HTTP
  CampaignConfig
  CampaignIndex
  DomainParser
)

add_library(CampaignIndex
  CampaignIndex.cpp
  GeoChannelIndex.cpp
)

target_link_libraries(CampaignIndex
  Generics
  CampaignConfig
  CreativeTemplateArgs
)

add_library( CreativeTemplateArgs SHARED
  CreativeTemplateArgs.cpp
)

target_link_libraries(CreativeTemplateArgs
  Generics
  Commons
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
  Generics
  AdServerCommonsXSD
  CampaignManagerConfigXSD
  CreativeTemplateArgs
)

target_include_directories(InstantiateAdArgs
  PUBLIC
    /opt/foros/vanga/include
)

add_library(SecToken
  SecToken.cpp
)

add_library(PassbackTemplate
  PassbackTemplate.cpp
)

add_library(DomainParser
  DomainParser.cpp
)

add_dependencies(DomainParser DomainConfigXSD)

add_library(CampaignManagerLogger SHARED
  CampaignManagerLogger.cpp
  CampaignManagerLogAdapter.cpp
)

target_link_libraries(CampaignManagerLogger
  Generics
  LogCommons
  CreativeTemplateArgs
  CampaignConfig
  CTRProviderCore
  ExpressionChannelIndex
)

target_include_directories(CampaignManagerLogger
  PUBLIC
    /opt/foros/vanga/include
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
  jemalloc

  CampaignConfigSource
  Stream
  HTTP
  HttpServer
  CORBACommons
  Commons

  AdServerCommonsXSD
  CampaignManagerConfigXSD

  CampaignCommonsIDL
  CampaignServerIDL
  CampaignTypes

  ExpressionChannelIndex
  ExpressionChannel
  CreativeTemplateArgs
  InstantiateAdArgs
  CampaignConfig
  CampaignIndex
  DomainParser
  CampaignManagerLogger
  PassbackTemplate
  SecToken

  CTRProviderCore
  CTRProviderImplLib
  BidRateProvider
  DomainConfigXSD
  StatInfoIDL
  CampaignManagerGrpc
  BillingServerGrpcClient
)

target_include_directories(CampaignManager PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)
