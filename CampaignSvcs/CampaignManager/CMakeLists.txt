include_directories(/opt/foros/vanga/include . ${LIBXML2_INCLUDE_DIR})

add_idl(CampaignManagerIDL CampaignManager.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignManager)

target_link_libraries(CampaignManagerIDL CorbaTypesIDL CampaignFreqIDL CampaignCommonsIDL)

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


add_definitions(-D_REENTRANT -D_USE_OCCI  -DXSD_CXX11)

add_library(CTRProvider SHARED
     CTRProvider.cpp XGBoostPredictor.cpp CTRTrivialPredictor.cpp
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
)




add_library(CampaignConfig SHARED
CampaignConfig.cpp CreativeTemplate.cpp

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
 Generics
 Commons

)


add_library(SecToken 
SecToken.cpp
)

add_library(PassbackTemplate
PassbackTemplate.cpp
)

add_library( DomainParser
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
  CampaignManagerImpl.cpp 
  CampaignManagerMain.cpp 
  InstantiateCreative.cpp 
  CreativeTextGenerator.cpp 
  CampaignConfigSource.cpp 
  ConfigManips.cpp 
  SelectCreative.cpp 
  CampaignSelector.cpp 
  BillingStateContainer.cpp
)
target_link_libraries(CampaignManager
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
 CampaignConfigSource
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

)


install(TARGETS CampaignManager CampaignManagerLogger CreativeTemplateArgs CampaignConfig CampaignConfig CTRProvider CampaignManagerIDL BidRateProvider  DESTINATION ${INSTALL_DIR})