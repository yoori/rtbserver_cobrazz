add_pst(DMPProfilingInfoPST  DMPProfilingInfo.pst ${PST_DIRECTORY}/Frontends/ProfilingServer)

add_library(HashFilter SHARED
  HashFilter.cpp
)

target_link_libraries(HashFilter 
  Generics
  Sync
  pthread
)

add_executable(ProfilingServer
  DMPKafkaStreamer.cpp
  ProfilingServer.cpp
  ProfilingServerStats.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(ProfilingServer
  AdServerCommonsXSD
  BidRateProvider
  CORBACommons
  CTRProvider
  CampaignConfig
  CampaignConfigSource
  CampaignIndex
  CampaignManagerLogger
  Commons
  CreativeTemplateArgs
  DMPProfilingInfoPST
  Generics
  GrpcCampaignManagerPool
  GrpcChannelOperationPool
  HashFilter
  KafkaProducer
  ProcessControlVars
  ProfilingServerConfigXSD
  RequestMatchers
  SecToken
  UtilCommons
  XMLUtility
  zmq
)