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
)

#install(TARGETS ChannelAdmin DESTINATION ${INSTALL_DIR})
