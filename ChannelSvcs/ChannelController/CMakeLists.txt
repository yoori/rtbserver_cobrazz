set(CHANNEL_CONTROLLER_GRPC_PROTO_DIR ${PB_DIRECTORY}/ChannelSvcs/ChannelController)
set(CHANNEL_CONTROLLER_GRPC_PROTO ChannelControllerGrpc.proto)

add_grpc_sources(
  ChannelControllerGrpc
  ${CHANNEL_CONTROLLER_GRPC_PROTO}
  ${CHANNEL_CONTROLLER_GRPC_PROTO_DIR})

target_link_libraries(ChannelControllerGrpc
  PUBLIC
    protobuf::libprotobuf
)

add_executable(ChannelController
  ChannelControllerMain.cpp
  ChannelControllerImpl.cpp
  ChannelControllerGrpc.cpp)

target_link_libraries(ChannelController
  PRIVATE
    TAO::PortableServer
    TAO::Valuetype
    ChannelControllerGrpc
    ChannelServerGrpc
    ChannelControllerConfigXSD
    Commons
    CORBAConfig
    CORBACommons
    Generics
    Logger
    ProcessControlIDL
    Sync
    gRPC::grpc++
)

target_include_directories(ChannelController PRIVATE
  ${CHANNEL_CONTROLLER_GRPC_PROTO_DIR})
