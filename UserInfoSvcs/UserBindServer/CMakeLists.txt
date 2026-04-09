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
  UserBindContainer.cpp
  UserBindChunk.cpp
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

# UserBindContainer
target_link_libraries(UserBindContainer
  Generics
  PrivacyFilter
  Commons
  LogCommons
  ProfileMap
)

# UserBindOperationProfilePST
add_pst(
  UserBindOperationProfilePST
  UserBindOperationProfile.pst
  ${PST_DIRECTORY}/UserInfoSvcs/UserBindServer
)

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
  MessageSaver
  LogCommons
  UserBindContainer
  UserBindServerConfigXSD
  UserBindServerIDL
  UserBindServerGrpc
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
