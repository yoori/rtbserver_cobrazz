add_idl(ChannelCommonsIDL ChannelCommons.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)
add_idl(ChannelServerIDL ChannelServer.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)
add_idl(ChannelUpdateBaseIDL ChannelUpdateBase.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)

target_link_libraries(ChannelCommonsIDL
  Commons
  BLogic
  LogCommons
  CORBACommons
  CorbaTypesIDL
)

target_link_libraries(ChannelUpdateBaseIDL
  Commons
  BLogic
  LogCommons
  CORBACommons
  CorbaTypesIDL
  ChannelCommonsIDL
)

target_link_libraries(ChannelServerIDL
  Commons
  LogCommons
  CORBACommons
  CorbaTypesIDL
  ChannelCommonsIDL
  ChannelUpdateBaseIDL
  CampaignServerIDL
  CorbaObjectRefIDL
  ProcessControlIDL
)

add_library(ChannelCommons SHARED
  ChannelUtils.cpp ChannelUpdateStatLogger.cpp TriggerParser.cpp
)

target_link_libraries(ChannelCommons
  Generics
  HTTP
  Commons
  BLogic
  LogCommons
  ChannelCommonsIDL
)

#install(TARGETS ChannelCommons ChannelServerIDL ChannelUpdateBaseIDL ChannelCommonsIDL  DESTINATION ${INSTALL_DIR})
