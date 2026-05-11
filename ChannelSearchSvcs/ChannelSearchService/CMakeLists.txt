add_idl(ChannelSearchServiceIDL  ChannelSearchService.idl ${IDL_DIRECTORY}/ChannelSearchSvcs/ChannelSearchService)

target_link_libraries(ChannelSearchServiceIDL
  Generics
  CORBACommons
)

add_executable(ChannelSearchService
  ChannelMatcher.cpp
  ChannelSearchServiceMain.cpp
  ChannelSearchServiceImpl.cpp
)
target_link_libraries(ChannelSearchService
  AdServerCommonsXSD
  CORBACommons
  CampaignServerIDL
  ChannelManagerControllerIDL
  ChannelSearchServiceConfigXSD
  ChannelSearchServiceIDL
  ChannelCommonsIDL
  ChannelClusterControlIDL
  ChannelSessionFactory
  DictionaryProviderIDL
  Commons
  ExpressionChannel
  ExpressionChannelIndex
  Polyglot
  ProcessControlVars

)
