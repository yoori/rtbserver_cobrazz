add_library(UserBindContainer STATIC
  ExternalIdHashAdapter.cpp
  ChunkUtils.cpp
  UserBindContainer.cpp
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

target_link_libraries(UserBindServerGrpc
  PUBLIC
    protobuf::libprotobuf
    GrpcBatch
)

add_library(UserBindServerGrpcClient STATIC)

add_adserver_grpc_client_sources(
  UserBindServerGrpcClient
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindServerGrpc.proto
  ${CMAKE_CURRENT_BINARY_DIR}
  AdServer::UserInfoSvcs
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(UserBindServerGrpcClient
  PUBLIC
    Commons
    GrpcBatch
    UserBindServerGrpc
    gRPC::grpc++
  PRIVATE
    Sync
)

# UserBindContainer
target_link_libraries(UserBindContainer
  PRIVATE
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

target_link_libraries(UserBindContainer
  PRIVATE
    UserBindRocksDBProfilePST
)

# UserBindServer
add_executable(UserBindServer
  UserBindServerMain.cpp
  UserBindServerCore.cpp
  UserBindOperationSaver.cpp
  UserBindOperationLoader.cpp
)

target_link_libraries(UserBindServer
  PRIVATE
    PkgConfig::Jemalloc
    Commons
    Generics
    GrpcBatch
    Logger
    MessageSaver
    LogCommons
    ProfileMap
    Sync
    UserBindContainer
    UserBindServerConfigXSD
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
  target_link_libraries(UserBindServer
    PRIVATE
      Protobuf::libprotobuf
  )
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(UserBindServer
    PRIVATE
      protobuf::libprotobuf
  )
else()
  target_link_libraries(UserBindServer
    PRIVATE
      ${Protobuf_LIBRARIES}
  )
endif()

target_link_libraries(UserBindServer
  PRIVATE
    gRPC::grpc++
)

# Temporary test binary for gRPC batching lifecycle checks. It keeps the same
# process/bootstrap code as UserBindServer, but short-circuits get_user_id.
add_executable(MockUserBindServer
  MockUserBindServerMain.cpp
  UserBindServerGrpc.cpp
)

target_link_libraries(MockUserBindServer
  PRIVATE
    PkgConfig::Jemalloc
    Commons
    Generics
    GrpcBatch
    Logger
    Sync
    UserBindServerGrpc
)

target_include_directories(MockUserBindServer PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)

target_compile_definitions(MockUserBindServer PRIVATE
  MOCK_USER_BIND_SERVER_FAST_GET_USER_ID
)

if(TARGET Protobuf::libprotobuf)
  target_link_libraries(MockUserBindServer
    PRIVATE
      Protobuf::libprotobuf
  )
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(MockUserBindServer
    PRIVATE
      protobuf::libprotobuf
  )
else()
  target_link_libraries(MockUserBindServer
    PRIVATE
      ${Protobuf_LIBRARIES}
  )
endif()

target_link_libraries(MockUserBindServer
  PRIVATE
    gRPC::grpc++
)
