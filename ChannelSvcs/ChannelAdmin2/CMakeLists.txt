add_executable(ChannelAdmin2
  ChannelAdmin2.cpp
)

target_link_libraries(ChannelAdmin2
  Logger
  Generics
  Commons
  ChannelClient
  ChannelControllerGrpc
  ChannelServerGrpcClient
)

target_include_directories(ChannelAdmin2 PRIVATE
  ${PB_DIRECTORY}/ChannelSvcs/ChannelController
  ${PB_DIRECTORY}/ChannelSvcs/ChannelServer
)
