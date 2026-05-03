set(USER_BIND_CONTROLLER_GRPC_PROTO_DIR ${PB_DIRECTORY}/UserInfoSvcs/UserBindController2)
set(USER_BIND_CONTROLLER_GRPC_PROTO UserBindControllerGrpc.proto)

add_grpc_sources(
  UserBindControllerGrpc
  ${USER_BIND_CONTROLLER_GRPC_PROTO}
  ${USER_BIND_CONTROLLER_GRPC_PROTO_DIR})

target_link_libraries(UserBindControllerGrpc protobuf::libprotobuf)

add_executable(UserBindController2
  UserBindController2Main.cpp
  UserBindController2Impl.cpp
  UserBindController2Grpc.cpp)

target_link_libraries(UserBindController2
  UserBindControllerGrpc
  UserBindServerGrpc
  UserBindController2ConfigXSD
  Commons
  Generics
  gRPC::grpc++)

target_include_directories(UserBindController2 PRIVATE
  ${USER_BIND_CONTROLLER_GRPC_PROTO_DIR}
  ${PB_DIRECTORY}/UserInfoSvcs/UserBindServer)
