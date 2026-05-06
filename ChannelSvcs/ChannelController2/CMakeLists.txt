set(CHANNEL_CONTROLLER_GRPC_PROTO_DIR ${PB_DIRECTORY}/ChannelSvcs/ChannelController2)
set(CHANNEL_CONTROLLER_GRPC_PROTO ChannelControllerGrpc.proto)

add_grpc_sources(
  ChannelControllerGrpc
  ${CHANNEL_CONTROLLER_GRPC_PROTO}
  ${CHANNEL_CONTROLLER_GRPC_PROTO_DIR})

target_link_libraries(ChannelControllerGrpc protobuf::libprotobuf)

add_executable(ChannelController2
  ChannelController2Main.cpp
  ChannelController2Impl.cpp
  ChannelController2Grpc.cpp)

target_link_libraries(ChannelController2
  ChannelControllerGrpc
  ChannelController2ConfigXSD
  Commons
  Generics
  gRPC::grpc++)

target_include_directories(ChannelController2 PRIVATE
  ${CHANNEL_CONTROLLER_GRPC_PROTO_DIR})
