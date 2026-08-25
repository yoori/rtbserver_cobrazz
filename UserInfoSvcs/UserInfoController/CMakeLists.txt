set(USER_INFO_CONTROLLER_GRPC_PROTO_DIR ${PB_DIRECTORY}/UserInfoSvcs/UserInfoController)
set(USER_INFO_CONTROLLER_GRPC_PROTO UserInfoControllerGrpc.proto)

add_grpc_sources(
  UserInfoControllerGrpc
  ${USER_INFO_CONTROLLER_GRPC_PROTO}
  ${USER_INFO_CONTROLLER_GRPC_PROTO_DIR})

target_link_libraries(UserInfoControllerGrpc
  PUBLIC
    protobuf::libprotobuf
)

add_executable(UserInfoController
  UserInfoControllerMain.cpp
  UserInfoControllerImpl.cpp
  UserInfoControllerGrpc.cpp)

target_link_libraries(UserInfoController
  PRIVATE
    TAO::PortableServer
    TAO::Valuetype
    UserInfoControllerGrpc
    UserInfoManagerGrpc
    UserInfoControllerConfigXSD
    CORBACommons
    Commons
    Generics
    Logger
    ProcessControlIDL
    Sync
    gRPC::grpc++
)

target_include_directories(UserInfoController PRIVATE
  ${USER_INFO_CONTROLLER_GRPC_PROTO_DIR}
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoManager)
