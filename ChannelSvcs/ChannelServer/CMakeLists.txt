
add_library(ChannelContainer SHARED
  ContainerMatchers.cpp
  DictionaryMatcher.cpp
  SoftMatcher.cpp
  ChannelChunk.cpp
  ChannelContainer.cpp
  UpdateContainer.cpp
)

target_link_libraries(ChannelContainer
  Commons
  ChannelCommons
  DictionaryProviderIDL
)

# ChannelServerGrpc
set(CHANNEL_SERVER_GRPC_PROTO_DIR ${PB_DIRECTORY}/ChannelSvcs/ChannelServer)
set(CHANNEL_SERVER_GRPC_PROTO ChannelServerGrpc.proto)

add_grpc_sources(
  ChannelServerGrpc
  ${CHANNEL_SERVER_GRPC_PROTO}
  ${CHANNEL_SERVER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(ChannelServerGrpc protobuf::libprotobuf GrpcBatch)

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
  gRPC::grpc++)

add_executable(ChannelServer
  ChannelServerVariant.cpp
  ChannelServerCore.cpp
  ChannelServerCustomImpl.cpp
  ChannelServerGrpc.cpp
  ChannelServerControlImpl.cpp
  ChannelUpdateImpl.cpp
  ChannelServerMain.cpp
)

target_link_libraries(ChannelServer
  jemalloc

  HTTP
  HttpServer
  XMLUtility
  CORBAConfig
  CORBACommons
  AdServerCommonsXSD
  Postgres
  ChannelServerConfigXSD
  ChannelCommons
  ChannelServerIDL
  ChannelUpdateBaseIDL
  ChannelCommonsIDL
  ChannelContainer
  ChannelServerGrpc
  ChannelSessionFactory
  CampaignServerStubs_v350
  CampaignServer_v350IDL
  BLogic
  LogCommons
  Polyglot
  GenericSegmentor
)

target_include_directories(ChannelServer PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)

if(TARGET Protobuf::libprotobuf)
  target_link_libraries(ChannelServer Protobuf::libprotobuf)
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(ChannelServer protobuf::libprotobuf)
else()
  target_link_libraries(ChannelServer ${Protobuf_LIBRARIES})
endif()

target_link_libraries(ChannelServer gRPC::grpc++)

#install(TARGETS ChannelServer ChannelContainer  DESTINATION ${INSTALL_DIR})
