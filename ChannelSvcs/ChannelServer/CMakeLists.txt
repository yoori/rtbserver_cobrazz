
add_library(ChannelContainer SHARED

	ContainerMatchers.cpp 
      DictionaryMatcher.cpp 
      SoftMatcher.cpp 
      ChannelChunk.cpp 
      ChannelContainer.cpp 
      UpdateContainer.cpp
)


target_link_libraries(ChannelContainer
  Commons
  ChannelCommons
  DictionaryProviderIDL
)

add_executable(ChannelServer
ChannelServerVariant.cpp 
  ChannelServerImpl.cpp 
  ChannelServerControlImpl.cpp 
  ChannelUpdateImpl.cpp 
  ProcessStatsControl.cpp 
  ChannelServerMain.cpp

)

target_link_libraries(ChannelServer
  HTTP
  XMLUtility
  CORBACommons
  ProcessControlVars


  AdServerCommonsXSD
  Postgres
  ChannelServerConfigXSD
  ChannelCommons
ChannelServerIDL ChannelUpdateBaseIDL
    ChannelCommonsIDL
    ChannelUpdateBaseIDL
    ChannelServerIDL
  ChannelContainer
  ChannelSessionFactory

#  CampaignServerStubs
   CampaignServerStubs_v350
 CampaignServer_v350IDL
 ProcessControlIDL
 StatsIDL
 BLogic
 # ColoUpdateStat
 LogCommons
  Polyglot
  GenericSegmentor
)

install(TARGETS ChannelServer ChannelContainer  DESTINATION ${INSTALL_DIR})