add_idl(BillingServerIDL BillingServer.idl ${IDL_DIRECTORY}/CampaignSvcs/BillingServer)

target_link_libraries(BillingServerIDL
  CORBACommons StatsIDL
  CorbaTypesIDL
  ProcessControlIDL
)

include(${UNIXCOMMONS}/src/UServerUtils/Grpc/Generator/GrpcTargets.cmake)
add_grpc_library(BillingServerProto
  PROTOS
    BillingServer.proto
  SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(BillingContainer STATIC
  BillingContainer.cpp
  BillingContainerState.cpp
)

add_library(CTROptimizer STATIC
  CTROptimizer.cpp
)

add_executable(BillingServer
  BillingServerImpl.cpp
  BillingServerMain.cpp
)

target_link_libraries(BillingServer
  ACE
  ACE_SSL
  AdServerCommonsXSD
  BillStatInfoIDL
  BillStatServerSource
  BillingContainer
  BillingServerConfigXSD
  BillingServerIDL
  BillingServerProto
  CORBACommons
  CTROptimizer
  CampaignCommonsIDL
  CampaignManagerConfigXSD
  CampaignServerConfigXSD
  CampaignServerIDL
  Commons
  CorbaTypesIDL
  DomainConfigXSD
  Generics
  HostDistributionFileXSD
  Logger
  ProcessControlIDL
  StatInfoIDL
  String
  Sync
  TAO
  TAO_AnyTypeCode
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
  UServerUtils
  crypto
  event
  pcre
  pthread
  ssl
  xerces-c
  z
)