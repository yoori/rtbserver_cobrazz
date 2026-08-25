add_executable(UserBindAdmin
  Application.cpp
)

target_link_libraries(UserBindAdmin
  PRIVATE
    gRPC::grpc++
    Generics
    Commons
    Logger
    UserBindClient
    UserBindServerGrpcClient
    UserBindControllerGrpc
)

target_include_directories(UserBindAdmin PRIVATE
  ${PB_DIRECTORY}/UserInfoSvcs/UserBindServer
  ${PB_DIRECTORY}/UserInfoSvcs/UserBindController
)
