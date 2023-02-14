
add_idl(CampaignFreqIDL CampaignFreq.idl ${IDL_DIRECTORY}/Commons)
add_idl(CookieInfoPackIDL CookieInfoPack.idl ${IDL_DIRECTORY}/Commons)
add_idl(CorbaTypesIDL CorbaTypes.idl ${IDL_DIRECTORY}/Commons)
add_idl(FreqCapInfoIDL FreqCapInfo.idl ${IDL_DIRECTORY}/Commons)

target_link_libraries(CampaignFreqIDL CorbaTypesIDL)
target_link_libraries(CookieInfoPackIDL CorbaTypesIDL)
target_link_libraries(FreqCapInfoIDL CorbaTypesIDL)



add_library(Commons  SHARED
  CorbaConfig.cpp 
  HostDistribution.cpp 
  PathManip.cpp 
  CountActiveObject.cpp 
  ZmqConfig.cpp 
  IPCrypter.cpp 
  UserInfoManip.cpp 
  Algs.cpp 
  CsvReader.cpp 
  ExternalUserIdUtils.cpp 
  ZmqSocketHolder.cpp 
  SecToken.cpp

)

target_link_libraries(Commons 
    ProcessControlIDL
    HostDistributionFileXSD xerces-c Generics AdServerCommonsXSD HTTP CORBACommons crypto
    TAO TAO_AnyTypeCode  TAO_CodecFactory TAO_CosEvent TAO_CosNaming TAO_CosNotification TAO_DynamicAny TAO_EndpointPolicy TAO_FaultTolerance
    TAO_FT_ClientORB TAO_FT_ServerORB TAO_FTORB_Utils TAO_IORManip TAO_IORTable 
    TAO_Messaging TAO_PI TAO_PI_Server TAO_PortableGroup TAO_PortableServer TAO_Security TAO_SSLIOP TAO_TC TAO_TC_IIOP TAO_Valuetype ACE_SSL 
    zmq  pthread ACE Logger Sync
    CorbaObjectRefIDL
    )


add_library(Gason SHARED
    Gason.cpp
)

target_link_libraries(Gason Generics)



add_library(ProcessControlVars SHARED
ProcessControlVarsImpl.cpp
)

target_link_libraries(ProcessControlVars
Stream
CORBACommons
AdServerCommonsXSD
Generics
CorbaTypesIDL
Commons
    zmq  pthread ACE Logger Sync
)





ADD_SUBDIRECTORY(Kafka)
#ADD_SUBDIRECTORY(Oracle)
ADD_SUBDIRECTORY(Postgres)
ADD_SUBDIRECTORY(Xslt)


#install(TARGETS CampaignFreqIDL CookieInfoPackIDL CorbaTypesIDL FreqCapInfoIDL Commons Gason ProcessControlVars   DESTINATION ${INSTALL_DIR})
