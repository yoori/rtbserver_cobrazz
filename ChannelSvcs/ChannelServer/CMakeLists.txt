add_library(ChannelContainer SHARED
  ContainerMatchers.cpp
  DictionaryMatcher.cpp
  SoftMatcher.cpp
  ChannelChunk.cpp
  ChannelContainer.cpp
  UpdateContainer.cpp
)

target_link_libraries(ChannelContainer
  PUBLIC
    Boost::headers
  PRIVATE
    ChannelCommons
    DictionaryProviderGrpc
)

# ChannelServerGrpc
set(CHANNEL_SERVER_GRPC_PROTO_DIR ${PB_DIRECTORY}/ChannelSvcs/ChannelServer)
set(CHANNEL_SERVER_GRPC_PROTO ChannelServerGrpc.proto)

add_grpc_sources(
  ChannelServerGrpc
  ${CHANNEL_SERVER_GRPC_PROTO}
  ${CHANNEL_SERVER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(ChannelServerGrpc
  PUBLIC
    protobuf::libprotobuf
    GrpcBatch
)

add_library(ChannelServerGrpcClient STATIC)

add_adserver_grpc_client_sources(
  ChannelServerGrpcClient
  ${CMAKE_SOURCE_DIR}/ChannelSvcs/ChannelServer/ChannelServerGrpc.proto
  ${CMAKE_CURRENT_BINARY_DIR}
  AdServer::ChannelSvcs
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(ChannelServerGrpcClient
  PUBLIC
    Commons
    GrpcBatch
    ChannelServerGrpc
    gRPC::grpc++
  PRIVATE
    Sync
)

add_library(ChannelUpdateGrpcClient SHARED ChannelUpdateGrpcClient.cpp)
target_link_libraries(ChannelUpdateGrpcClient
  PUBLIC ChannelServerGrpc
  PRIVATE Commons gRPC::grpc++)

add_executable(ChannelServer
  ChannelServerVariant.cpp
  ChannelServerCore.cpp
  ChannelServerGrpc.cpp
  ChannelServerMain.cpp
)

target_link_libraries(ChannelServer
  PRIVATE
    PkgConfig::Jemalloc
    HTTP
    HttpServer
    Generics
    Logger
    AdServerCommonsXSD
    Postgres
    ChannelServerConfigXSD
    ChannelCommons
    CampaignServerGrpc
    BLogic
    ChannelContainer
    ChannelServerGrpc
    ChannelUpdateGrpcClient
    ChannelSessionFactory
    Polyglot
    GenericSegmentor
)

target_include_directories(ChannelServer PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)

if (TARGET Protobuf::libprotobuf)
  target_link_libraries(ChannelServer
    PRIVATE
      Protobuf::libprotobuf
  )
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(ChannelServer
    PRIVATE
      protobuf::libprotobuf
  )
else()
  target_link_libraries(ChannelServer
    PRIVATE
      ${Protobuf_LIBRARIES}
  )
endif()

target_link_libraries(ChannelServer
  PRIVATE
    gRPC::grpc++
)
