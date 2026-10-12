add_idl(BillStatInfoIDL BillStatInfo.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer NO_SERVANT)
add_idl(StatInfoIDL StatInfo.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer NO_SERVANT)
add_idl(CampaignServerIDL CampaignServer.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer)

add_dependencies(StatInfoIDL CampaignCommonsIDL)
add_grpc_sources(CampaignServerGrpc CampaignServer.proto
  ${PB_DIRECTORY}/CampaignSvcs/CampaignServer)
target_sources(CampaignServerGrpc PRIVATE
  ../CampaignServerClient/CampaignServerClient.cpp
  CampaignServerGrpc.cpp CampaignServerGrpcAlgs.cpp CampaignServerPool.cpp)
target_link_libraries(CampaignServerGrpc PUBLIC CampaignServerIDL gRPC::grpc++ protobuf::libprotobuf)

target_link_libraries(BillStatInfoIDL
  PUBLIC
    CorbaTypesIDL
)

target_link_libraries(CampaignServerIDL
  PUBLIC
    CampaignCommonsIDL
    BillStatInfoIDL
    StatInfoIDL
)

add_library(BillStatServerSource STATIC
  BillStatServerSource.cpp
  BillStatSource.cpp
)

target_link_libraries(BillStatServerSource
  PRIVATE
    Generics
    Commons
    CORBACommons
    BillStatInfoIDL
    CampaignServerGrpc
)

add_library(CampaignConfigModifier STATIC
  CampaignConfigModifier.cpp
  StatLogGeneralizerSource.cpp
  StatDBSource.cpp
)

target_link_libraries(CampaignConfigModifier
  PRIVATE
    Generics
    Commons
    CORBACommons
    Postgres
    LogGeneralizerGrpc
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
  PRIVATE
    GrpcBatch
    LogCommons
    Commons
    CORBAConfig
    Postgres
    BillStatServerSource
    CampaignCommonsIDL
    CampaignConfigModifier
    CORBACommons
    CorbaTypesIDL
    CampaignServerGrpc
    StatInfoIDL
    AdServerCommonsXSD
    Generics
    HTTP
    Logger
    Sync
    CampaignServerConfigXSD
    DomainConfigXSD
    HostDistributionFileXSD
    ProcessControlIDL
)
