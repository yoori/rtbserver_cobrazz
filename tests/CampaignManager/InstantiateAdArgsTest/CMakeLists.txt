add_executable(InstantiateAdArgsTest
  InstantiateAdArgsTest.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CampaignManagerCore.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CreativeInstantiator.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CreativeTextGenerator.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/ConfigManips.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/SelectCreative.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CampaignSelector.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/BillingStateContainer.cpp
)

target_link_libraries(InstantiateAdArgsTest
  PRIVATE
    jemalloc

    CampaignConfigSource
    Stream
    HTTP
    HttpServer
    CORBACommons
    Commons
    XslTransformer

    AdServerCommonsXSD
    CampaignManagerConfigXSD

    CampaignCommonsIDL
    CampaignServerIDL
    CampaignTypes

    ExpressionChannelIndex
    ExpressionChannel
    InstantiateAdArgs
    CreativeTemplateArgs
    CampaignConfig
    CampaignIndex
    DomainParser
    CampaignManagerLogger
    PassbackTemplate
    SecToken

    CTRProvider
    BidRateProvider
    DomainConfigXSD
    StatInfoIDL
    CampaignManagerGrpc
    BillingServerGrpcClient
    Generics
)

target_include_directories(InstantiateAdArgsTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    /opt/foros/vanga/include
)
