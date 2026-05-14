add_idl(UserBindServerIDL UserBindServer.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserBindServer)

# UserBindServerIDL
target_link_libraries(UserBindServerIDL
  Commons
  CORBACommons
  CorbaTypesIDL
  UserBindOperationProfilePST
)

add_library(UserBindContainer STATIC
  ExternalIdHashAdapter.cpp
  ChunkUtils.cpp
  MigratingUserBindChunk.cpp
  UserBindContainer.cpp
  UserBindChunk.cpp
  UserBindRocksDBChunk.cpp
  BindRequestContainer.cpp
  BindRequestChunk.cpp
)

# UserBindServerGrpc
set(USER_BIND_GRPC_PROTO_DIR ${PB_DIRECTORY}/UserInfoSvcs/UserBindServer)
set(USER_BIND_GRPC_PROTO UserBindServerGrpc.proto)

add_grpc_sources(
  UserBindServerGrpc
  ${USER_BIND_GRPC_PROTO}
  ${USER_BIND_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(UserBindServerGrpc protobuf::libprotobuf GrpcBatch)

add_library(UserBindServerGrpcClient STATIC)

add_adserver_grpc_client_sources(
  UserBindServerGrpcClient
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindServerGrpc.proto
  ${CMAKE_CURRENT_BINARY_DIR}
  AdServer::UserInfoSvcs
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(UserBindServerGrpcClient
  PUBLIC
    GrpcBatch
    UserBindServerGrpc
    gRPC::grpc++)

# UserBindContainer
target_link_libraries(UserBindContainer
  Generics
  PrivacyFilter
  Commons
  LogCommons
  ProfileMap
  ProfileMapRocksDB
)

# UserBindOperationProfilePST
add_pst(
  UserBindOperationProfilePST
  UserBindOperationProfile.pst
  ${PST_DIRECTORY}/UserInfoSvcs/UserBindServer
)

# UserBindRocksDBProfilePST
add_pst(
  UserBindRocksDBProfilePST
  UserBindRocksDBProfile.pst
  ${PST_DIRECTORY}/UserInfoSvcs/UserBindServer
)

target_link_libraries(UserBindContainer UserBindRocksDBProfilePST)

# UserBindServer
add_executable(UserBindServer
  UserBindServerMain.cpp
  UserBindServerImpl.cpp
  UserBindServerCore.cpp
  UserBindOperationSaver.cpp
  UserBindOperationLoader.cpp
)

target_link_libraries(UserBindServer
  CORBACommons
  PrivacyFilter
  ProcessControlVars
  Commons
  ProfileMap
  ProfileMapRocksDB
  MessageSaver
  LogCommons
  UserBindContainer
  UserBindServerConfigXSD
  UserBindServerIDL
  UserBindServerGrpc
  HttpServer
)

target_sources(UserBindServer PRIVATE
  UserBindServerGrpc.cpp
)

target_include_directories(UserBindServer PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)

if(TARGET Protobuf::libprotobuf)
  target_link_libraries(UserBindServer Protobuf::libprotobuf)
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(UserBindServer protobuf::libprotobuf)
else()
  target_link_libraries(UserBindServer ${Protobuf_LIBRARIES})
endif()

target_link_libraries(UserBindServer gRPC::grpc++)
