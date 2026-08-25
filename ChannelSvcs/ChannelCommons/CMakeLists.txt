add_idl(ChannelCommonsIDL ChannelCommons.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)
add_idl(ChannelServerIDL ChannelServer.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)
add_idl(ChannelUpdateBaseIDL ChannelUpdateBase.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)

target_link_libraries(ChannelCommonsIDL
  PRIVATE
    CorbaTypesIDL
)

target_link_libraries(ChannelUpdateBaseIDL
  PRIVATE
    ChannelCommonsIDL
)

target_link_libraries(ChannelServerIDL
  PRIVATE
    CorbaTypesIDL
    ChannelCommonsIDL
    ChannelUpdateBaseIDL
    CorbaObjectRefIDL
)
add_dependencies(ChannelServerIDL CampaignServerIDL)

add_library(ChannelCommons SHARED
  ChannelUtils.cpp ChannelUpdateStatLogger.cpp TriggerParser.cpp
)

target_link_libraries(ChannelCommons
  PRIVATE
  Generics
  HTTP
  BLogic
  LogCommons
)
add_dependencies(ChannelCommons ChannelCommonsIDL)

#install(TARGETS ChannelCommons ChannelServerIDL ChannelUpdateBaseIDL ChannelCommonsIDL  DESTINATION ${INSTALL_DIR})
