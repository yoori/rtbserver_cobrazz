add_library(CTROptimizer STATIC
  CTROptimizer.cpp
)

add_library(BillingContainer STATIC
  BillingContainer.cpp
  BillingContainerState.cpp
)

target_link_libraries(BillingContainer
  CTROptimizer)

# BillingServerGrpc
set(BILLING_SERVER_GRPC_PROTO_DIR ${PB_DIRECTORY}/CampaignSvcs/BillingServer)
set(BILLING_SERVER_GRPC_PROTO BillingServerGrpc.proto)

add_grpc_sources(
  BillingServerGrpc
  ${BILLING_SERVER_GRPC_PROTO}
  ${BILLING_SERVER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(BillingServerGrpc protobuf::libprotobuf GrpcBatch)

add_executable(BillingServer
  BillingServerMain.cpp
  BillingServerCore.cpp
  BillingServerGrpc.cpp
)

target_link_libraries(BillingServer
  BillStatServerSource
  BillingContainer
  BillingServerGrpc
  HttpServer
  Commons
  CORBAConfig

  ProcessControlIDL
  BillStatInfoIDL
  CorbaTypesIDL
  CampaignServerIDL
  CampaignCommonsIDL
  StatInfoIDL

  AdServerCommonsXSD
  BillingServerConfigXSD
  CampaignManagerConfigXSD
  CampaignServerConfigXSD
  DomainConfigXSD
  HostDistributionFileXSD

  Generics
  Logger
  Sync
  String
  CORBACommons

  ACE
  xerces-c
  pcre ssl crypto z event pthread
  TAO TAO_AnyTypeCode  TAO_CodecFactory TAO_CosEvent TAO_CosNaming TAO_CosNotification TAO_DynamicAny
  TAO_EndpointPolicy TAO_FaultTolerance
  TAO_FT_ClientORB TAO_FT_ServerORB TAO_FTORB_Utils TAO_IORManip TAO_IORTable
  TAO_Messaging TAO_PI TAO_PI_Server TAO_PortableGroup TAO_PortableServer TAO_Security TAO_SSLIOP
  TAO_TC TAO_TC_IIOP TAO_Valuetype ACE_SSL
  jemalloc
)

target_include_directories(BillingServer PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)

if (TARGET Protobuf::libprotobuf)
  target_link_libraries(BillingServer Protobuf::libprotobuf)
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(BillingServer protobuf::libprotobuf)
else()
  target_link_libraries(BillingServer ${Protobuf_LIBRARIES})
endif()

target_link_libraries(BillingServer gRPC::grpc++)
