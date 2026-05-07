add_library(ChannelClient SHARED
  ChannelCorbaClient.cpp
  ChannelDistributedGrpcClient.cpp
  ChannelGrpcAlgs.cpp
)

target_link_libraries(ChannelClient
  CORBACommons
  ChannelControllerGrpc
  ChannelManagerControllerIDL
  ChannelServerGrpcClient
  ChannelSessionFactory
)
