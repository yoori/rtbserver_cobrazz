add_idl(BillStatInfoIDL BillStatInfo.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer)
add_idl(StatInfoIDL StatInfo.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer)
add_idl(CampaignServerIDL CampaignServer.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer)

target_link_libraries(StatInfoIDL CorbaTypesIDL)
target_link_libraries(BillStatInfoIDL CorbaTypesIDL)
target_link_libraries(CampaignServerIDL CampaignCommonsIDL  CorbaTypesIDL CampaignServer_v350IDL CampaignCommons_v350IDL BillStatInfo_v350IDL BillStatInfoIDL StatInfoIDL)

add_library(BillStatServerSource STATIC
  BillStatServerSource.cpp 
  BillStatSource.cpp
)

target_link_libraries(BillStatServerSource
  Generics
  Commons
  CORBACommons
  CampaignServerIDL
  CampaignServerStubs_v350
  CampaignServer_v350IDL
  StatInfoIDL
  BillStatInfoIDL
)

add_library(CampaignConfigModifier STATIC
  CampaignConfigModifier.cpp 
  StatLogGeneralizerSource.cpp 
  StatDBSource.cpp
)

target_link_libraries(CampaignConfigModifier 
  Generics
  Commons
  Postgres
  LogGeneralizerIDL
)

add_executable(CampaignServer
  ModifyConfigSource.cpp 
  CampaignConfigDBSource.cpp 
  CampaignConfigServerSource.cpp 
  CampaignServerImpl.cpp 
  CampaignServerLogger.cpp 
  CampaignServerMain.cpp 
  CampaignServerStatValues.cpp 
  BillStatDBSource.cpp
)

target_link_libraries(CampaignServer
  LogCommons
  CampaignServerIDL
  Commons
  Postgres
  LogGeneralizerLib
  LogGeneralizerIDL
  BillStatServerSource
  ProcessControlVars

  ExpressionChannel
  CampaignConfigModifier
  BillStatServerSource
  CORBACommons
  CampaignServer_v350IDL
  CampaignServerStubs_v350
  CampaignCommons_v350IDL
  BillStatInfo_v350IDL
  CORBAConfigParser
  CampaignServerIDL
  StatInfoIDL
  BillStatInfoIDL
  AdServerCommonsXSD
  SNMPAgent
  Logger
  CampaignConfigModifier
  CorbaTypesIDL
  BillingServerIDL
  Generics ACE Logger Sync 
  pcre ssl crypto z event 
  String  
  xerces-c 
  CTROptimizer

  BillingServerIDL
  BillStatInfoIDL
  CorbaTypesIDL
  CampaignServerIDL 
  CampaignCommonsIDL 
  StatInfoIDL

  AdServerCommonsXSD 
  CampaignServerConfigXSD 
  DomainConfigXSD
  HostDistributionFileXSD 
    
  pcre ssl crypto z event  pthread 
  TAO TAO_AnyTypeCode  TAO_CodecFactory TAO_CosEvent TAO_CosNaming TAO_CosNotification TAO_DynamicAny TAO_EndpointPolicy TAO_FaultTolerance
  TAO_FT_ClientORB TAO_FT_ServerORB TAO_FTORB_Utils TAO_IORManip TAO_IORTable 
  TAO_Messaging TAO_PI TAO_PI_Server TAO_PortableGroup TAO_PortableServer TAO_Security TAO_SSLIOP TAO_TC TAO_TC_IIOP TAO_Valuetype ACE_SSL
)

ADD_SUBDIRECTORY(Compatibility)

install(TARGETS BillStatInfoIDL StatInfoIDL CampaignServerIDL BillStatServerSource CampaignServer  DESTINATION ${INSTALL_DIR})
