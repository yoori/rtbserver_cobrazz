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
  ProfilingServerStats.cpp 
  ProfilingServer.cpp 
  RequestInfoFiller.cpp

)
target_link_libraries(ProfilingServer
 Generics
 Commons

 AdServerCommonsXSD
 
 XMLUtility

 CORBACommons
 ProcessControlVars

 RequestMatchers
# CampaignManagerStubs
CampaignManagerIDL BidRateProvider CTRProvider CampaignConfig CampaignConfigSource CampaignIndex CreativeTemplateArgs SecToken  CampaignManagerLogger
# UserInfoManagerControllerStubs
UserInfoManagerControllerIDL UserInfoClusterControlIDL  UserInfoManagerControllerL

 ProfilingServerConfigXSD
 zmq
 UtilCommons
# DMPProfilingInfo
 KafkaProducer
 HashFilter
DMPProfilingInfoPST

)

install(TARGETS ProfilingServer HashFilter  DESTINATION ${INSTALL_DIR})