add_idl(ChannelCommonsIDL ChannelCommons.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons NO_SERVANT)
add_idl(ChannelServerIDL ChannelServer.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons VALUETYPE)
add_idl(ChannelUpdateBaseIDL ChannelUpdateBase.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons VALUETYPE)

target_link_libraries(ChannelCommonsIDL
  PUBLIC
    CorbaTypesIDL
)

target_link_libraries(ChannelUpdateBaseIDL
  PUBLIC
    ChannelCommonsIDL
)

target_link_libraries(ChannelServerIDL
  PUBLIC
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
