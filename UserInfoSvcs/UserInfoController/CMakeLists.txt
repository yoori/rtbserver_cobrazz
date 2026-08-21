set(USER_INFO_CONTROLLER_GRPC_PROTO_DIR ${PB_DIRECTORY}/UserInfoSvcs/UserInfoController)
set(USER_INFO_CONTROLLER_GRPC_PROTO UserInfoControllerGrpc.proto)

add_grpc_sources(
  UserInfoControllerGrpc
  ${USER_INFO_CONTROLLER_GRPC_PROTO}
  ${USER_INFO_CONTROLLER_GRPC_PROTO_DIR})

target_link_libraries(UserInfoControllerGrpc protobuf::libprotobuf)

add_executable(UserInfoController
  UserInfoControllerMain.cpp
  UserInfoControllerImpl.cpp
  UserInfoControllerGrpc.cpp)

target_link_libraries(UserInfoController
  UserInfoControllerGrpc
  UserInfoManagerGrpc
  UserInfoControllerConfigXSD
  CORBAConfig
  Commons
  Generics
  gRPC::grpc++)

target_include_directories(UserInfoController PRIVATE
  ${USER_INFO_CONTROLLER_GRPC_PROTO_DIR}
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoManager)
