add_executable(BillingAdmin
  Application.cpp
)

target_link_libraries(BillingAdmin
  PRIVATE
    BillingServerGrpc
    BillingServerGrpcClient
    Commons
    Generics
    GrpcBatch
    Logger
    Sync
    gRPC::grpc++
)
