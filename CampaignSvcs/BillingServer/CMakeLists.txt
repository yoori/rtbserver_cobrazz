add_library(CTROptimizer STATIC
  CTROptimizer.cpp
)

target_link_libraries(CTROptimizer
  PRIVATE
    Generics
)

add_library(BillingContainer STATIC
  BillingContainer.cpp
  BillingContainerState.cpp
)

target_link_libraries(BillingContainer
  PRIVATE
    CTROptimizer
    Generics
)

# BillingServerGrpc
set(BILLING_SERVER_GRPC_PROTO_DIR ${PB_DIRECTORY}/CampaignSvcs/BillingServer)
set(BILLING_SERVER_GRPC_PROTO BillingServerGrpc.proto)

add_grpc_sources(
  BillingServerGrpc
  ${BILLING_SERVER_GRPC_PROTO}
  ${BILLING_SERVER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(BillingServerGrpc
  PUBLIC
    protobuf::libprotobuf
    GrpcBatch
)

add_executable(BillingServer
  BillingServerMain.cpp
  BillingServerCore.cpp
  BillingServerGrpc.cpp
)

target_link_libraries(BillingServer
  PRIVATE
    BillStatServerSource
    BillingContainer
    BillingServerGrpc
    HttpServer
    Commons
    CORBAConfig
    CampaignServerIDL
    AdServerCommonsXSD
    BillingServerConfigXSD
    Generics
    Logger
    ProcessControlIDL
    Sync
    CORBACommons
    PkgConfig::Jemalloc
)

target_include_directories(BillingServer PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)

if (TARGET Protobuf::libprotobuf)
  target_link_libraries(BillingServer
    PRIVATE
      Protobuf::libprotobuf
  )
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(BillingServer
    PRIVATE
      protobuf::libprotobuf
  )
else()
  target_link_libraries(BillingServer
    PRIVATE
      ${Protobuf_LIBRARIES}
  )
endif()

target_link_libraries(BillingServer
  PRIVATE
    gRPC::grpc++
)
