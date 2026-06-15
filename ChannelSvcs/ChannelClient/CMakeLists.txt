add_library(ChannelGrpcClient SHARED
  ChannelClientUtils.cpp
  ChannelDistributedGrpcClient.cpp
  ChannelGrpcAlgs.cpp
)

target_link_libraries(ChannelGrpcClient
  Commons
  Logger
  ChannelControllerGrpc
  ChannelServerGrpcClient
  ChannelServerIDL
)
