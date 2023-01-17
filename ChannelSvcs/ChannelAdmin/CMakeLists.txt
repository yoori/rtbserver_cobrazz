add_executable(ChannelAdmin
    Application.cpp
)
target_link_libraries(ChannelAdmin
  Logger
  Generics
  XMLUtility
  Commons
  UtilCommons
  ChannelCommons
  ChannelContainer
  ChannelProxyIDL
  ChannelProxy_v28IDL
  ChannelUpdate_v28IDL  
  ChannelSessionFactory
  DictionaryProviderIDL
)

install(TARGETS ChannelAdmin DESTINATION ${INSTALL_DIR})