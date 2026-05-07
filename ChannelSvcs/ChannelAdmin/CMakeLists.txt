add_executable(ChannelAdmin
    Application.cpp
)

add_executable(ChannelAdmin2
    ChannelAdmin2.cpp
)
target_link_libraries(ChannelAdmin
  Logger
  Generics
  XMLUtility
  Commons
  UtilCommons
  ChannelCommons
  ChannelClient
  ChannelContainer
  ChannelProxyIDL
  ChannelProxy_v28IDL
  ChannelUpdate_v28IDL
  ChannelSessionFactory
  DictionaryProviderIDL
)

target_link_libraries(ChannelAdmin2
  Logger
  Generics
  Commons
  ChannelClient
  ChannelControllerGrpc
  ChannelServerGrpcClient
)

target_include_directories(ChannelAdmin2 PRIVATE
  ${PB_DIRECTORY}/ChannelSvcs/ChannelController2
  ${PB_DIRECTORY}/ChannelSvcs/ChannelServer
)

#install(TARGETS ChannelAdmin DESTINATION ${INSTALL_DIR})
