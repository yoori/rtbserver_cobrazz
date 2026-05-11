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
  ChannelClient
  ChannelContainer
  ChannelSessionFactory
  DictionaryProviderIDL
)

#install(TARGETS ChannelAdmin DESTINATION ${INSTALL_DIR})
