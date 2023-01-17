include_directories(${IDL_DIRECTORY}/ChannelSvcs/ChannelProxy)

add_idl(ChannelProxyIDL ChannelProxy.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelProxy)
add_idl(ChannelProxy_v28IDL ChannelProxy_v28.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelProxy)
add_idl(ChannelUpdate_v28IDL ChannelUpdate_v28.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelProxy)

target_link_libraries(ChannelProxyIDL
  Generics
  CORBACommons
  ChannelCommons
  ChannelProxy_v28IDL
 )

target_link_libraries(ChannelProxy_v28IDL
  Generics
  CORBACommons
  ChannelCommons
  ChannelUpdate_v28IDL
)
target_link_libraries(ChannelUpdate_v28IDL
  Generics
  CORBACommons
  ChannelCommons
  AdServerCommonsXSD
    XMLUtility
  CORBACommons
  ProcessControlVars
  ChannelProxyConfigXSD
  ChannelContainer

  Commons
  ChannelCommonsIDL
  ChannelServerIDL
  ChannelUpdateBaseIDL
  ChannelCommons
#  ChannelProxy
  LogCommons
  ChannelSessionFactory
#  ChannelProxy

)


add_executable(ChannelProxy 
ChannelProxyImpl.cpp ChannelProxyMain.cpp ChannelProxyImpl_v28.cpp
)
target_link_libraries(ChannelProxy
ChannelProxyIDL
ChannelProxy_v28IDL
ChannelUpdate_v28IDL
)
install(TARGETS ChannelProxy ChannelUpdate_v28IDL ChannelProxyIDL  DESTINATION ${INSTALL_DIR})