add_executable(CampaignManagerInstantiateCreativeTest
  CampaignManagerInstantiateCreativeTest.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CampaignManagerCore.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CreativeInstantiator.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CreativeTextGenerator.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/ConfigManips.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/SelectCreative.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CampaignSelector.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/BillingStateContainer.cpp
)

target_link_libraries(CampaignManagerInstantiateCreativeTest
  PRIVATE
    jemalloc
    TAO::PortableServer
    TAO::Valuetype

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
    InstantiateAdArgs
    CreativeTemplateArgs
    CampaignConfig
    CampaignBillingAccount
    CampaignIndex
    DomainParser
    CampaignManagerLogger
    CTRProvider
    PassbackTemplate
    SecToken

    BidRateProvider
    DomainConfigXSD
    CampaignManagerGrpc
    BillingServerGrpcClient
    Generics
    Logger
    Sync
)

target_include_directories(CampaignManagerInstantiateCreativeTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    /opt/foros/vanga/include
)
