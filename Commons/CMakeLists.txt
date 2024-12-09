add_idl(CampaignFreqIDL CampaignFreq.idl ${IDL_DIRECTORY}/Commons)
add_idl(CookieInfoPackIDL CookieInfoPack.idl ${IDL_DIRECTORY}/Commons)
add_idl(CorbaTypesIDL CorbaTypes.idl ${IDL_DIRECTORY}/Commons)
add_idl(FreqCapInfoIDL FreqCapInfo.idl ${IDL_DIRECTORY}/Commons)

target_link_libraries(CampaignFreqIDL CorbaTypesIDL)
target_link_libraries(CookieInfoPackIDL CorbaTypesIDL)
target_link_libraries(FreqCapInfoIDL CorbaTypesIDL)

add_library(Commons SHARED
  Algs.cpp
  CorbaConfig.cpp
  CountActiveObject.cpp
  CsvReader.cpp
  ExternalUserIdUtils.cpp
  HostDistribution.cpp
  IPCrypter.cpp
  PathManip.cpp
  SecToken.cpp
  UserInfoManip.cpp
  ZmqConfig.cpp
  ZmqSocketHolder.cpp
)

target_link_libraries(Commons
  ACE
  ACE_SSL
  AdServerCommonsXSD
  CORBACommons
  CorbaObjectRefIDL
  Generics
  HTTP
  HostDistributionFileXSD
  Logger
  ProcessControlIDL
  Sync
  TAO TAO_AnyTypeCode
  TAO_CodecFactory
  TAO_CosEvent
  TAO_CosNaming
  TAO_CosNotification
  TAO_DynamicAny
  TAO_EndpointPolicy
  TAO_FTORB_Utils
  TAO_FT_ClientORB
  TAO_FT_ServerORB
  TAO_FaultTolerance
  TAO_IORManip
  TAO_IORTable
  TAO_Messaging
  TAO_PI
  TAO_PI_Server
  TAO_PortableGroup
  TAO_PortableServer
  TAO_SSLIOP
  TAO_Security
  TAO_TC
  TAO_TC_IIOP
  TAO_Valuetype
  crypto
  pthread
  xerces-c
  zmq
)

add_library(Gason SHARED
  Gason.cpp
)

target_link_libraries(Gason
  Generics
)

add_library(ProcessControlVars SHARED
  ProcessControlVarsImpl.cpp
)

target_link_libraries(ProcessControlVars
  ACE
  AdServerCommonsXSD
  CORBACommons
  Commons
  CorbaTypesIDL
  Generics
  Logger
  Stream
  Sync
  pthread
  zmq
)

ADD_SUBDIRECTORY(Kafka)
ADD_SUBDIRECTORY(Postgres)
ADD_SUBDIRECTORY(Xslt)