add_library(ChannelGrpcClient SHARED
  ChannelClientUtils.cpp
  ChannelDistributedGrpcClient.cpp
  ChannelGrpcAlgs.cpp
)

target_link_libraries(ChannelGrpcClient
  PRIVATE
  Commons
  Logger
  ChannelControllerGrpc
  ChannelServerGrpcClient
  ChannelServerIDL
)

target_include_directories(ChannelGrpcClient PUBLIC
  ${CMAKE_BINARY_DIR}/ChannelSvcs/ChannelServer
  ${PB_DIRECTORY}/ChannelSvcs/ChannelServer)
