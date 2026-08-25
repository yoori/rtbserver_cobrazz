add_executable(ChannelAdmin
  ChannelAdmin.cpp
)

target_link_libraries(ChannelAdmin
  PRIVATE
    TAO::Valuetype
    Logger
    Generics
    HTTP
    BLogic
    Commons
    CorbaTypesIDL
    ChannelGrpcClient
    ChannelControllerGrpc
    ChannelServerGrpcClient
)

target_include_directories(ChannelAdmin PRIVATE
  ${PB_DIRECTORY}/ChannelSvcs/ChannelController
  ${PB_DIRECTORY}/ChannelSvcs/ChannelServer
)
