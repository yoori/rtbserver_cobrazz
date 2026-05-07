set(USER_INFO_CONTROLLER_GRPC_PROTO_DIR ${PB_DIRECTORY}/UserInfoSvcs/UserInfoController2)
set(USER_INFO_CONTROLLER_GRPC_PROTO UserInfoControllerGrpc.proto)

add_grpc_sources(
  UserInfoControllerGrpc
  ${USER_INFO_CONTROLLER_GRPC_PROTO}
  ${USER_INFO_CONTROLLER_GRPC_PROTO_DIR})

target_link_libraries(UserInfoControllerGrpc protobuf::libprotobuf)

add_executable(UserInfoController2
  UserInfoController2Main.cpp
  UserInfoController2Impl.cpp
  UserInfoController2Grpc.cpp)

target_link_libraries(UserInfoController2
  UserInfoControllerGrpc
  UserInfoManagerGrpc
  UserInfoController2ConfigXSD
  Commons
  Generics
  gRPC::grpc++)

target_include_directories(UserInfoController2 PRIVATE
  ${USER_INFO_CONTROLLER_GRPC_PROTO_DIR}
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoManager)
