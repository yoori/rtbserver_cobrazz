add_grpc_sources(CampaignServerGrpc CampaignServer.proto
  ${PB_DIRECTORY}/CampaignSvcs/CampaignServer)
target_sources(CampaignServerGrpc PRIVATE
  ../CampaignServerClient/CampaignServerClient.cpp
  CampaignServerGrpc.cpp CampaignServerGrpcAlgs.cpp CampaignServerPool.cpp)
target_link_libraries(CampaignServerGrpc PUBLIC gRPC::grpc++ protobuf::libprotobuf)

add_library(BillStatServerSource STATIC
  BillStatServerSource.cpp
  BillStatSource.cpp
)

target_link_libraries(BillStatServerSource
  PRIVATE
    Generics
    Commons
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
    Postgres
    BillStatServerSource
    CampaignConfigModifier
    CampaignServerGrpc
    AdServerCommonsXSD
    Generics
    HTTP
    Logger
    Sync
    CampaignServerConfigXSD
    DomainConfigXSD
    HostDistributionFileXSD
)
