add_idl(ChannelCommonsIDL ChannelCommons.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)
add_idl(ChannelServerIDL ChannelServer.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)
add_idl(ChannelUpdateBaseIDL ChannelUpdateBase.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)


target_link_libraries(ChannelCommonsIDL
 Generics
 HTTP
  Commons
 BLogic
 LogCommons
CORBACommons
CorbaTypesIDL
ChannelCommons
)

target_link_libraries(ChannelUpdateBaseIDL
 Generics
 HTTP
  Commons
 BLogic
 LogCommons
CORBACommons
CorbaTypesIDL
ChannelCommons
ChannelCommonsIDL
)

target_link_libraries(ChannelServerIDL
Generics
 HTTP
  Commons
 BLogic
 LogCommons
CORBACommons
CorbaTypesIDL
ChannelCommons
ChannelCommonsIDL
ChannelUpdateBaseIDL
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
)

#install(TARGETS ChannelCommons ChannelServerIDL ChannelUpdateBaseIDL ChannelCommonsIDL  DESTINATION ${INSTALL_DIR})