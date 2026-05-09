add_library(ChannelClient SHARED
  ChannelClientUtils.cpp
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
  FeConfigXSD
)
