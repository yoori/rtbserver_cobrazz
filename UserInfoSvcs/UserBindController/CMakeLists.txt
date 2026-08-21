set(USER_BIND_CONTROLLER_GRPC_PROTO_DIR ${PB_DIRECTORY}/UserInfoSvcs/UserBindController)
set(USER_BIND_CONTROLLER_GRPC_PROTO UserBindControllerGrpc.proto)

add_grpc_sources(
  UserBindControllerGrpc
  ${USER_BIND_CONTROLLER_GRPC_PROTO}
  ${USER_BIND_CONTROLLER_GRPC_PROTO_DIR})

target_link_libraries(UserBindControllerGrpc protobuf::libprotobuf)

add_executable(UserBindController
  UserBindControllerMain.cpp
  UserBindControllerImpl.cpp
  UserBindControllerGrpc.cpp)

target_link_libraries(UserBindController
  UserBindControllerGrpc
  UserBindServerGrpc
  UserBindControllerConfigXSD
  CORBAConfig
  Commons
  Generics
  gRPC::grpc++)

target_include_directories(UserBindController PRIVATE
  ${USER_BIND_CONTROLLER_GRPC_PROTO_DIR}
  ${PB_DIRECTORY}/UserInfoSvcs/UserBindServer)
