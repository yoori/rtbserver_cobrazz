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

    CampaignConfigSource
    Stream
    HTTP
    HttpServer
    CORBACommons
    Commons
    XslTransformer
    Gason
    PlainStorage3

    AdServerCommonsXSD
    CampaignManagerConfigXSD

    CampaignCommonsIDL
    CampaignServerIDL
    CampaignTypes

    ExpressionChannelIndex
    ExpressionChannel
    CreativeTemplateArgs
    CampaignConfig
    CampaignIndex
    DomainParser
    CampaignManagerLogger
    PassbackTemplate
    SecToken

    BidRateProvider
    DomainConfigXSD
    StatInfoIDL
    CampaignManagerGrpc
    BillingServerGrpcClient
)

target_include_directories(CampaignManagerInstantiateCreativeTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    /opt/foros/vanga/include
)
