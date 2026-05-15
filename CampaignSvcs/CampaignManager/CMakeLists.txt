include_directories(/opt/foros/vanga/include . ${LIBXML2_INCLUDE_DIR})

add_idl(CampaignManagerIDL
  CampaignManager.idl
  ${IDL_DIRECTORY}/CampaignSvcs/CampaignManager
)

target_link_libraries(CampaignManagerIDL
  CorbaTypesIDL
  CampaignFreqIDL
  CampaignCommonsIDL
)

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
  CampaignManagerIDL
  ProcessControlIDL
  pthread
  pcre
  HTTP
)

add_definitions(-D_REENTRANT -D_USE_OCCI -DXSD_CXX11)

add_library(CTRProvider SHARED
  CTRProvider.cpp
  CTR/XGBoostPredictor.cpp
  CTR/XGBoostCTREvaluator.cpp
  CTR/VangaCTREvaluator.cpp
  CTR/FTRLCTREvaluator.cpp
  CTR/TrivialCTREvaluator.cpp
  CTR/CatBoostCTREvaluator.cpp
  CTR/Algorithm.cpp
  CTR/Feature.cpp
  CTR/FeatureDescriptorResolver.cpp
)

link_directories(/opt/foros/vanga/lib)

target_link_libraries(CTRProvider
  Generics
  PlainStorage3
  Gason
  CampaignConfig
  pcre
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
  XslTransformer
  ExpressionChannel
  CampaignManagerIDL
  BillingServerIDL
)

add_library(CampaignConfigSource
  CampaignConfigSource.cpp
)

target_link_libraries(CampaignConfigSource
  Generics
  HTTP
  CampaignConfig
  CampaignManagerIDL
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
)

target_link_libraries(CampaignManagerLogger
  Generics
  LogCommons
)

add_executable(CampaignManager
  CampaignManagerCore.cpp
  CampaignManagerImpl.cpp
  CampaignManagerGrpc.cpp
  CampaignManagerMain.cpp
  InstantiateCreative.cpp
  CreativeTextGenerator.cpp
  ConfigManips.cpp
  SelectCreative.cpp
  CampaignSelector.cpp
  BillingStateContainer.cpp
)

target_link_libraries(CampaignManager
  CampaignConfigSource
  Stream
  HTTP
  CORBACommons
  Commons
  XslTransformer
  Gason
  PlainStorage3

  AdServerCommonsXSD
  CampaignManagerConfigXSD

  CampaignCommonsIDL
  CampaignServerIDL
  CampaignManagerIDL
  CampaignTypes

  ProcessControlVars

  ExpressionChannelIndex
  ExpressionChannel
  CreativeTemplateArgs
  CampaignConfig
  CampaignIndex
  DomainParser
  CampaignManagerLogger
  PassbackTemplate
  SecToken
  BillingServerIDL
  KafkaProducer

  CTRProvider
  BidRateProvider
  DomainConfigXSD
  StatInfoIDL
  CampaignManagerGrpc
)

target_include_directories(CampaignManager PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)
