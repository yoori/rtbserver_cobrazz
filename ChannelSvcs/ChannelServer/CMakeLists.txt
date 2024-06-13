add_library(ChannelContainer SHARED
  ChannelChunk.cpp
  ChannelContainer.cpp
  ContainerMatchers.cpp
  DictionaryMatcher.cpp
  SoftMatcher.cpp
  UpdateContainer.cpp
)

target_link_libraries(ChannelContainer
  ChannelCommons
  Commons
  DictionaryProviderIDL
)

add_library(GrpcChannelOperationPool SHARED
  GrpcChannelOperationPool.cpp
)

target_link_libraries(GrpcChannelOperationPool
  ChannelCommons
  UServerUtils
)

add_executable(ChannelServer
  ChannelServerControlImpl.cpp
  ChannelServerImpl.cpp
  ChannelServerMain.cpp
  ChannelServerVariant.cpp
  ChannelUpdateImpl.cpp
  ProcessStatsControl.cpp
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
  ChannelServerIDL
  ChannelUpdateBaseIDL
  ChannelCommonsIDL
  ChannelUpdateBaseIDL
  ChannelServerIDL
  ChannelContainer
  ChannelSessionFactory
  CampaignServerStubs_v350
  CampaignServer_v350IDL
  ProcessControlIDL
  StatsIDL
  BLogic
  LogCommons
  Polyglot
  GenericSegmentor
)