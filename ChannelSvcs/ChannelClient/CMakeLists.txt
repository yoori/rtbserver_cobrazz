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

add_library(ChannelClient SHARED
  ChannelCorbaClient.cpp
)

target_link_libraries(ChannelClient
  ChannelGrpcClient
  CORBACommons
  ChannelManagerControllerIDL
  ChannelSessionFactory
)
