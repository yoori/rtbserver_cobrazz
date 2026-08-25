
add_executable(ChannelSearchAdmin
  Application.cpp
)
target_link_libraries(ChannelSearchAdmin
  PRIVATE
    TAO::Valuetype
    Generics
    GrpcBatch
    HTTP
    CorbaTypesIDL
    UtilCommons
    ChannelSearchServiceIDL
    ChannelServerIDL
    ChannelServerGrpc
    ChannelGrpcClient
)
