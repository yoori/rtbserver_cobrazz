include_directories(/opt/foros/vanga/include . ${LIBXML2_INCLUDE_DIR})

add_idl(CampaignManagerIDL CampaignManager.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignManager)

target_link_libraries(CampaignManagerIDL CorbaTypesIDL CampaignFreqIDL CampaignCommonsIDL)

include(${UNIXCOMMONS}/src/UServerUtils/Grpc/Generator/GrpcTargets.cmake)
add_grpc_library(CampaignManagerProto
  PROTOS
    CampaignManager.proto
  SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(BidRateProvider SHARED
  BidCostProvider.cpp
)

target_link_libraries(BidRateProvider
  CORBACommons
  CampaignManagerIDL
  CorbaTypesIDL
  Generics
  HTTP
  ProcessControlIDL
  pcre
  pthread
)

add_definitions(-D_REENTRANT -D_USE_OCCI  -DXSD_CXX11)

add_library(CTRProvider SHARED
  CTRProvider.cpp XGBoostPredictor.cpp
)

link_directories(/opt/foros/vanga/lib)

target_link_libraries(CTRProvider
  /opt/foros/vanga/lib/libVangaDTree.so
  BidCostPredictorLib
  CampaignConfig
  Gason
  Generics
  PlainStorage3
  pcre
  xgboost
)

add_library(CampaignConfig SHARED
  CampaignConfig.cpp
  CreativeTemplate.cpp
)

target_link_libraries(CampaignConfig
  BillingServerIDL
  CampaignManagerIDL
  ExpressionChannel
  Generics
  HTTP
  XslTransformer
)

add_library(CampaignConfigSource 
  CampaignConfigSource.cpp
)

target_link_libraries(CampaignConfigSource
  CampaignConfig
  CampaignIndex
  CampaignManagerIDL
  DomainParser
  Generics
  HTTP
)

add_library(CampaignIndex
  CampaignIndex.cpp GeoChannelIndex.cpp
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
  Commons
  Generics
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

add_dependencies(DomainParser  DomainConfigXSD)

add_library(CampaignManagerLogger SHARED
  CampaignManagerLogger.cpp
)

target_link_libraries(CampaignManagerLogger
  Generics
  LogCommons
)

add_executable(CampaignManager
  BillingStateContainer.cpp
  CampaignManagerImpl.cpp
  CampaignManagerMain.cpp
  CampaignSelector.cpp
  ConfigManips.cpp
  CreativeTextGenerator.cpp
  InstantiateCreative.cpp
  SelectCreative.cpp
)

target_link_libraries(CampaignManager
  AdServerCommonsXSD
  BidRateProvider
  BillingServerIDL
  CORBACommons
  CTRProvider
  CampaignCommonsIDL
  CampaignConfig
  CampaignConfigSource
  CampaignConfigSource
  CampaignIndex
  CampaignManagerConfigXSD
  CampaignManagerIDL
  CampaignManagerLogger
  CampaignManagerProto
  CampaignServerIDL
  CampaignTypes
  Commons
  CreativeTemplateArgs
  DomainConfigXSD
  DomainParser
  ExpressionChannel
  ExpressionChannelIndex
  Gason
  HTTP
  KafkaProducer
  PassbackTemplate
  PlainStorage3
  ProcessControlVars
  SecToken
  StatInfoIDL
  Stream
  XslTransformer
  UServerUtils
)