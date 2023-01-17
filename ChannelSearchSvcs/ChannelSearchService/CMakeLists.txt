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
 ChannelProxyIDL
 ChannelProxy_v28IDL
 ChannelSearchServiceConfigXSD
 ChannelSearchServiceIDL
 ChannelUpdate_v28IDL
 ChannelCommonsIDL
 ChannelManagerControllerIDL
 ChannelClusterControlIDL
 ChannelProxyIDL
 DictionaryProviderIDL
 Commons
 ExpressionChannel
 ExpressionChannelIndex
 Polyglot
 ProcessControlVars

 )

install(TARGETS ChannelSearchServiceIDL ChannelSearchService DESTINATION ${INSTALL_DIR})